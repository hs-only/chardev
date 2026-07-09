#include<linux/atomic.h>
#include<linux/cdev.h>
#include<linux/delay.h>
#include<linux/device.h>
#include<linux/fs.h>
#include<linux/init.h>
#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/printk.h>
#include<linux/types.h>
#include<linux/uaccess.h>
#include<linux/version.h>

#include<asm/errno.h>

static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char __user *, size_t, loff_t *);

#define DEVICE_NAME "chardev" /* Dev name as it appears in /proc/devices*/
#define BUF_LEN /* Mx length of the message from the device */

static int major; /* major number assigned to the driver*/

enum {
    CDEV_NOT_USED,
    CDEV_EXCLUSIVE_OPEN,
};

static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED);

static char msg[BUF_LEN+1];

static struct class *cls;

static struct file_operations chardev_fops={
    .read=device_read,
    .write=device_write,
    .open=device_open,
    .release=device_release,
};

static int __init chardev_init(void){
    major=register_chrdev(0, DEVICE_NAME, &chardev_fops);

    if(major<0){
        pr_alert("Registering char device failed with %d\n", major);
        return major;
    }
    pr_info("I was assigned major number %d.\n", major);
    
#if LINUX_VERSION_CODE>=KERNEL_VERSION(6, 4, 0)
    cls = class_create(DEVICE_NAME);
#else
    cls=class_create(THIS_MODULE, DEVICE_NAME);
#endif
    if(IS_ERR(cls)){
        pr_err("Failed to create class for device\n");
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(cls);
    }
    device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    
    pr_info("Device created on /dev/%s\n", DEVICE_NAME);
    return 0;
}

static void __exit chardev_exit(void){
    device_destroy(cls, MKDEV(major, 0));
    class_destroy(cls);

    /*unregister */
    unregister_chrdev(major, DEVICE_NAME);
}

/* called when a process try to open device file*/

static int device_open(struct inode *inode, struct file *file){
    static int counter = 0;
    if(atomic_cmpxchg(&already_open, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN))
        return -EBUSY;

    sprintf(msg, "I already told you %d times Hello World!\n", counter++);
    return 0;
}

/* called when a process closes the device file*/
static int device_release(struct inode *inode, struct file *file){
    /* now ready for next caller*/
    atomic_set(&already_open, CDEV_NOT_USED);

    return 0;
}

/*called when a process, whcih already opened the file, attempts to read from it*/
static ssize_t device_read(struct file *filep, char __user *buffer, size_t length, loff_t *offset){
    int bytes_read=0;
    const char *msg_ptr=msg;
    
    if(!*(msg_ptr+*offset)){ /*end of message*/
        *offset=0; /*reset the offset*/
        return 0; /*signify end of file*/
    }

    msg_ptr+=*offset;

    /*Put the data in buffer*/
    while(length && *msg_ptr){
        put_user(*(msg_ptr++), buffer++);
        length--;
        bytes_read++;
    }

    *offset+=bytes_read;
    return bytes_read;
}

static ssize_t device_write(struct file *flip, const char __user *buff, size_t len, loff_t *off){
    pr_alert("Soory, this operation is not supported.\n");
    return -EINVAL;
}

module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");