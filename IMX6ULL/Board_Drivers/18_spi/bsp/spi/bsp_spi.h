#ifndef __BSP_SPI_H
#define __BSP_SPI_H

#include "imx6ul.h"

void spi_init(ECSPI_Type *base);
unsigned char spich0_readwrite_byte(ECSPI_Type *base, unsigned char txdata);

#endif