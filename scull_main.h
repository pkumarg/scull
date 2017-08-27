/* System headers */
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




/* Local headers */




/* Defines */



/* Function declaration - Externs/Exported */



/* Function declaration - static */


/* Debug macros*/
#ifdef SCULL_DEBUG_TRACE
#define SCULL_DEBUG_LEVEL KERN_INFO
#define DBG_INFO(formate, ...) (printk(SCULL_DEBUG_LEVEL "SCULL[INFO]: "formate, __VA_ARGS__))
#define DBG_MED(formate, ...) (printk(SCULL_DEBUG_LEVEL "SCULL[MED]: "formate, __VA_ARGS__))
#define DBG_HIGH(formate, ...) (printk(SCULL_DEBUG_LEVEL "SCULL[HIGH]: "formate, __VA_ARGS__))
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
