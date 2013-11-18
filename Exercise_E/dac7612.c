#include <linux/err.h>
#include <linux/ctype.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/module.h>
#include "dac7612.h"
#include "dac7612-spi.h"

#define DAC7612_MAJOR 60
#define DAC7612_MINOR 0
#define MAXLEN 64
#define COMPLIMENTARY_BIT 11
#define DAC7612_AMOUNT 2
#define MODULE_DEBUG 1
#define USECDEV 0
#define DAC7612_CHANNEL_A 2
#define DAC7612_CHANNEL_B 3

MODULE_AUTHOR("TeamMPS");
MODULE_LICENSE("Dual BSD/GPL");

static struct cdev dac7612Dev;
struct file_operations dac7612_Fops;
static int devno;

#define ERRGOTO(label, ...)                     \
  {                                             \
  printk (__VA_ARGS__);                         \
  goto label;                                   \
  } while(0)

static int __init dac7612_cdrv_init(void)
{
  int err;	// Error variable
  
  printk("dac7612 driver initializing\n");  

  err=dac7612_spi_init();
  if(err)
    ERRGOTO(error, "Failed SPI Initialization\n");

  
  /* Allocate chrdev region */

  devno = MKDEV(DAC7612_MAJOR, DAC7612_MINOR);

  err = register_chrdev_region(devno, DAC7612_AMOUNT, "dac7612"); 
  if(err)
    ERRGOTO(err_spi_init, "Failed registering char region (%d,%d) +%d, error %d\n",
            DAC7612_MAJOR, DAC7612_MINOR, DAC7612_AMOUNT, err);
  
  /* Register Char Device */
  cdev_init(&dac7612Dev, &dac7612_Fops);
  err = cdev_add(&dac7612Dev, devno, DAC7612_AMOUNT);
  if (err)
    ERRGOTO(err_register, "Error %d adding DAC7612 device\n", err);

  return 0;
  
  err_register:
  unregister_chrdev_region(devno, DAC7612_AMOUNT);

  err_spi_init:
  dac7612_spi_exit();
  
  
  error:
  return err;
}

static void __exit dac7612_cdrv_exit(void)
{
  printk("dac7612 driver Exit\n");
  cdev_del(&dac7612Dev);

  unregister_chrdev_region(devno, DAC7612_AMOUNT);

  dac7612_spi_exit();
}

int dac7612_cdrv_open(struct inode *inode, struct file *filep)
{
  int major = imajor(inode);
  int minor = iminor(inode);

  printk("Opening DAC7612 Device [major], [minor]: %i, %i\n", major, minor);

  if (minor > DAC7612_AMOUNT-1)
  {
    printk("Minor no out of range (0-%i): %i\n", DAC7612_AMOUNT, minor);
    return -ENODEV;
  }

  return 0;
}

int dac7612_cdrv_release(struct inode *inode, struct file *filep)
{
  int major = imajor(inode);
  int minor = iminor(inode);

  printk("Closing DAC7612 Device [major], [minor]: %i, %i\n", major, minor);

  if (minor > DAC7612_AMOUNT-1)
    return -ENODEV;
    
  return 0;
}

ssize_t dac7612_cdrv_write(struct file *filep, const char __user *ubuf, 
                           size_t count, loff_t *f_pos)
{
  int minor, len, value, addr;
  char kbuf[MAXLEN];    
    
  minor = MINOR(filep->f_dentry->d_inode->i_rdev);
 
  len = count < MAXLEN ? count : MAXLEN;

  if(copy_from_user(kbuf, ubuf, len))
    return -EFAULT;
	
  kbuf[len] = '\0';   // Pad null termination to string

  if(MODULE_DEBUG)
    printk("string from user: %s\n", kbuf);

  sscanf(kbuf,"%i", &value);

  printk(KERN_ALERT "Writing %i to dac7612 [Minor] %i \n", value, minor);

  switch (minor)
  {
    case 0:
      addr = DAC7612_CHANNEL_A;
      break;
    case 1:
      addr = DAC7612_CHANNEL_B;
      break;
    default:
      printk("Unknown minor number - Address unknown %i\n", minor);
      return -ENODEV;
  }
  
  // Calling the spi write function with address and value:
  dac7612_spi_write_reg14(addr, value);
  
  return count;
}

struct file_operations dac7612_Fops = 
{
  .owner   = THIS_MODULE,
  .open    = dac7612_cdrv_open,
  .release = dac7612_cdrv_release,
  .write   = dac7612_cdrv_write,
};

module_init(dac7612_cdrv_init);
module_exit(dac7612_cdrv_exit);
