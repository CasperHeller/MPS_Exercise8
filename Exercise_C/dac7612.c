#include <linux/err.h>
#include <linux/ctype.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/module.h>
#include "dac7612.h"
#include "dac7612-spi.h"

#define DAC7612_MAJOR       64
#define DAC7612_MINOR        0
#define MAXLEN              64
#define NBR_ADC_CH           8
#define COMPLIMENTARY_BIT   11

#define MODULE_DEBUG 0
#define USECDEV 0

/* Pointer to SPI Device */

/* Char Driver Globals */
static struct cdev dac7612Dev;
struct file_operations dac7612_Fops;
static int devno;

#define ERRGOTO(label, ...)                     \
  {                                             \
  printk (__VA_ARGS__);                         \
  goto label;                                   \
  } while(0)


/* Local Methods */
int dac7612_convert(u8 channel, s16* value)
{
  /* Start Conversion */
  u8 status;
  int err = dac7612_spi_write_reg8(DAC7612_GAINMUX,
                                   DAC7612_GAIN_1X |
                                   DAC7612_CH_SINGLE_ENDED |
                                   (channel & 0x07)| 
                                   DAC7612_CONVERT);

  if(err)
    return err;
  
  /* poll DAC7612 until conversion is ready */
  do
  {
    err = dac7612_spi_read_reg8(DAC7612_GAINMUX, &status);
  }
  while((!err) && ( (status & DAC7612_CONVERT) > 0));

  /* Read Result - Register wise its handled via u16, however we want it in s16. */
  err = dac7612_spi_read_reg16(DAC7612_RESULTLO, (u16*)value);
  if(*value & DAC7612_RESULTLO_OVR)
    printk(KERN_ALERT "DAC7612: Error! PGA Out of Range\n");
  *value = *value >> 4; // right-align result, lower 4-bits are zero

  //  if(MODULE_DEBUG)
  printk(KERN_DEBUG "DAC7612: Channel %i result: %i mV\n", channel, *value);

  return err;
}




static int __init dac7612_cdrv_init(void)
{
  int err; 
  
  printk("dac7612 driver initializing\n");

  err=dac7612_spi_init();
  if(err)
    ERRGOTO(error, "Failed SPI Initialization\n");

  
  /* Allocate chrdev region */
  devno = MKDEV(DAC7612_MAJOR, DAC7612_MINOR);
  err = register_chrdev_region(devno, NBR_ADC_CH, "dac7612");
  if(err)
    ERRGOTO(err_spi_init, "Failed registering char region (%d,%d) +%d, error %d\n",
            DAC7612_MAJOR, DAC7612_MINOR, NBR_ADC_CH, err);
  
  /* Register Char Device */
  cdev_init(&dac7612Dev, &dac7612_Fops);
  err = cdev_add(&dac7612Dev, devno, NBR_ADC_CH);
  if (err)
    ERRGOTO(err_register, "Error %d adding DAC7612 device\n", err);


  
  /* Configure DAC7612 according to data sheet */
  dac7612_spi_write_reg8(DAC7612_REFOSC,
                         DAC7612_REFOSC_OSCR |
                         DAC7612_REFOSC_OSCE |
                         DAC7612_REFOSC_REFE |
                         DAC7612_REFOSC_BUFE |
                         DAC7612_REFOSC_R2V);
  
  return 0;
  
  err_register:
  unregister_chrdev_region(devno, NBR_ADC_CH);

  err_spi_init:
  dac7612_spi_exit();
  
  
  error:
  return err;
}

static void __exit dac7612_cdrv_exit(void)
{
  printk("dac7612 driver Exit\n");
  cdev_del(&dac7612Dev);

  unregister_chrdev_region(devno, NBR_ADC_CH);

  dac7612_spi_exit();
}

int dac7612_cdrv_open(struct inode *inode, struct file *filep)
{
  int major = imajor(inode);
  int minor = iminor(inode);

  printk("Opening DAC7612 Device [major], [minor]: %i, %i\n", major, minor);

  if (minor > NBR_ADC_CH-1)
  {
    printk("Minor no out of range (0-%i): %i\n", NBR_ADC_CH, minor);
    return -ENODEV;
  }

  return 0;
}

int dac7612_cdrv_release(struct inode *inode, struct file *filep)
{
  int major = imajor(inode);
  int minor = iminor(inode);

  printk("Closing DAC7612 Device [major], [minor]: %i, %i\n", major, minor);

  if (minor > NBR_ADC_CH-1)
    return -ENODEV;
    
  return 0;
}

ssize_t dac7612_cdrv_write(struct file *filep, const char __user *ubuf,
                           size_t count, loff_t *f_pos)
{
  int minor, len, value;
  char kbuf[MAXLEN];    
    
  minor = MINOR(filep->f_dentry->d_inode->i_rdev);
  if (minor != DAC7612_MINOR) {
    printk(KERN_ALERT "dac7612 Write to wrong Minor No:%i \n", minor);
    return 0; }
  printk(KERN_ALERT "Writing to dac7612 [Minor] %i \n", minor);
    
  len = count < MAXLEN ? count : MAXLEN;
  if(copy_from_user(kbuf, ubuf, len))
    return -EFAULT;
	
  kbuf[len] = '\0';   // Pad null termination to string

  if(MODULE_DEBUG)
    printk("string from user: %s\n", kbuf);
  sscanf(kbuf,"%i", &value);
  if(MODULE_DEBUG)
    printk("value %i\n", value);

  /*
   * Write something here
   */

  return count;
}

ssize_t dac7612_cdrv_read(struct file *filep, char __user *ubuf,
                          size_t count, loff_t *f_pos)
{
  int minor;
  char resultBuf[MAXLEN];
  s16 result;
  int err;
    
  minor = MINOR(filep->f_dentry->d_inode->i_rdev);
  if (minor > NBR_ADC_CH-1) {
    printk(KERN_ALERT "dac7612 read from wrong Minor No:%i \n", minor);
    return 0; }
  if(MODULE_DEBUG)
    printk(KERN_ALERT "Reading from dac7612 [Minor] %i \n", minor);
    
  /* Start Conversion */
  err = dac7612_convert((minor & 0xff), &result);
  if(err)
    return -EFAULT;
  
  /* Convert to string and copy to user space */
  count = snprintf (resultBuf, sizeof resultBuf, "%d\n", result);
  count++;
  if(copy_to_user(ubuf, resultBuf, count))
    return -EFAULT;

  return count;
}



struct file_operations dac7612_Fops =
{
  .owner   = THIS_MODULE,
  .open    = dac7612_cdrv_open,
  .release = dac7612_cdrv_release,
  .write   = dac7612_cdrv_write,
  .read    = dac7612_cdrv_read,
};

module_init(dac7612_cdrv_init);
module_exit(dac7612_cdrv_exit);

MODULE_AUTHOR("Peter Hoegh Mikkelsen <phm@iha.dk>");
MODULE_LICENSE("GPL");

