/*
 * Copyright IBM Corporation, 2010
 * Author Aneesh Kumar K.V <aneesh.kumar@linux.vnet.ibm.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2.1 of the GNU Lesser General Public License
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it would be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#ifndef FS_9P_XATTR_H
#define FS_9P_XATTR_H

#include <linux/xattr.h>
#include <net/9p/9p.h>
#include <net/9p/client.h>

#if RHEL6_COMPAT
struct inode;
struct dentry;

struct compat_xattr_handler {
	const char *prefix;
	int flags;      /* fs private flags passed back to the handlers */
	size_t (*list)(struct dentry *dentry, char *list, size_t list_size,
	               const char *name, size_t name_len, int handler_flags);
	int (*get)(struct dentry *dentry, const char *name, void *buffer,
	               size_t size, int handler_flags);
	int (*set)(struct dentry *dentry, const char *name, const void *buffer,
	               size_t size, int flags, int handler_flags);
};



extern struct compat_xattr_handler *v9fs_xattr_handlers[];
ssize_t v9fs_vfs_getxattr(struct dentry *dentry, const char *name, void *buffer, size_t size);
ssize_t v9fs_vfs_listxattr(struct dentry *dentry, char *buffer, size_t buffer_size);
int v9fs_vfs_setxattr(struct dentry *dentry, const char *name, const void *value, size_t size, int flags);
int v9fs_vfs_removexattr(struct dentry *dentry, const char *name);
#else
extern const struct xattr_handler *v9fs_xattr_handlers[];
#endif

extern struct compat_xattr_handler v9fs_xattr_user_handler;
extern const struct compat_xattr_handler v9fs_xattr_acl_access_handler;
extern const struct compat_xattr_handler v9fs_xattr_acl_default_handler;

extern ssize_t v9fs_fid_xattr_get(struct p9_fid *, const char *,
				  void *, size_t);
extern ssize_t v9fs_xattr_get(struct dentry *, const char *,
			      void *, size_t);
extern int v9fs_xattr_set(struct dentry *, const char *,
			  const void *, size_t, int);
extern ssize_t v9fs_listxattr(struct dentry *, char *, size_t);
#endif /* FS_9P_XATTR_H */
