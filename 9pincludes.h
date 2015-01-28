#include <linux/module.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/idr.h>
#include <linux/slab.h>

//#define CONFIG_9P_FSCACHE 1
#define CONFIG_9P_FS_POSIX_ACL 1
//#define CONFIG_9P_FS_SECURITY 1
#define CONFIG_NET_9P_DEBUG 1
