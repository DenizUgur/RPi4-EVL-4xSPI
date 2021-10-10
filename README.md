# EVL Setup Guide for Raspberry Pi 4B

In the end of this guide, you will be able to use 4 SPI peripherals with DMA enabled on EVL kernel. SPI0,4,5,6 are used and each of them sample a SPI encoder at 30KHz (40-bit at 1.25MHz).

A tested SD card image is available from [here](https://github.com/DenizUgur/RPi4-EVL-4xSPI/releases/tag/V1). However, setup is pretty easy and it is highly recommended you do it manually.

# Getting the resources

Prepare the environment

  ```bash
  $~> mkdir development
  $~> cd development

  $~/development> wget https://raw.githubusercontent.com/DenizUgur/RPi4-EVL-4xSPI/master/0001-spidev-x4-rpi4b.patch

  $~/development> sudo apt install build-essential \
                              git \
                              bison \
                              flex \
                              gcc-arm-linux-gnueabihf \
                              g++-arm-linux-gnueabihf
  ```

EVL patched Linux

  ```bash
  $~/development> git clone --depth 1 --branch evl/v5.10 https://git.evlproject.org/linux-evl.git

  $~/development> cd linux-evl

  $~/development/linux-evl> git checkout 536c8af4842ce95fd6bea497ff3c960cf3a29482
  ```

Apply the provided patch to the Linux Tree

  ```bash
  $~/development/linux-evl> patch -p1 < ../0001-spidev-x4-rpi4b.patch
  ```

EVL Libraries

  ```bash
  $~/development/linux-evl> cd ..

  $~/development> git clone --depth 1 https://git.evlproject.org/libevl.git

  $~/development> cd libevl

  $~/development/libevl> git checkout d12db5d2688ca3aa06a738a924171ef5fe85c6ab
  ```

# Compiling

Okay, this section needs no explanation.

```bash
export BASE=$(realpath $PWD)
export USER=$(whoami)
export SD_BOOT=boot
export SD_FS=rootfs
export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-

mkdir -p linux-evl-build
mkdir -p libevl-build

# Prepare EVL Kernel
cd $BASE/linux-evl-build

make -j8 -C $BASE/linux-evl O=$PWD ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE multi_v7_defconfig

wget https://raw.githubusercontent.com/DenizUgur/RPi4-EVL-4xSPI/master/evl_arm_defconfig -O .config

time make -j8 -C $BASE/linux-evl O=$PWD ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE

cd $BASE/libevl-build
time make -j8 -C $BASE/libevl O=$PWD ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE UAPI=$BASE/linux-evl all
```

# Installing to the target

Download and flash Raspberry Pi OS Lite from [raspberrypi's website](https://www.raspberrypi.org/software/operating-systems/). You can use Balena Etcher (macOS) or using the command below.

```bash
dd if=raspian.img of=/dev/sdX bs=1M conv=fsync
```

where `/dev/sdX` is the device path of your sd card found by `lsblk`.

After flashing the device. Boot it up for once and configure keyboard, localization, network etc. It is recommended that you fix the network IP at this stage as serial communication is not tested. Also install and start `openssh-server`.

Then you can power it off and plug the sd card into your host computer. Move on with the following commands.

```bash
cd $BASE/linux-evl-build
time make -j8 -C $BASE/linux-evl O=$PWD ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE INSTALL_MOD_PATH=/media/$USER/$SD_FS modules_install

cp arch/arm/boot/zImage /media/$USER/$SD_BOOT/kernel-evl.img
cp arch/arm/boot/dts/bcm2711-rpi-4-b.dtb /media/$USER/$SD_BOOT/bcm2711-rpi-4-b-evl.dtb

cd $BASE/libevl-build
time make -j8 -C $BASE/libevl O=$PWD ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE UAPI=$BASE/linux-evl DESTDIR=/media/$USER/$SD_FS/usr/evl install_all

# Modify boot configuration
echo "kernel=kernel-evl.img" >> /media/$USER/$SD_BOOT/config.txt
echo "device_tree=bcm2711-rpi-4-b-evl.dtb" >> /media/$USER/$SD_BOOT/config.txt

# Modify .bashrc
echo "export PATH=$PATH:/usr/evl/bin" >> /media/$USER/$SD_FS/home/pi/.bashrc
echo "export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/evl/lib" >> /media/$USER/$SD_FS/home/pi/.bashrc

sync
```

# Checking the installation

Boot up the device and execute the following commands.

```bash
sudo -Es
```

The following command should return mostly okay. There will be 2-3 failed tests as they are not enabled in the kernel configuration.

```bash
evl test
```

Lastly test if the OOB SPI is working.

```bash
echo performance | tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

busybox devmem 0xfe204e00 32 0x03800000

/usr/evl/tidbits/oob-spi /dev/spidev0.0
/usr/evl/tidbits/oob-spi /dev/spidev1.0
/usr/evl/tidbits/oob-spi /dev/spidev2.0
/usr/evl/tidbits/oob-spi /dev/spidev3.0
```

> Note: `prepare.sh` file contains these commands too.

If everything returns an output of repeating 42's with the duration of transfer then it's okay.

# Extras

An example application was provided, `oob-spi-v2.c`. You can compile this application on either host or target but headers and `libevl.so` must be present in your workspace.

```bash
gcc oob-spi-v2.c -lrt -lpthread -I/usr/evl/include /usr/evl/lib/libevl.so -O2 -g -o oob-spi-v2
```

An example systemd service file was provided, `encoder.service`. You can enable this service by executing following commands.

```bash
sudo mv /home/pi/oob-spi-v2 /usr/local/bin/oob-spi-v2
sudo mv /home/pi/encoder.service /etc/systemd/system/encoder.service

sudo systemctl start encoder
sudo systemctl enable encoder
```

> Warning: Do not forget that you need to execute the command starting with busybox (look at previous section) at either boot or before executing the sample application.