#include "scull_main.h"

MODULE_VERSION("1.0");
MODULE_AUTHOR("Pushpendra Kumar");
MODULE_DESCRIPTION("LDD3 scull module");
MODULE_LICENSE("GPL");

const char *scull_name = "scull";
static const unsigned scull_minor = 0; // Scull minor range start
static unsigned scull_major = 0;
static const unsigned count_minor  = 1;

dev_t scull_dev_id = 0;
struct cdev scull_cdev;


/*<---			Function declarations			--->*/
// Scull operations
loff_t scull_llseek(struct file *, loff_t, int);
ssize_t scull_read(struct file *, char __user *, size_t, loff_t *);
ssize_t scull_write(struct file *, const char __user *, size_t, loff_t *);
long scull_unlocked_ioctl(struct file *, unsigned int, unsigned long);
int scull_open(struct inode *, struct file *);
int scull_release(struct inode *, struct file *);


// Scull device operations structure
struct file_operations scull_fops = {
	.owner = THIS_MODULE,
	.llseek = scull_llseek,
	.read = scull_read,
	.write = scull_write,
	.unlocked_ioctl = scull_unlocked_ioctl,
	.open = scull_open,
	.release = scull_release
};

// Scull device structure
typedef struct scull_dev_t
{
	struct scull_qset *data; /* Pointer to first quantum set */
	int quantum; /* the current quantum size */
	int qset; /* the current array size */
	unsigned long size; /* amount of data stored here */
	unsigned int access_key; /* used by sculluid and scullpriv */
	struct semaphore sem; /* mutual exclusion semaphore */
	struct cdev cdev; /* Char device structure */
} scull_dev;

static int __init scull_init(void)
{
	int retVal = 0;

	printk(KERN_INFO "Initializing scull...\n");

	if(scull_major)
	{
		scull_dev_id = MKDEV(scull_major, scull_minor);
		retVal = register_chrdev_region(scull_dev_id, count_minor, scull_name);
	}
	else
	{
		retVal = alloc_chrdev_region(&scull_dev_id, scull_minor, count_minor,
				scull_name);
	}

	if(retVal < 0)
	{
		printk("char dev region alloc/reg failed major=%d retVal=%d",
				scull_major, retVal);
		return retVal;
	}

	printk(KERN_INFO "scull major=%u minor=%u\n", MAJOR(scull_dev_id),
			MINOR(scull_dev_id));

	// Initialize the device structure
	cdev_init(&scull_cdev, &scull_fops);

	return retVal;
}

static void __exit scull_exit(void)
{
	printk(KERN_INFO "Exiting scull...\n");
	unregister_chrdev_region(scull_dev_id, count_minor);
}


loff_t scull_llseek(struct file *filp, loff_t offset, int pos)
{
	return offset;
}

ssize_t scull_read(struct file *filp, char __user *read_buff, size_t size, loff_t *offset)
{
	ssize_t read_bytes = 0;
	return read_bytes;
}

ssize_t scull_write(struct file *filp, const char __user *write_buff, size_t size, loff_t *offset)
{
	ssize_t write_bytes = 0;
	return write_bytes;
}

long scull_unlocked_ioctl(struct file *filp, unsigned int something, unsigned long sometihng1)
{
	long no_idea = 0;
	return no_idea;
}

int scull_open(struct inode *f_inode, struct file *filp)
{
	int result = 0;
	return result;
}

int scull_release(struct inode *f_inode, struct file *filp)
{
	int result = 0;
	return result;
}

module_init(scull_init);
module_exit(scull_exit);
