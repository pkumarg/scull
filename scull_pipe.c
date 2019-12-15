struct scull_pipe {
    wait_queue_head_t inq, outq;
    char *buffer, *end;
    int buffersize;
    char *rp, *wp;
    int nreaders, nwriters;
    struct fasync_struct *async_queue;
    struct semaphore sem;
    struct cdev cdev;
};

static ssize_t scull_p_read (struct file *filp, char __user *buf, size_t count,
        loff_t *f_pos)
{
    struct scull_pipe *dev = filp->private_data;

    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;

    /* nothing to read */
    while (dev->rp == dev->wp)
    {
        up(&dev->sem); /* release the lock */
        if (filp->f_flags & O_NONBLOCK)
            return -EAGAIN;

        SCULL_HIGH("\"%s\" reading: going to sleep\n", current->comm);
        if (wait_event_interruptible(dev->inq, (dev->rp != dev->wp)))
            return -ERESTARTSYS; /* signal: tell the fs layer to handle it */

        /* otherwise loop, but first reacquire the lock */
        if (down_interruptible(&dev->sem))
            return -ERESTARTSYS;
    }

    /* ok, data is there, return something */
    if (dev->wp > dev->rp)
        count = min(count, (size_t)(dev->wp - dev->rp));
    else /* the write pointer has wrapped, return data up to dev->end */
        count = min(count, (size_t)(dev->end - dev->rp));

    if (copy_to_user(buf, dev->rp, count))
    {
        up (&dev->sem);
        return -EFAULT;
    }
    dev->rp += count;
    if (dev->rp == dev->end)
        dev->rp = dev->buffer; /* wrapped */

    up (&dev->sem);

    /* finally, awake any writers and return */
    wake_up_interruptible(&dev->outq);

    SCULL_INFO("\"%s\" did read %li bytes\n", current->comm, (long)count);

    return count;
}

/* How much space is free? */
static int spacefree(struct scull_pipe *dev)
{
    if (dev->rp = = dev->wp)
        return dev->buffersize - 1;
    return ((dev->rp + dev->buffersize - dev->wp) % dev->buffersize) - 1;
}

/* Wait for space for writing; caller must hold device semaphore. On
 * * error the semaphore will be released before returning. */
static int scull_getwritespace(struct scull_pipe *dev, struct file *filp)
{
    while (spacefree(dev) = = 0) { /* full */
        DEFINE_WAIT(wait);
        up(&dev->sem);
        if (filp->f_flags & O_NONBLOCK)
            return -EAGAIN;
        PDEBUG("\"%s\" writing: going to sleep\n",current->comm);
        prepare_to_wait(&dev->outq, &wait, TASK_INTERRUPTIBLE);
        if (spacefree(dev) = = 0)
            schedule( );
        finish_wait(&dev->outq, &wait);
        if (signal_pending(current))
            return -ERESTARTSYS; /* signal: tell the fs layer to handle it */
        if (down_interruptible(&dev->sem))
            return -ERESTARTSYS;
    }
    return 0;
}

static ssize_t scull_p_write(struct file *filp, const char __user *buf, size_t count,
        loff_t *f_pos)
{
    struct scull_pipe *dev = filp->private_data;
    int result;

    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;

    /* Make sure there's space to write */
    result = scull_getwritespace(dev, filp);
    if (result)
        return result; /* scull_getwritespace called up(&dev->sem) */
    /* ok, space is there, accept something */
    count = min(count, (size_t)spacefree(dev));
    if (dev->wp >= dev->rp)
        count = min(count, (size_t)(dev->end - dev->wp)); /* to end-of-buf */
    else /* the write pointer has wrapped, fill up to rp-1 */
        count = min(count, (size_t)(dev->rp - dev->wp - 1));
    PDEBUG("Going to accept %li bytes to %p from %p\n", (long)count, dev->wp, buf);
    if (copy_from_user(dev->wp, buf, count)) {
        up (&dev->sem);
        return -EFAULT;
    }

    dev->wp += count;
    if (dev->wp = = dev->end)
        dev->wp = dev->buffer; /* wrapped */
    up(&dev->sem);
    /* finally, awake any reader */
    wake_up_interruptible(&dev->inq);
    /* blocked in read( ) and select( ) */
    /* and signal asynchronous readers, explained late in chapter 5 */
    if (dev->async_queue)
        kill_fasync(&dev->async_queue, SIGIO, POLL_IN);
    PDEBUG("\"%s\" did write %li bytes\n",current->comm, (long)count);
    return count;
}

static unsigned int scull_p_poll(struct file *filp, poll_table *wait)
{
    struct scull_pipe *dev = filp->private_data;
    unsigned int mask = 0;
    /*
     * * The buffer is circular; it is considered full
     * * if "wp" is right behind "rp" and empty if the
     * * two are equal.
     * */
    down(&dev->sem);
    poll_wait(filp, &dev->inq, wait);
    poll_wait(filp, &dev->outq, wait);
    if (dev->rp != dev->wp)
        mask |= POLLIN | POLLRDNORM;
    /* readable */
    if (spacefree(dev))
        mask |= POLLOUT | POLLWRNORM;
    /* writable */
    up(&dev->sem);
    return mask;
}

static int scull_p_fasync(int fd, struct file *filp, int mode)
{
    struct scull_pipe *dev = filp->private_data;
    return fasync_helper(fd, filp, mode, &dev->async_queue);
}
