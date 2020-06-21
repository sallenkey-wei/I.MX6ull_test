#include "fsl_common.h"
#include "fsl_iomuxc.h"
#include "MCIMX6Y2.h"
#include "beep.h"



void beep_init(){
	IOMUXC_SetPinMux(IOMUXC_SNVS_SNVS_TAMPER1_GPIO5_IO01, 0);
	IOMUXC_SetPinConfig(IOMUXC_SNVS_SNVS_TAMPER1_GPIO5_IO01, 0x10b0);
	GPIO5->GDIR |= (1<<1);
	GPIO5->DR |= (1<<1);
}

void beep_switch(com_status_t status){
	switch(status){
	case ON:
		GPIO5->DR &= ~(1<<1);
		break;
	case OFF:
		GPIO5->DR |= (1<<1);
		break;
	}
}

