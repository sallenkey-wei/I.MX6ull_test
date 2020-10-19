#!/bin/bash

make distclean
make mx6ull_alientek_emmc_defconfig
make V=1 -j8
