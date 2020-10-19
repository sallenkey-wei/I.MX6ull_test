#!/bin/bash

make distclean
make mx6ull_14x14_evk_emmc_defconfig
make V=1 -j8
