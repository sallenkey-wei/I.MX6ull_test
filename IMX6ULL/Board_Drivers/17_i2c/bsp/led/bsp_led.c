#include "fsl_common.h"
#include "fsl_iomuxc.h"
#include "MCIMX6Y2.h"
#include "bsp_led.h"

void led_init()
{
	IOMUXC_SetPinMux(IOMUXC_GPIO1_IO03_GPIO1_IO03, 0);
	IOMUXC_SetPinConfig(IOMUXC_GPIO1_IO03_GPIO1_IO03, 0x10B0);
	GPIO1->GDIR |= (1 << 3);
	GPIO1->DR |= (1 << 3);
}

void led_on()
{
	GPIO1->DR &= ~(1 << 3);
}

void led_off()
{
	GPIO1->DR |= (1 << 3);
}

void led_switch(unsigned char ledState)
{
	switch (ledState)
	{
	case ON:
		led_on();
		break;
	case OFF:
		led_off();
		break;
	default:
		break;
	}
}