#ifndef  __FT5X06_H
#define  __FT5X06_H

#define MAX_SUPPORT_POINTS			5
#define TOUCH_EVENT_RESERVED 		0x03
#define TOUCH_EVENT_DOWN 			0x00
#define TOUCH_EVENT_UP				0x01



#define FT5X06_TD_STATUS_REG		0x02   /* 状态寄存器地址            */
#define FT6X06_DEVICE_MODE_REG      0x00   /* 模式寄存器              */
#define FT5426_IDG_MODE_REG         0xA4   /* 中断模式               */
#define FT5X06_READLEN              29     /* 要读取的寄存器个数 */

#endif
