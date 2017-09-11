#include "scull_main.h"

MODULE_VERSION("1.0");
MODULE_AUTHOR("Pushpendra Kumar");
MODULE_DESCRIPTION("LDD3 scull module");
MODULE_LICENSE("GPL");

// Module params definitions
static int curr_quantums   = SCULL_QUANTUM_SIZE;
static int curr_qsets      = SCULL_QSET_SIZE;

// Module parameters
module_param(curr_quantums, int, S_IRUGO);
module_param(curr_qsets, int, S_IRUGO);

// Constants
const char *scull_name = "scull";
const char *proc_file_name = "scullmem";
static const unsigned scull_minor = 0; // Scull minor range start
static unsigned scull_major = 0;
static const unsigned count_minor  = 1;
const int scull_nr_devs = 1; // Number of scull devices

dev_t scull_dev_id = 0;


/*<---			Function declarations			--->*/
// Scull operations
loff_t scull_llseek(struct file *, loff_t, int);
ssize_t scull_read(struct file *, char __user *, size_t, loff_t *);
ssize_t scull_write(struct file *, const char __user *, size_t, loff_t *);
long scull_unlocked_ioctl(struct file *, unsigned int, unsigned long);
int scull_open(struct inode *, struct file *);
int scull_release(struct inode *, struct file *);

// Scull sequence operations
int scull_proc_open(struct inode *inode, struct file *file);
void *scull_seq_start(struct seq_file *sfile, loff_t *pos);
void *scull_seq_next(struct seq_file *sfile, void *v, loff_t *pos);
void scull_seq_stop(struct seq_file *sfile, void *v);
int scull_seq_show(struct seq_file *sfile, void *v);

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
    int curr_quantums; /* the curr min allocatable quantum */
    int curr_qsets; /* the curr min allocatable qset size */
    unsigned long size; /* amount of data stored here */
    unsigned int access_key; /* used by sculluid and scullpriv */
    struct mutex lock; /* mutual exclusion mutex */
    struct cdev cdev; /* Char device structure */
};

// SCULL sequence operations
static struct seq_operations scull_seq_ops = {
    .start = scull_seq_start,
    .next = scull_seq_next,
    .stop = scull_seq_stop,
    .show = scull_seq_show
};

static struct file_operations scull_proc_ops = {
    .owner = THIS_MODULE,
    .open = scull_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = seq_release
};


struct scull_dev *p_scull_dev = NULL;
struct proc_dir_entry *proc_dir_p = NULL;

// Scull private funcitons
int scull_trim(struct scull_dev *p_dev);
struct scull_qset* scull_follow(struct scull_dev *p_dev, int item);

// Function definitions
static int __init scull_init(void)
{
    int ret_val = 0;

    DBG_FUNC_ENTER();

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
        DBG_FUNC_EXIT();
        return ret_val;
    }

    printk(KERN_INFO "scull major=%u minor=%u\n", MAJOR(scull_dev_id),
            MINOR(scull_dev_id));

    // Allocate scull_dev
    p_scull_dev = (struct scull_dev *) kmalloc(sizeof(struct scull_dev), (GFP_KERNEL | __GFP_REPEAT));

    if(!p_scull_dev)
    {
        printk("Failed to allocate scull_dev\n.");
        DBG_FUNC_EXIT();
        return -ENOMEM;
    }

    memset(p_scull_dev, 0, sizeof(struct scull_dev));

    // Initialize cdev struct
    cdev_init(&p_scull_dev->cdev, &scull_fops);
    p_scull_dev->cdev.owner = THIS_MODULE;
    p_scull_dev->cdev.ops = &scull_fops;

    // Initilize the mutex
    mutex_init(&p_scull_dev->lock);

    // Lets add our device in Kernel subsystem to make it live
    // They say about last argument is that it's 1 in almost all
    // cases and my case not seems specific so taking as 1
    ret_val = cdev_add(&p_scull_dev->cdev, scull_dev_id, 1);

    if (ret_val < 0)
    {
        printk(KERN_ERR "cdev_add() failed %d", ret_val);
        return ret_val;
    }

    // Creating /proc/scullmem for debugging
    proc_dir_p = proc_create(proc_file_name, 0, NULL, &scull_proc_ops);

    DBG_FUNC_EXIT();
    return ret_val;
}

static void __exit scull_exit(void)
{
    DBG_FUNC_ENTER();

    printk(KERN_INFO "scull exiting...\n");

    if(p_scull_dev)
        cdev_del(&p_scull_dev->cdev);

    mutex_destroy(&p_scull_dev->lock);

    kfree(p_scull_dev);

    proc_remove(proc_dir_p);

    unregister_chrdev_region(scull_dev_id, count_minor);

    DBG_FUNC_EXIT();
}


loff_t scull_llseek(struct file *filp, loff_t off, int whence)
{
    struct scull_dev *dev = filp->private_data;
    loff_t newpos;
    switch(whence) {
        case 0: /* SEEK_SET */
            newpos = off;
            break;
        case 1: /* SEEK_CUR */
            newpos = filp->f_pos + off;
            break;
        case 2: /* SEEK_END */
            newpos = dev->size + off;
            break;
        default: /* can't happen */
            return -EINVAL;
    }

    if (newpos < 0)
        return -EINVAL;

    filp->f_pos = newpos;

    return newpos;
}

ssize_t scull_read(struct file *filp, char __user *read_buff, size_t size, loff_t *offset)
{
    struct scull_dev *p_dev;
    struct scull_qset *p_qset;    /* the first listitem */
    int curr_quantums;
    int curr_qsets;
    int itemsize;
    int item, s_pos, q_pos, rest;
    ssize_t retval = 0;

    DBG_FUNC_ENTER();

    p_dev = filp->private_data; 

    curr_quantums = p_dev->curr_quantums;
    curr_qsets = p_dev->curr_qsets;
    itemsize = curr_quantums * curr_qsets; /* how many bytes in the listitem */


    if (mutex_lock_interruptible(&p_dev->lock))
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
    mutex_unlock(&p_dev->lock);
    return retval;
}

ssize_t scull_write(struct file *filp, const char __user *write_buff, size_t size, loff_t *offset)
{
    struct scull_dev *p_dev;
    struct scull_qset *p_qset;
    int curr_quantums;
    int curr_qsets;
    int itemsize;
    int item, s_pos, q_pos, rest;
    ssize_t retval = -ENOMEM;

    DBG_FUNC_ENTER();

    p_dev = filp->private_data;

    curr_quantums = p_dev->curr_quantums;
    curr_qsets = p_dev->curr_qsets;
    itemsize = curr_quantums * curr_qsets;

    if (mutex_lock_interruptible(&p_dev->lock))
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
    mutex_unlock(&p_dev->lock);

    DBG_FUNC_EXIT();
    return retval;
}

long scull_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int err = 0, tmp;
	int retval = 0;
	/*
	 * extract the type and number bitfields, and don't decode
	 * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok( )
	 */
	if (_IOC_TYPE(cmd) != SCULL_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > SCULL_IOC_MAXNR) return -ENOTTY;
	/*
	 * the direction is a bitmask, and VERIFY_WRITE catches R/W
	 * transfers. `Type' is user-oriented, while
	 * access_ok is kernel-oriented, so the concept of "read" and
	 * "write" is reversed
	 */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if (err) return -EFAULT;
	switch(cmd) {
		case SCULL_IOCRESET:
			curr_quantums = SCULL_QUANTUM_SIZE;
			curr_qsets = SCULL_QSET_SIZE;
			break;
		case SCULL_IOCSQUANTUM: /* Set: arg points to the value */
			if (! capable (CAP_SYS_ADMIN))
				return -EPERM;
			retval = __get_user(curr_quantums, (int __user *)arg);
			break;
		case SCULL_IOCTQUANTUM: /* Tell: arg is the value */
			if (! capable (CAP_SYS_ADMIN))
				return -EPERM;
			curr_quantums = arg;
			break;
		case SCULL_IOCGQUANTUM: /* Get: arg is pointer to result */
			retval = __put_user(curr_quantums, (int __user *)arg);
			break;
		case SCULL_IOCQQUANTUM: /* Query: return it (it's positive) */
			return curr_quantums;
		case SCULL_IOCXQUANTUM: /* eXchange: use arg as pointer */
			if (! capable (CAP_SYS_ADMIN))
				return -EPERM;
			tmp = curr_quantums;
			retval = __get_user(curr_quantums, (int __user *)arg);
			if (retval == 0)
				retval = __put_user(tmp, (int __user *)arg);
			break;
		case SCULL_IOCHQUANTUM: /* sHift: like Tell + Query */
			if (! capable (CAP_SYS_ADMIN))
				return -EPERM;
			tmp = curr_quantums;
			curr_quantums = arg;
			return tmp;
		default: /* redundant, as cmd was checked against MAXNR */
			return -ENOTTY;
	}
	return retval;
}

int scull_open(struct inode *f_inode, struct file *filp)
{
    struct scull_dev *p_dev;

    DBG_FUNC_ENTER();

    p_dev = container_of(f_inode->i_cdev, struct scull_dev, cdev);
    filp->private_data = p_dev; // For other methods use

    DBG_INFO("%s(): %p\n", __func__, p_dev);
    // Trimming to zero in case of write only operation
    if ( (filp->f_flags & O_ACCMODE) == O_WRONLY) {
        scull_trim(p_dev); /* ignore errors */
    }

    DBG_FUNC_EXIT();
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
    int curr_qsets;
    int iter_qset;

    DBG_FUNC_ENTER();

    if (mutex_lock_interruptible(&p_dev->lock))
        return -ERESTARTSYS;

    curr_qsets = p_dev->curr_qsets;
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
    p_dev->curr_quantums = SCULL_QUANTUM_SIZE;
    p_dev->curr_qsets = SCULL_QSET_SIZE;
    p_dev->qdata = NULL;

    mutex_unlock(&p_dev->lock);

    DBG_FUNC_EXIT();
    return 0;
}

struct scull_qset* scull_follow(struct scull_dev *p_dev, int item)
{
    struct scull_qset *qset;
    DBG_FUNC_ENTER();

    qset = p_dev->qdata;

    // In case if there is nothing allocate first one
    if (!qset)
    {
        qset = kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
        p_dev->qdata = qset;

        if (qset == NULL)
        {
            DBG_FUNC_EXIT();
            return NULL;
        }

        memset(qset, 0, sizeof(struct scull_qset));
    }

    // Follow untill we reach end of the list
    while(item)
    {
        if (!qset->next)
        {
            // Ok we got the end lets allocate a new one
            qset->next = kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
            if (qset->next == NULL)
            {
                DBG_FUNC_EXIT();
                return NULL;
            }
            memset(qset->next, 0, sizeof(struct scull_qset));
        }

        qset = qset->next;
        item--;
    }

    DBG_FUNC_EXIT();
    return qset;
}

void *scull_seq_start(struct seq_file *s, loff_t *pos)
{
    if (*pos >= scull_nr_devs)
        return NULL; /* No more to read */

    return p_scull_dev + *pos;
}

void *scull_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
    (*pos)++;

    if (*pos >= scull_nr_devs)
        return NULL;

    return p_scull_dev + *pos;
}

void scull_seq_stop(struct seq_file *sfile, void *v)
{
    // Nothing to do
}

int scull_seq_show(struct seq_file *seq_fil_p, void *v)

{
    struct scull_dev *dev = (struct scull_dev *) v;
    struct scull_qset *curr_qset_p;
    int iter_data;

    DBG_FUNC_ENTER();

    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;

    seq_printf(seq_fil_p, "Device %i: curr_qsets=%i curr_quantums=%i total_size=%li\n",
            (int) (dev - p_scull_dev), dev->curr_qsets,
            dev->curr_quantums, dev->size);

    for (curr_qset_p = dev->qdata; curr_qset_p; curr_qset_p = curr_qset_p->next)
    {
        seq_printf(seq_fil_p, " item at %p, qset at %p\n", curr_qset_p, curr_qset_p->data);
        if (curr_qset_p->data && !curr_qset_p->next)
        {
            for (iter_data = 0; iter_data < dev->curr_qsets; iter_data++)
            {
                if (curr_qset_p->data[iter_data])
                    seq_printf(seq_fil_p, " % 4i: %8p\n",
                            iter_data, curr_qset_p->data[iter_data]);
            }
        }
    }

    mutex_unlock(&dev->lock);

    DBG_FUNC_EXIT();
    return 0;
}

int scull_proc_open(struct inode *inode, struct file *file)
{
    return seq_open(file, &scull_seq_ops);
}

module_init(scull_init);
module_exit(scull_exit);
