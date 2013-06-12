EXTRA_CFLAGS += -Wno-cast-qual \
	-I$(src)/../../include/net/9p \
	-I$(src)/../../include \
	-I$(src) \
	-include $(src)/../../compat.h \
	-include $(src)/../../include/net/9p/9p.h \
	-include $(src)/../../include/net/9p/client.h \
	-include $(src)/../../include/net/9p/transport.h

obj-m := 9p.o

ifeq ($(VERSION),2)
9p-objs := \
	compat/vfs_super.o \
	compat/vfs_inode.o \
	compat/vfs_inode_dotl.o \
	compat/vfs_addr.o \
	compat/vfs_file.o \
	compat/vfs_dir.o \
	compat/vfs_dentry.o \
	compat/v9fs.o \
	compat/fid.o
else
9p-objs := \
	vfs_super.o \
	vfs_inode.o \
	vfs_inode_dotl.o \
	vfs_addr.o \
	vfs_file.o \
	vfs_dir.o \
	vfs_dentry.o \
	v9fs.o \
	fid.o  \
	xattr.o \
	xattr_user.o
endif

#9p-$(CONFIG_9P_FSCACHE) += cache.o
#9p-$(CONFIG_9P_FS_POSIX_ACL) += acl.o
