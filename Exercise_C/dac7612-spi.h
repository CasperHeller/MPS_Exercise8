#ifndef dac7612_SPI_H
#define dac7612_SPI_H
#include <linux/spi/spi.h>
#include <linux/input.h>

int dac7612_spi_read_reg8(u8 addr, u8* value);
int dac7612_spi_read_reg16(u8 addr, u16* value);
int dac7612_spi_write_reg8(u8 addr, u8 data);
int dac7612_spi_init(void);
int dac7612_spi_exit(void);

#endif

