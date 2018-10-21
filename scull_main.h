/* System headers */
#include <linux/version.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kern_levels.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mutex.h>




/* Local headers */




/* Defines */
#define SCULL_QUANTUM_SIZE        4000
#define SCULL_QSET_SIZE           1000

/* Use 'k' as magic number */
#define SCULL_IOC_MAGIC 'k'
/* Please use a different 8-bit number in your code */
#define SCULL_IOCRESET _IO(SCULL_IOC_MAGIC, 0)
/*
 * S means "Set" through a ptr,
 * T means "Tell" directly with the argument value
 * G means "Get": reply by setting through a pointer
 * Q means "Query": response is on the return value
 * X means "eXchange": switch G and S atomically
 * H means "sHift": switch T and Q atomically
 */
#define SCULL_IOCSQUANTUM _IOW(SCULL_IOC_MAGIC, 1, int)
#define SCULL_IOCSQSET _IOW(SCULL_IOC_MAGIC, 2, int)
#define SCULL_IOCTQUANTUM _IO(SCULL_IOC_MAGIC, 3)
#define SCULL_IOCTQSET _IO(SCULL_IOC_MAGIC, 4)
#define SCULL_IOCGQUANTUM _IOR(SCULL_IOC_MAGIC, 5, int)
#define SCULL_IOCGQSET _IOR(SCULL_IOC_MAGIC, 6, int)
#define SCULL_IOCQQUANTUM _IO(SCULL_IOC_MAGIC, 7)
#define SCULL_IOCQQSET _IO(SCULL_IOC_MAGIC, 8)
#define SCULL_IOCXQUANTUM _IOWR(SCULL_IOC_MAGIC, 9, int)
#define SCULL_IOCXQSET _IOWR(SCULL_IOC_MAGIC,10, int)
#define SCULL_IOCHQUANTUM _IO(SCULL_IOC_MAGIC, 11)
#define SCULL_IOCHQSET _IO(SCULL_IOC_MAGIC, 12)

#define SCULL_IOC_MAXNR 14


/* Function declaration - Externs/Exported */



/* Function declaration - static */


/* Debug macros*/
#ifdef SCULL_DEBUG_TRACE
#define SCULL_DEBUG_LEVEL KERN_INFO
#define DBG_INFO(formate, ...) (printk(SCULL_DEBUG_LEVEL "SCULL[INFO]: "formate, ##__VA_ARGS__))
#define DBG_MED(formate, ...) (printk(SCULL_DEBUG_LEVEL "SCULL[MED]: "formate, ##__VA_ARGS__))
#define DBG_HIGH(formate, ...) (printk(SCULL_DEBUG_LEVEL "SCULL[HIGH]: "formate, ##__VA_ARGS__))
#else
#define DBG_INFO(formate, ...)
#define DBG_MED(formate, ...)
#define DBG_HIGH(formate, ...)
#endif

#ifdef SCULL_FUNC_TRACE
#define DBG_FUNC_ENTER() (printk(SCULL_DEBUG_LEVEL "SCULL %s(): Enter\n", __func__))
#define DBG_FUNC_EXIT() (printk(SCULL_DEBUG_LEVEL "SCULL %s(): Exit\n", __func__))
#else
#define DBG_FUNC_ENTER()
#define DBG_FUNC_EXIT()
#endif
