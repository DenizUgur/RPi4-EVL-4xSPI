#!/bin/bash
echo -e "\e[41mChange scaling governor to performance\e[0m"
echo performance | tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

echo -e "\e[41mEnabling DMA for SPI devices\e[0m"
busybox devmem 0xfe204e00 32 0x03800000

echo -e "\e[41mDry-run for SPI devices\e[0m"
/usr/evl/tidbits/oob-spi /dev/spidev0.0
/usr/evl/tidbits/oob-spi /dev/spidev1.0
/usr/evl/tidbits/oob-spi /dev/spidev2.0
/usr/evl/tidbits/oob-spi /dev/spidev3.0
