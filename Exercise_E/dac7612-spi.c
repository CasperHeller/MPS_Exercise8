#include <linux/err.h>
#include <plat/mcspi.h>
#include <asm/uaccess.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/spi/spi.h>
#include "dac7612.h"
#include <linux/module.h>

#define MODULE_DEBUG 1

MODULE_AUTHOR("TeamMPS");
MODULE_LICENSE("Dual BSD/GPL");

static struct spi_device *dac7612_spi_device = NULL;

int dac7612_spi_write_reg14(u8 addr, u16 data)
{
  struct spi_transfer t[2];
  struct spi_message m;
  u16 cmd;

  /* Check for valid spi device */
  if(!dac7612_spi_device)
    return -ENODEV;

  /* Create Cmd byte:
   *
   * | ADDR   |        DATA               |
   * | 13 12  | 11 10 9 8 7 6 5 4 3 2 1 0 |
   */
  
  cmd = (addr << 12);   // Roll addr up to bit 13 and 12
  cmd |= (data & 0b111111111111); // Mask data, so only bit 11-0 is written to cmd

  /* Init Message */
  memset(&t, 0, sizeof(t)); 
  spi_message_init(&m);
  m.spi = dac7612_spi_device;

  if(MODULE_DEBUG)
    printk(KERN_DEBUG "DAC7612: Write Reg14 Addr 0x%x Data 0x%02x\n Command: 0x%x\n", addr, data, cmd); 

  /* Configure tx/rx buffers */
  t[0].tx_buf = &cmd;
  t[0].rx_buf = NULL;
  t[0].len = 2;
  spi_message_add_tail(&t[0], &m);

  /* Transmit SPI Data (blocking) */
  spi_sync(m.spi, &m);

  return 0;
}

/*
 * DAC7612 Probe
 * Used by the SPI Master to probe the device
 * When the SPI device is registered. 
 */
static int __devinit dac7612_spi_probe(struct spi_device *spi)
{
  spi->bits_per_word = 14;  
  spi_setup(spi);

  /* In this case we assume just one device */ 
  dac7612_spi_device = spi;  
  
  return 0;
}

static int __devexit dac7612_remove(struct spi_device *spi)
{
  printk (KERN_ALERT "dac7612 SPI driver has been removed while in use\n");
  
  dac7612_spi_device = 0;
  return 0;
}

static struct spi_driver dac7612_spi_driver = {
  .driver  = {
    .name  = "dac7612",
    .bus   = &spi_bus_type,
    .owner = THIS_MODULE,
  },
  .probe   = dac7612_spi_probe,
  .remove  = __devexit_p(dac7612_remove),
};


/*
 * Init / Exit routines called from 
 * character driver. Init registers the spi driver
 * and the spi host probes the device upon this.
 * Exit unregisters the driver and the spi host
 * calls _remove upon this
 */
int dac7612_spi_init(void)
{
  
  int err;

  err = spi_register_driver(&dac7612_spi_driver);
  
  if(err<0)
    printk (KERN_ALERT "Error %d registering the dac7612 SPI driver\n", err);
  
  /* 
   * Probe should have been called if a device is available 
   * setting the current spi device
   * if not return -ENODEV
   */
  if (dac7612_spi_device == NULL)
  {
    spi_unregister_driver(&dac7612_spi_driver); 
    err = -ENODEV;
  }
  
  return err;
}

int dac7612_spi_exit(void)
{
  /*
   * Un-register spi driver and device
   * Spi host calls _remove upon this
   */
  spi_unregister_driver(&dac7612_spi_driver); 

  return 0;
}
