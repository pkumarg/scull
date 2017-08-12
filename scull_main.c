#include "scull_main.h"

MODULE_VERSION("1.0");
MODULE_AUTHOR("Pushpendra Kumar");
MODULE_DESCRIPTION("LDD3 scull module");
MODULE_LICENSE("GPL");

const char *scull_name = "scull";
const unsigned base_minor = 0;
const unsigned count_minor  = 1;

dev_t scull_dev = 0;

static int __init scull_init(void)
{
	int retVal = 0;
	printk(KERN_INFO "Initializing scull...\n");
	if(0 != (retVal = alloc_chrdev_region(&scull_dev, base_minor,
					count_minor, scull_name)))
	{
		printk(KERN_ERR "alloc_chrdev_region() failed: %d\n", retVal);
	}

	printk(KERN_INFO "scull major=%u minor=%u\n", MAJOR(scull_dev), MINOR(scull_dev));

	return 0;
}

static void __exit scull_exit(void)
{
	printk(KERN_INFO "Exiting scull...\n");
	unregister_chrdev_region(scull_dev, count_minor);
}

module_init(scull_init);
module_exit(scull_exit);
