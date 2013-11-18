#include <linux/err.h>
#include <plat/mcspi.h>
#include <asm/uaccess.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/spi/spi.h>
#include "ads7870.h"
#include <linux/module.h>

#define MODULE_DEBUG 0

static struct spi_device *ads7870_spi_device = NULL;

int ads7870_spi_read_reg8(u8 addr, u8* value)
{
	struct spi_transfer t[2];
	struct spi_message m;
	u8 cmd;
	u8 data = 0;

    /* Check for valid spi device */
    if(!ads7870_spi_device)
      return -ENODEV;
    
	/* Create Cmd byte:
	 *
	 * | 0|RD| 8|     ADDR     |
	 *   7  6  5  4  3  2  1  0
     */	 
	cmd = (1<<6) | (0<<5) | (addr & 0x1f);

	/* Init Message */
	memset(t, 0, sizeof(t));
	spi_message_init(&m);
	m.spi = ads7870_spi_device;

	/* Configure tx/rx buffers */
	t[0].tx_buf = &cmd;
	t[0].rx_buf = NULL;
	t[0].len = 1;
	spi_message_add_tail(&t[0], &m);

	t[1].tx_buf = NULL;
	t[1].rx_buf = &data;
	t[1].len = 1;
	spi_message_add_tail(&t[1], &m);

	/* Transmit SPI Data (blocking) */
	spi_sync(m.spi, &m);

	if(MODULE_DEBUG)
	  printk(KERN_DEBUG "ADS7870: Read Reg8 Addr 0x%02x Data: 0x%02x\n", cmd, data);

    *value = data;
	return 0;
}

int ads7870_spi_read_reg16(u8 addr, u16* value)
{
	struct spi_transfer t[2];
	struct spi_message m;
	u8 cmd;
	u16 data = 0;

    /* Check for valid spi device */
    if(!ads7870_spi_device)
      return -ENODEV;

	/* Create Cmd byte:
	 *
	 * | 0|RD|16|     ADDR     |
	 *   7  6  5  4  3  2  1  0
     */	 
	cmd = (1<<6) | (1<<5) | (addr & 0x1f);

	/* Init Message */
	memset(t, 0, sizeof(t));
	spi_message_init(&m);
	m.spi = ads7870_spi_device;

	/* Configure tx/rx buffers */
	t[0].tx_buf = &cmd;
	t[0].rx_buf = NULL;
	t[0].len = 1;
	if(MODULE_DEBUG)
	  printk("requesting data from addr 0x%x\n", cmd);
	spi_message_add_tail(&t[0], &m);

	t[1].tx_buf = NULL;
	t[1].rx_buf = &data;
	t[1].len = 2;
	spi_message_add_tail(&t[1], &m);

	/* Transmit SPI Data (blocking) */
	spi_sync(m.spi, &m);

	if(MODULE_DEBUG)
	  printk(KERN_DEBUG "ADS7870: Read Reg16 Addr 0x%02x Data: 0x%04x\n", cmd, data);

    *value = data;
	return 0;
}


int ads7870_spi_write_reg8(u8 addr, u8 data)
{
  struct spi_transfer t[2];
  struct spi_message m;
  u8 cmd;

  /* Check for valid spi device */
  if(!ads7870_spi_device)
    return -ENODEV;

  
  /* Create Cmd byte:
   *
   * | 0|WR| 8|     ADDR     |
   *   7  6  5  4  3  2  1  0
   */ 
  cmd =  (0<<6) | (0<<5) | (addr & 0x1f);

  /* Init Message */
  memset(&t, 0, sizeof(t)); 
  spi_message_init(&m);
  m.spi = ads7870_spi_device;

  if(MODULE_DEBUG)
    printk(KERN_DEBUG "ADS7870: Write Reg8 Addr 0x%x Data 0x%02x\n", addr, data); 

  /* Configure tx/rx buffers */
  t[0].tx_buf = &cmd;
  t[0].rx_buf = NULL;
  t[0].len = 1;
  spi_message_add_tail(&t[0], &m);

  t[1].tx_buf = &data;
  t[1].rx_buf = NULL;
  t[1].len = 1;
  spi_message_add_tail(&t[1], &m);

  /* Transmit SPI Data (blocking) */
  spi_sync(m.spi, &m);

  return 0;
}

/*
 * ADS7870 Probe
 * Used by the SPI Master to probe the device
 * When the SPI device is registered. 
 */
static int __devinit ads7870_spi_probe(struct spi_device *spi)
{
  int err;
  u8 value;
  
  
  spi->bits_per_word = 8;  
  spi_setup(spi);

  /* In this case we assume just one device */ 
  ads7870_spi_device = spi;  

  
  err = ads7870_spi_read_reg8(ADS7870_ID, &value);
  printk(KERN_DEBUG "Probing ADS7870, ADS7870 Revision %i\n", 
         value);

  
  return err;
}

static int __devexit ads7870_remove(struct spi_device *spi)
{
  printk (KERN_ALERT "ads7870 SPI driver has been removed while in use\n");
  
  ads7870_spi_device = 0;
  return 0;
}

static struct spi_driver ads7870_spi_driver = {
  .driver = {
    .name = "ads7870",
    .bus = &spi_bus_type,
    .owner = THIS_MODULE,
  },
  .probe = ads7870_spi_probe,
  .remove = __devexit_p(ads7870_remove),
};


/*
 * Init / Exit routines called from 
 * character driver. Init registers the spi driver
 * and the spi host probes the device upon this.
 * Exit unregisters the driver and the spi host
 * calls _remove upon this
 */
int ads7870_spi_init(void)
{
  
  int err;

  err = spi_register_driver(&ads7870_spi_driver);
  
  if(err<0)
    printk (KERN_ALERT "Error %d registering the ads7870 SPI driver\n", err);
  
  /* 
   * Probe should have been called if a device is available 
   * setting the current spi device
   * if not return -ENODEV
   */
  if (ads7870_spi_device == NULL)
  {
    spi_unregister_driver(&ads7870_spi_driver); 
    err = -ENODEV;
  }
  
  return err;
}

int ads7870_spi_exit(void)
{
  /*
   * Un-register spi driver and device
   * Spi host calls _remove upon this
   */
  spi_unregister_driver(&ads7870_spi_driver); 

  return 0;
}


