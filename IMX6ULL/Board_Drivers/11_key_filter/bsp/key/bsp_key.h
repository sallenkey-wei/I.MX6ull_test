#ifndef __BSP_KEY_H
#define __BSP_KEY_H

typedef enum{
	KEY_NONE=0,
	KEY0,
}key_status;

void key_init();
key_status key_getvalue();


#endif
