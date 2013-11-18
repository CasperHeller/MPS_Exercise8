insmod hotplug_dac7612_spi_device.ko
insmod dac7612mod.ko
mknod /dev/dac0 c 64 0
mknod /dev/dac1 c 64 1
