#ifndef DAC7612_SPI_H
#define DAC7612_SPI_H
#include <linux/spi/spi.h>
#include <linux/input.h>

int dac7612_spi_write_reg14(u8 addr, u16 data);
int dac7612_spi_init(void);
int dac7612_spi_exit(void);

#endif
