#include <linux/err.h>
#include <linux/ctype.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/module.h>
#include "ads7870.h"
#include "ads7870-spi.h"

#define ADS7870_MAJOR       64
#define ADS7870_MINOR        0
#define MAXLEN              64
#define NBR_ADC_CH           8
#define COMPLIMENTARY_BIT   11

#define MODULE_DEBUG 0
#define USECDEV 0

/* Pointer to SPI Device */

/* Char Driver Globals */
static struct cdev ads7870Dev;
struct file_operations ads7870_Fops;
static int devno;

#define ERRGOTO(label, ...)                     \
  {                                             \
  printk (__VA_ARGS__);                         \
  goto label;                                   \
  } while(0)


/* Local Methods */
int ads7870_convert(u8 channel, s16* value)
{
  /* Start Conversion */
  u8 status;
  int err = ads7870_spi_write_reg8(ADS7870_GAINMUX, 
                                   ADS7870_GAIN_1X | 
                                   ADS7870_CH_SINGLE_ENDED |
                                   (channel & 0x07)| 
                                   ADS7870_CONVERT);

  if(err)
    return err;
  
  /* poll ADS7870 until conversion is ready */
  do
  {
    err = ads7870_spi_read_reg8(ADS7870_GAINMUX, &status);
  }
  while((!err) && ( (status & ADS7870_CONVERT) > 0));

  /* Read Result - Register wise its handled via u16, however we want it in s16. */
  err = ads7870_spi_read_reg16(ADS7870_RESULTLO, (u16*)value);
  if(*value & ADS7870_RESULTLO_OVR)
    printk(KERN_ALERT "ADS7870: Error! PGA Out of Range\n");
  *value = *value >> 4; // right-align result, lower 4-bits are zero

  //  if(MODULE_DEBUG)
  printk(KERN_DEBUG "ADS7870: Channel %i result: %i mV\n", channel, *value);

  return err;
}




static int __init ads7870_cdrv_init(void)
{
  int err; 
  
  printk("ads7870 driver initializing\n");  

  err=ads7870_spi_init();
  if(err)
    ERRGOTO(error, "Failed SPI Initialization\n");

  
  /* Allocate chrdev region */
  devno = MKDEV(ADS7870_MAJOR, ADS7870_MINOR);
  err = register_chrdev_region(devno, NBR_ADC_CH, "ads7870");  
  if(err)
    ERRGOTO(err_spi_init, "Failed registering char region (%d,%d) +%d, error %d\n",
            ADS7870_MAJOR, ADS7870_MINOR, NBR_ADC_CH, err);
  
  /* Register Char Device */
  cdev_init(&ads7870Dev, &ads7870_Fops);
  err = cdev_add(&ads7870Dev, devno, NBR_ADC_CH);
  if (err)
    ERRGOTO(err_register, "Error %d adding ADS7870 device\n", err);


  
  /* Configure ADS7870 according to data sheet */
  ads7870_spi_write_reg8(ADS7870_REFOSC, 
                         ADS7870_REFOSC_OSCR |
                         ADS7870_REFOSC_OSCE |
                         ADS7870_REFOSC_REFE |
                         ADS7870_REFOSC_BUFE |
                         ADS7870_REFOSC_R2V);
  
  return 0;
  
  err_register:
  unregister_chrdev_region(devno, NBR_ADC_CH);

  err_spi_init:
  ads7870_spi_exit();
  
  
  error:
  return err;
}

static void __exit ads7870_cdrv_exit(void)
{
  printk("ads7870 driver Exit\n");
  cdev_del(&ads7870Dev);

  unregister_chrdev_region(devno, NBR_ADC_CH);

  ads7870_spi_exit();
}

int ads7870_cdrv_open(struct inode *inode, struct file *filep)
{
  int major = imajor(inode);
  int minor = iminor(inode);

  printk("Opening ADS7870 Device [major], [minor]: %i, %i\n", major, minor);

  if (minor > NBR_ADC_CH-1)
  {
    printk("Minor no out of range (0-%i): %i\n", NBR_ADC_CH, minor);
    return -ENODEV;
  }

  return 0;
}

int ads7870_cdrv_release(struct inode *inode, struct file *filep)
{
  int major = imajor(inode);
  int minor = iminor(inode);

  printk("Closing ADS7870 Device [major], [minor]: %i, %i\n", major, minor);

  if (minor > NBR_ADC_CH-1)
    return -ENODEV;
    
  return 0;
}

ssize_t ads7870_cdrv_write(struct file *filep, const char __user *ubuf, 
                           size_t count, loff_t *f_pos)
{
  int minor, len, value;
  char kbuf[MAXLEN];    
    
  minor = MINOR(filep->f_dentry->d_inode->i_rdev);
  if (minor != ADS7870_MINOR) {
    printk(KERN_ALERT "ads7870 Write to wrong Minor No:%i \n", minor);
    return 0; }
  printk(KERN_ALERT "Writing to ads7870 [Minor] %i \n", minor);
    
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

ssize_t ads7870_cdrv_read(struct file *filep, char __user *ubuf, 
                          size_t count, loff_t *f_pos)
{
  int minor;
  char resultBuf[MAXLEN];
  s16 result;
  int err;
    
  minor = MINOR(filep->f_dentry->d_inode->i_rdev);
  if (minor > NBR_ADC_CH-1) {
    printk(KERN_ALERT "ads7870 read from wrong Minor No:%i \n", minor);
    return 0; }
  if(MODULE_DEBUG)
    printk(KERN_ALERT "Reading from ads7870 [Minor] %i \n", minor);
    
  /* Start Conversion */
  err = ads7870_convert((minor & 0xff), &result);
  if(err)
    return -EFAULT;
  
  /* Convert to string and copy to user space */
  count = snprintf (resultBuf, sizeof resultBuf, "%d\n", result);
  count++;
  if(copy_to_user(ubuf, resultBuf, count))
    return -EFAULT;

  return count;
}



struct file_operations ads7870_Fops = 
{
  .owner   = THIS_MODULE,
  .open    = ads7870_cdrv_open,
  .release = ads7870_cdrv_release,
  .write   = ads7870_cdrv_write,
  .read    = ads7870_cdrv_read,
};

module_init(ads7870_cdrv_init);
module_exit(ads7870_cdrv_exit);

MODULE_AUTHOR("Peter Hoegh Mikkelsen <phm@iha.dk>");
MODULE_LICENSE("GPL");

