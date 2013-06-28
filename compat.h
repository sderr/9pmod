#ifndef _RHEL6_COMPAT_H
#define _RHEL6_COMPAT_H

/* Tested with RHEL 6.3 (2.6.32-204) */

/* use when changing the code is unavoidable */
#define RHEL6_COMPAT 1
#define CONFIG_NET_9P_DEBUG 1

/* prereqs for 9p includes pulled in early with -include */
#include <linux/types.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/sched.h>
//#include <linux/parser.h> /* careful: not set up for multiple includes */

/* missing */
#include <linux/magic.h>
#ifndef V9FS_MAGIC
#define V9FS_MAGIC		0x01021997
#endif

/* missing */
#include <linux/kernel.h>
#undef pr_fmt /* defined by kernel.h but we don't want it. */
#include <linux/version.h>

#ifndef USHRT_MAX
#define USHRT_MAX ((u16)(~0U))
#endif


/* missing */
#include <linux/nsproxy.h>

#define P9_DPRINTK p9_debug
#define P9_EPRINTK(level, args...) printk(level args)

/* missing - copied from fs/attr.c in 2.6.38-rc2 */
static __inline__
void setattr_copy(struct inode *inode, const struct iattr *attr)
{
        unsigned int ia_valid = attr->ia_valid;

        if (ia_valid & ATTR_UID)
                inode->i_uid = attr->ia_uid;
        if (ia_valid & ATTR_GID)
                inode->i_gid = attr->ia_gid;
        if (ia_valid & ATTR_ATIME)
                inode->i_atime = timespec_trunc(attr->ia_atime,
                                                inode->i_sb->s_time_gran);
        if (ia_valid & ATTR_MTIME)
                inode->i_mtime = timespec_trunc(attr->ia_mtime,
                                                inode->i_sb->s_time_gran);
        if (ia_valid & ATTR_CTIME)
                inode->i_ctime = timespec_trunc(attr->ia_ctime,
                                                inode->i_sb->s_time_gran);
        if (ia_valid & ATTR_MODE) {
                umode_t mode = attr->ia_mode;

                if (!in_group_p(inode->i_gid) && !capable(CAP_FSETID))
                        mode &= ~S_ISGID;
                inode->i_mode = mode;
        }
}

/* missing */
#define flush_work_sync flush_work

#include <linux/net.h>
#include <linux/syscalls.h> /* killme */

static inline struct file *sock_alloc_file(struct socket *sock, int flags, const char *dname)
{
	int fd;
	fd = sock_map_fd(sock, 0);
	if (fd < 0)
		return ERR_PTR(fd);
	get_file(sock->file);
	sys_close(fd);  /* still racy */
	return sock->file;
}

struct p9_fid;
extern ssize_t v9fs_fid_readn(struct p9_fid *fid, char *data, char __user *udata, u32 count,
	       u64 offset);
extern ssize_t v9fs_fid_writen(struct p9_fid *fid, const char *data, const char __user *udata, u32 count,
	       u64 offset);

#endif /* _RHEL6_COMPAT_H */
