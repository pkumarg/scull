#include "scull_main.h"

MODULE_VERSION("1.0");
MODULE_AUTHOR("Pushpendra Kumar");
MODULE_DESCRIPTION("LDD3 scull module");
MODULE_LICENSE("GPL");

// Constants
#define SCULL_QUANTUM_SIZE        1024
#define SCULL_QSET_SIZE           1000

const char *scull_name = "scull";
static const unsigned scull_minor = 0; // Scull minor range start
static unsigned scull_major = 0;
static const unsigned count_minor  = 1;

dev_t scull_dev_id = 0;


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
struct scull_qset;
struct scull_qset
{
    void **data;
    struct scull_qset *next;
};

struct scull_dev
{
	struct scull_qset *data; /* Pointer to first quantum set */
	int quantum; /* the current quantum size */
	int qset; /* the current array size */
	unsigned long size; /* amount of data stored here */
	unsigned int access_key; /* used by sculluid and scullpriv */
	struct semaphore sem; /* mutual exclusion semaphore */
	struct cdev cdev; /* Char device structure */
};

struct scull_dev *p_scull_dev = NULL;

// Scull private funcitons
int scull_trim(struct scull_dev *dev);
struct scull_qset* scull_follow(struct scull_dev *dev, int item);

static int __init scull_init(void)
{
	int ret_val = 0;

	printk(KERN_INFO "Initializing scull...\n");

	if(scull_major)
	{
		scull_dev_id = MKDEV(scull_major, scull_minor);
		ret_val = register_chrdev_region(scull_dev_id, count_minor, scull_name);
	}
	else
	{
		ret_val = alloc_chrdev_region(&scull_dev_id, scull_minor, count_minor,
				scull_name);
	}

	if(ret_val < 0)
	{
		printk("char dev region alloc/reg failed major=%d ret_val=%d",
				scull_major, ret_val);
		return ret_val;
	}

	printk(KERN_INFO "scull major=%u minor=%u\n", MAJOR(scull_dev_id),
			MINOR(scull_dev_id));

	// Allocate scull_dev
	p_scull_dev = (struct scull_dev *) kmalloc(sizeof(struct scull_dev), (GFP_KERNEL | __GFP_REPEAT));

	if(!p_scull_dev)
	{
		printk("Failed to allocate scull_dev\n.");
		return -ENOMEM;
	}

	// Initialize cdev struct
	cdev_init(&p_scull_dev->cdev, &scull_fops);
	p_scull_dev->cdev.owner = THIS_MODULE;
	p_scull_dev->cdev.ops = &scull_fops;

	// Lets add our device in Kernel subsystem to make it live
	// They say about last argument is that it's 1 in almost all
	// cases and my case not seems specific so taking as 1
	ret_val = cdev_add(&p_scull_dev->cdev, scull_dev_id, 1);

	if (ret_val < 0)
		printk(KERN_ERR "cdev_add() failed %d", ret_val);
	return ret_val;
}

static void __exit scull_exit(void)
{
	printk(KERN_INFO "Exiting scull...\n");

	if(p_scull_dev)
		cdev_del(&p_scull_dev->cdev);

	kfree(p_scull_dev);

	unregister_chrdev_region(scull_dev_id, count_minor);
}


loff_t scull_llseek(struct file *filp, loff_t offset, int pos)
{
	return offset;
}

ssize_t scull_read(struct file *filp, char __user *read_buff, size_t size, loff_t *offset)
{
    struct scull_dev *dev = filp->private_data; 
    struct scull_qset *dptr;    /* the first listitem */
    int quantum = dev->quantum, qset = dev->qset;
    int itemsize = quantum * qset; /* how many bytes in the listitem */
    int item, s_pos, q_pos, rest;
    ssize_t retval = 0;

    if (down_interruptible(&dev->sem))
	return -ERESTARTSYS;
    if (*offset >= dev->size)
	goto out;
    if (*offset + size > dev->size)
	size = dev->size - *offset;

    /* find listitem, qset index, and offset in the quantum */
    item = (long)*offset / itemsize;
    rest = (long)*offset % itemsize;
    s_pos = rest / quantum; q_pos = rest % quantum;

    /* follow the list up to the right position (defined elsewhere) */
    dptr = scull_follow(dev, item);

    if (dptr == NULL || !dptr->data || ! dptr->data[s_pos])
	goto out; /* don't fill holes */

    /* read only up to the end of this quantum */
    if (size > quantum - q_pos)
	size = quantum - q_pos;

    if (copy_to_user(read_buff, (dptr->data[s_pos] + q_pos), size))
    {
	retval = -EFAULT;
	goto out;
    }

    *offset += size;
    retval = size;

out:
    up(&dev->sem);
    return retval;
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
    struct scull_dev *dev;
    dev = container_of(f_inode->i_cdev, struct scull_dev, cdev);
    filp->private_data = dev; // For other methods use

    // Trimming to zero in case of write only operation
    if ( (filp->f_flags & O_ACCMODE) == O_WRONLY) {
        scull_trim(dev); /* ignore errors */
    }

    return 0;
}

int scull_release(struct inode *f_inode, struct file *filp)
{
	int result = 0;
	return result;
}

int scull_trim(struct scull_dev *dev)
{
    struct scull_qset *next, *dptr;
    int qset = dev->qset;
    /* "dev" is not-null */
    int i;
    for (dptr = dev->data; dptr; dptr = next) { /* all the list items */
        if (dptr->data) {
            for (i = 0; i < qset; i++)
                kfree(dptr->data[i]);
            kfree(dptr->data);
            dptr->data = NULL;
        }
        next = dptr->next;
        kfree(dptr);
    }
    dev->size = 0;
    dev->quantum = SCULL_QUANTUM_SIZE;
    dev->qset = SCULL_QSET_SIZE;
    dev->data = NULL;
    return 0;
}

struct scull_qset* scull_follow(struct scull_dev *dev, int item)
{
    struct scull_qset *qset = NULL;
    return qset;
}

module_init(scull_init);
module_exit(scull_exit);
