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
    void **data; // pointer to array of quantums
    struct scull_qset *next;
};

struct scull_dev
{
	struct scull_qset *qdata; /* Pointer to first quantum set */
	int curr_quantums; /* the curr quantum size */
	int curr_qsets; /* the curr array size */
	unsigned long size; /* amount of data stored here */
	unsigned int access_key; /* used by sculluid and scullpriv */
	struct semaphore sem; /* mutual exclusion semaphore */
	struct cdev cdev; /* Char device structure */
};

struct scull_dev *p_scull_dev = NULL;

// Scull private funcitons
int scull_trim(struct scull_dev *p_dev);
struct scull_qset* scull_follow(struct scull_dev *p_dev, int item);

// Function definitions
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
    struct scull_dev *p_dev = filp->private_data; 
    struct scull_qset *p_qset;    /* the first listitem */
    int curr_quantums = p_dev->curr_quantums;
    int curr_qsets = p_dev->curr_qsets;
    int itemsize = curr_quantums * curr_qsets; /* how many bytes in the listitem */
    int item, s_pos, q_pos, rest;
    ssize_t retval = 0;

    if (down_interruptible(&p_dev->sem))
	return -ERESTARTSYS;
    if (*offset >= p_dev->size)
	goto out;
    if (*offset + size > p_dev->size)
	size = p_dev->size - *offset;

    /* find listitem, curr_qsets index, and offset in the curr_quantums */
    item = (long)*offset / itemsize;
    rest = (long)*offset % itemsize;
    s_pos = rest / curr_quantums; q_pos = rest % curr_quantums;

    /* follow the list up to the right position (defined elsewhere) */
    p_qset = scull_follow(p_dev, item);

    if (p_qset == NULL || !p_qset->data || ! p_qset->data[s_pos])
	goto out; /* don't fill holes */

    /* read only up to the end of this curr_quantums */
    if (size > curr_quantums - q_pos)
	size = curr_quantums - q_pos;

    if (copy_to_user(read_buff, (p_qset->data[s_pos] + q_pos), size))
    {
	retval = -EFAULT;
	goto out;
    }

    *offset += size;
    retval = size;

out:
    up(&p_dev->sem);
    return retval;
}

ssize_t scull_write(struct file *filp, const char __user *write_buff, size_t size, loff_t *offset)
{
    struct scull_dev *p_dev = filp->private_data;
    struct scull_qset *p_qset;
    int curr_quantums = p_dev->curr_quantums, curr_qsets = p_dev->curr_qsets;
    int itemsize = curr_quantums * curr_qsets;
    int item, s_pos, q_pos, rest;
    ssize_t retval = -ENOMEM; /* value used in "goto out" statements */

    if (down_interruptible(&p_dev->sem))
	return -ERESTARTSYS;

    /* find listitem, curr_qsets index and offset in the curr_quantums */
    item = (long)*offset / itemsize;
    rest = (long)*offset % itemsize;
    s_pos = rest / curr_quantums; q_pos = rest % curr_quantums;

    /* follow the list up to the right position */
    p_qset = scull_follow(p_dev, item);
    if (p_qset == NULL)
	goto out;
    if (!p_qset->data) {
	p_qset->data = kmalloc(curr_qsets * sizeof(char *), GFP_KERNEL);
	if (!p_qset->data)
	    goto out;
	memset(p_qset->data, 0, curr_qsets * sizeof(char *));
    }
    if (!p_qset->data[s_pos]) {
	p_qset->data[s_pos] = kmalloc(curr_quantums, GFP_KERNEL);
	if (!p_qset->data[s_pos])
	    goto out;
    }
    /* write only up to the end of this curr_quantums */
    if (size > curr_quantums - q_pos)
	size = curr_quantums - q_pos;

    if (copy_from_user(p_qset->data[s_pos]+q_pos, write_buff, size)) {
	retval = -EFAULT;
	goto out;
    }
    *offset += size;
    retval = size;

    /* update the size */
    if (p_dev->size < *offset)
	p_dev->size = *offset;

out:
    up(&p_dev->sem);
    return retval;
}

long scull_unlocked_ioctl(struct file *filp, unsigned int something, unsigned long sometihng1)
{
	long no_idea = 0;
	return no_idea;
}

int scull_open(struct inode *f_inode, struct file *filp)
{
    struct scull_dev *p_dev;
    p_dev = container_of(f_inode->i_cdev, struct scull_dev, cdev);
    filp->private_data = p_dev; // For other methods use

    // Trimming to zero in case of write only operation
    if ( (filp->f_flags & O_ACCMODE) == O_WRONLY) {
        scull_trim(p_dev); /* ignore errors */
    }

    return 0;
}

int scull_release(struct inode *f_inode, struct file *filp)
{
	int result = 0;
	return result;
}

// Release all the data in scull
int scull_trim(struct scull_dev *p_dev)
{
    struct scull_qset *p_next_qset;
    struct scull_qset *p_qset;
    int curr_qsets = p_dev->curr_qsets;
    int iter_qset;

    p_next_qset = p_qset = p_dev->qdata;

    while((p_qset = p_next_qset))
    {
        if(p_qset->data)
        {
            for (iter_qset = 0; iter_qset < curr_qsets; iter_qset++)
            {
                kfree(p_qset->data[iter_qset]);
            }
            kfree(p_qset->data);
            p_qset->data = NULL;
        }

        p_next_qset = p_qset->next;
        kfree(p_qset);
    }

    p_dev->size = 0;
    p_dev->curr_quantums = 0;
    p_dev->curr_qsets = 0;
    p_dev->qdata = NULL;
    return 0;
}

struct scull_qset* scull_follow(struct scull_dev *p_dev, int item)
{
    struct scull_qset *qset = NULL;
    return qset;
}

module_init(scull_init);
module_exit(scull_exit);
