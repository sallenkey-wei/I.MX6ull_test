#include <linux/major.h>
#include <linux/vmalloc.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/blkdev.h>
#include <linux/bitops.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/hdreg.h>

#define DEVICE_NAME "ramdisk"
#define RAMDISK_SIZE (2 * 1024 *1024)
#define RAMDISK_MINOR 3 // 最多支持3个分区

struct ramdisk_dev {
    int major;
    void * ramdisk_buf;
    spinlock_t lock;
    struct gendisk  * gendisk;
    struct request_queue * req_queue;
};

struct ramdisk_dev ramdisk_device;


static int ramdisk_open(struct block_device *bdev, fmode_t mode)
{
    printk("ramdisk_open\n");
    return 0;
}

static void ramdisk_release(struct gendisk *disk, fmode_t mode)
{
    printk("ramdisk_release\n");
    return;
}

static int ramdisk_getgeo(struct block_device *dev, struct hd_geometry *geo)
{
    geo->heads = 2; // 磁头
    geo->cylinders = 32; // 柱面(磁道)
    geo->sectors = RAMDISK_SIZE / (geo->heads * geo->cylinders * 512); // 扇区总数, 每个扇区512字节
    return 0;
}

static const struct block_device_operations ramdisk_fops =
{
	.owner		= THIS_MODULE,
	.open		= ramdisk_open,
	.release	= ramdisk_release,
    .getgeo     = ramdisk_getgeo,
};

static void ramdisk_request_fn(struct request_queue *q)
{
    struct request *req;

    req = blk_fetch_request(q); // 内部使用电梯调度算法, 模拟的时机械硬盘
    while (req) {
        unsigned long start = blk_rq_pos(req) << 9;
        unsigned long len  = blk_rq_cur_bytes(req);
        int err = 0;
        void *buffer = bio_data(req->bio);

        if (start + len > RAMDISK_SIZE) {
            pr_err(DEVICE_NAME ": bad access: block=%llu, "
                    "count=%u\n",
                    (unsigned long long)blk_rq_pos(req),
                    blk_rq_cur_sectors(req));
            err = -EIO;
            goto done;
        }

        if (rq_data_dir(req) == READ)
            memcpy(buffer, ramdisk_device.ramdisk_buf + start, len);
        else
            memcpy(ramdisk_device.ramdisk_buf + start, buffer, len);
    done:
        if (!__blk_end_request_cur(req, err))
            req = blk_fetch_request(q);
    }
}

static int __init ramdisk_init(void) {
    int ret = 0;
    ramdisk_device.major = register_blkdev(0, DEVICE_NAME);
    if (ramdisk_device.major < 0) {
        printk("auto alloc ramkdisk's major dev_num failed.\n");
        ret = -EBUSY;
        goto err;
    }
    printk("ramdisk major num = %d\n", ramdisk_device.major);

    ramdisk_device.ramdisk_buf = vmalloc(RAMDISK_SIZE);
    if (!ramdisk_device.ramdisk_buf) {
        ret = -ENOMEM;
        goto out_mem;
    }

    spin_lock_init(&ramdisk_device.lock);

    // alloc and init gendisk
    ramdisk_device.gendisk = alloc_disk(RAMDISK_MINOR);
    if (!ramdisk_device.gendisk) {
        ret = -ENOMEM;
        goto out_disk;
    }

    ramdisk_device.req_queue = blk_init_queue(ramdisk_request_fn, &ramdisk_device.lock);
    if (!ramdisk_device.req_queue) {
        ret = -ENOMEM;
        goto out_queue;
    }

    ramdisk_device.gendisk->major = ramdisk_device.major;
    ramdisk_device.gendisk->first_minor = 0;
    ramdisk_device.gendisk->fops = &ramdisk_fops;
    sprintf(ramdisk_device.gendisk->disk_name, DEVICE_NAME);

    ramdisk_device.gendisk->queue = ramdisk_device.req_queue;
    set_capacity(ramdisk_device.gendisk, RAMDISK_SIZE/512);

    add_disk(ramdisk_device.gendisk); // 和del_gendisk()配对

    return 0;

out_queue:
    put_disk(ramdisk_device.gendisk); // 减少引用计数，当计数为0时会释放占用的资源 和alloc_disk配对
out_disk:
    vfree(ramdisk_device.ramdisk_buf);
out_mem:
    unregister_blkdev(ramdisk_device.major, DEVICE_NAME);
err:
    return ret;
}

static void __exit ramdisk_exit(void) {
    del_gendisk(ramdisk_device.gendisk);
    put_disk(ramdisk_device.gendisk);
    blk_cleanup_queue(ramdisk_device.req_queue);
    vfree(ramdisk_device.ramdisk_buf);
    unregister_blkdev(ramdisk_device.major, DEVICE_NAME);
}

module_init(ramdisk_init);
module_exit(ramdisk_exit);

MODULE_AUTHOR("wyy");
MODULE_LICENSE("GPL");

