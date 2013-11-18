insmod hotplug_dac7612_spi_device.ko
insmod dac7612mod.ko

mknod /dev/dacA c 60 0
mknod /dev/dacB c 60 1
