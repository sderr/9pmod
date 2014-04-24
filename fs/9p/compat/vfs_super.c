/*
 *  linux/fs/9p/vfs_super.c
 *
 * This file contians superblock ops for 9P2000. It is intended that
 * you mount this file system on directories.
 *
 *  Copyright (C) 2004 by Eric Van Hensbergen <ericvh@gmail.com>
 *  Copyright (C) 2002 by Ron Minnich <rminnich@lanl.gov>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to:
 *  Free Software Foundation
 *  51 Franklin Street, Fifth Floor
 *  Boston, MA  02111-1301  USA
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/inet.h>
#include <linux/pagemap.h>
#include <linux/seq_file.h>
#include <linux/mount.h>
#include <linux/idr.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/statfs.h>
#include <linux/magic.h>
#include <net/9p/9p.h>
#include <net/9p/client.h>

#include "v9fs.h"
#include "v9fs_vfs.h"
#include "fid.h"
#include "xattr.h"
#include "acl.h"

static const struct super_operations v9fs_super_ops, v9fs_super_ops_dotl;

/**
 * v9fs_set_super - set the superblock
 * @s: super block
 * @data: file system specific data
 *
 */

static int v9fs_set_super(struct super_block *s, void *data)
{
	s->s_fs_info = data;
	return set_anon_super(s, data);
}

/**
 * v9fs_fill_super - populate superblock with info
 * @sb: superblock
 * @v9ses: session information
 * @flags: flags propagated from v9fs_mount()
 *
 */

static void
v9fs_fill_super(struct super_block *sb, struct v9fs_session_info *v9ses,
		int flags, void *data)
{
	sb->s_maxbytes = MAX_LFS_FILESIZE;
	sb->s_blocksize_bits = fls(v9ses->maxdata - 1);
	sb->s_blocksize = 1 << sb->s_blocksize_bits;
	sb->s_magic = V9FS_MAGIC;
	if (v9fs_proto_dotl(v9ses)) {
		sb->s_op = &v9fs_super_ops_dotl;
#if RHEL6_COMPAT
		sb->s_xattr = NULL;
#else
		sb->s_xattr = v9fs_xattr_handlers;
#endif
	} else
		sb->s_op = &v9fs_super_ops;
#if RHEL6_COMPAT
	/* not used on rhel6 */
#else
	sb->s_bdi = &v9ses->bdi;
#endif

	sb->s_flags = flags | MS_ACTIVE | MS_SYNCHRONOUS | MS_DIRSYNC |
	    MS_NOATIME;

#ifdef CONFIG_9P_FS_POSIX_ACL
	if ((v9ses->flags & V9FS_ACCESS_MASK) == V9FS_ACCESS_CLIENT)
		sb->s_flags |= MS_POSIXACL;
#endif

	save_mount_options(sb, data);
}

/**
 * v9fs_mount - mount a superblock
 * @fs_type: file system type
 * @flags: mount flags
 * @dev_name: device name that was mounted
 * @data: mount options
 *
 */
#if RHEL6_COMPAT
static int v9fs_get_sb(struct file_system_type *fs_type, int flags,
                       const char *dev_name, void *data,
                       struct vfsmount *mnt)
#else
static struct dentry *v9fs_mount(struct file_system_type *fs_type, int flags,
		       const char *dev_name, void *data)
#endif
{
	struct super_block *sb = NULL;
	struct inode *inode = NULL;
	struct dentry *root = NULL;
	struct v9fs_session_info *v9ses = NULL;
	int mode = S_IRWXUGO | S_ISVTX;
	struct p9_fid *fid;
	int retval = 0;

	P9_DPRINTK(P9_DEBUG_VFS, " \n");

	v9ses = kzalloc(sizeof(struct v9fs_session_info), GFP_KERNEL);
	if (!v9ses)
#if RHEL6_COMPAT
		return -ENOMEM;
#else
		return ERR_PTR(-ENOMEM);
#endif

	fid = v9fs_session_init(v9ses, dev_name, data);
	if (IS_ERR(fid)) {
		retval = PTR_ERR(fid);
		/*
		 * we need to call session_close to tear down some
		 * of the data structure setup by session_init
		 */
		goto close_session;
	}

	sb = sget(fs_type, NULL, v9fs_set_super, v9ses);
	if (IS_ERR(sb)) {
		retval = PTR_ERR(sb);
		goto clunk_fid;
	}
	v9fs_fill_super(sb, v9ses, flags, data);

#if RHEL6_COMPAT
	/* N.B. inserted code in vfs_inode.c::v9fs_create() and
	 * vfs_inode.c::v9fs_vfs_lookup ().
	 */
#else
	if (v9ses->cache)
		sb->s_d_op = &v9fs_cached_dentry_operations;
	else
		sb->s_d_op = &v9fs_dentry_operations;
#endif
	inode = v9fs_get_inode(sb, S_IFDIR | mode);
	if (IS_ERR(inode)) {
		retval = PTR_ERR(inode);
		goto release_sb;
	}

	root = d_alloc_root(inode);
	if (!root) {
		iput(inode);
		retval = -ENOMEM;
		goto release_sb;
	}
	sb->s_root = root;
	if (v9fs_proto_dotl(v9ses)) {
		struct p9_stat_dotl *st = NULL;
		st = p9_client_getattr_dotl(fid, P9_STATS_BASIC);
		if (IS_ERR(st)) {
			retval = PTR_ERR(st);
			goto release_sb;
		}

		v9fs_stat2inode_dotl(st, root->d_inode);
		kfree(st);
	} else {
		struct p9_wstat *st = NULL;
		st = p9_client_stat(fid);
		if (IS_ERR(st)) {
			retval = PTR_ERR(st);
			goto release_sb;
		}

		root->d_inode->i_ino = v9fs_qid2ino(&st->qid);
		v9fs_stat2inode(st, root->d_inode, sb);

		p9stat_free(st);
		kfree(st);
	}
	retval = v9fs_get_acl(inode, fid);
	if (retval)
		goto release_sb;
	v9fs_fid_add(root, fid);

	P9_DPRINTK(P9_DEBUG_VFS, " simple set mount, return 0\n");
#if RHEL6_COMPAT
	simple_set_mnt(mnt, sb);
	return 0;
#else
	return dget(sb->s_root);
#endif

clunk_fid:
	p9_client_clunk(fid);
close_session:
	v9fs_session_close(v9ses);
	kfree(v9ses);
#if RHEL6_COMPAT
	return retval;
#else
	return ERR_PTR(retval);
#endif

release_sb:
	/*
	 * we will do the session_close and root dentry release
	 * in the below call. But we need to clunk fid, because we haven't
	 * attached the fid to dentry so it won't get clunked
	 * automatically.
	 */
	p9_client_clunk(fid);
	deactivate_locked_super(sb);
#if RHEL6_COMPAT
	return retval;
#else
	return ERR_PTR(retval);
#endif
}

/**
 * v9fs_kill_super - Kill Superblock
 * @s: superblock
 *
 */

static void v9fs_kill_super(struct super_block *s)
{
	struct v9fs_session_info *v9ses = s->s_fs_info;

	P9_DPRINTK(P9_DEBUG_VFS, " %p\n", s);

#if RHEL6_COMPAT
	v9fs_dentry_release(s->s_root); /* clunk root */
#endif
	kill_anon_super(s);

	v9fs_session_cancel(v9ses);
	v9fs_session_close(v9ses);
	kfree(v9ses);
	s->s_fs_info = NULL;
	P9_DPRINTK(P9_DEBUG_VFS, "exiting kill_super\n");
}

static void
v9fs_umount_begin(struct super_block *sb)
{
	struct v9fs_session_info *v9ses;

	v9ses = sb->s_fs_info;
	v9fs_session_begin_cancel(v9ses);
}

static int v9fs_statfs(struct dentry *dentry, struct kstatfs *buf)
{
	struct v9fs_session_info *v9ses;
	struct p9_fid *fid;
	struct p9_rstatfs rs;
	int res;

	fid = v9fs_fid_lookup(dentry);
	if (IS_ERR(fid)) {
		res = PTR_ERR(fid);
		goto done;
	}

	v9ses = v9fs_inode2v9ses(dentry->d_inode);
	if (v9fs_proto_dotl(v9ses)) {
		res = p9_client_statfs(fid, &rs);
		if (res == 0) {
			buf->f_type = rs.type;
			buf->f_bsize = rs.bsize;
			buf->f_blocks = rs.blocks;
			buf->f_bfree = rs.bfree;
			buf->f_bavail = rs.bavail;
			buf->f_files = rs.files;
			buf->f_ffree = rs.ffree;
			buf->f_fsid.val[0] = rs.fsid & 0xFFFFFFFFUL;
			buf->f_fsid.val[1] = (rs.fsid >> 32) & 0xFFFFFFFFUL;
			buf->f_namelen = rs.namelen;
		}
		if (res != -ENOSYS)
			goto done;
	}
	res = simple_statfs(dentry, buf);
done:
	return res;
}

static const struct super_operations v9fs_super_ops = {
#ifdef CONFIG_9P_FSCACHE
	.alloc_inode = v9fs_alloc_inode,
	.destroy_inode = v9fs_destroy_inode,
#endif
	.statfs = simple_statfs,
#if RHEL6_COMPAT
	.clear_inode = v9fs_clear_inode,
#else
	.evict_inode = v9fs_evict_inode,
#endif
	.show_options = generic_show_options,
	.umount_begin = v9fs_umount_begin,
};

static const struct super_operations v9fs_super_ops_dotl = {
#ifdef CONFIG_9P_FSCACHE
	.alloc_inode = v9fs_alloc_inode,
	.destroy_inode = v9fs_destroy_inode,
#endif
	.statfs = v9fs_statfs,
#if RHEL6_COMPAT
	.clear_inode = v9fs_clear_inode,
#else
	.evict_inode = v9fs_evict_inode,
#endif
	.show_options = generic_show_options,
	.umount_begin = v9fs_umount_begin,
};

struct file_system_type v9fs_fs_type = {
	.name = "9p",
#if RHEL6_COMPAT
	.get_sb = v9fs_get_sb,
#else
	.mount = v9fs_mount,
#endif
	.kill_sb = v9fs_kill_super,
	.owner = THIS_MODULE,
	.fs_flags = FS_RENAME_DOES_D_MOVE,
};