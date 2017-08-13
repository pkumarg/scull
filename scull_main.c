#include "scull_main.h"

MODULE_VERSION("1.0");
MODULE_AUTHOR("Pushpendra Kumar");
MODULE_DESCRIPTION("LDD3 scull module");
MODULE_LICENSE("GPL");

const char *scull_name = "scull";
static const unsigned scull_minor = 0; // Scull minor range start
static unsigned scull_major = 0;
static const unsigned count_minor  = 1;

dev_t scull_dev = 0;

static int __init scull_init(void)
{
	int retVal = 0;

	printk(KERN_INFO "Initializing scull...\n");

	if(scull_major)
	{
		scull_dev = MKDEV(scull_major, scull_minor);
		retVal = register_chrdev_region(scull_dev, count_minor, scull_name);
	}
	else
	{
		retVal = alloc_chrdev_region(&scull_dev, scull_minor, count_minor,
				scull_name);
	}

	if(retVal < 0)
	{
		printk("char dev region alloc/reg failed major=%d retVal=%d",
				scull_major, retVal);
	}

	printk(KERN_INFO "scull major=%u minor=%u\n", MAJOR(scull_dev),
			MINOR(scull_dev));

	return retVal;
}

static void __exit scull_exit(void)
{
	printk(KERN_INFO "Exiting scull...\n");
	unregister_chrdev_region(scull_dev, count_minor);
}

module_init(scull_init);
module_exit(scull_exit);
