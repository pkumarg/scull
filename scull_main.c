#include "scull_main.h"

MODULE_VERSION("1.0");
MODULE_AUTHOR("Pushpendra Kumar");
MODULE_DESCRIPTION("LDD3 scull module");
MODULE_LICENSE("GPL");

static int __init scull_init(void)
{
	printk("Initializing scull...\n");
	return 0;
}

static void __exit scull_exit(void)
{
	printk("Exiting scull...\n");
}

module_init(scull_init);
module_exit(scull_exit);
