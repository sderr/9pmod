EXTRA_CFLAGS += -Wno-cast-qual \
	-I$(src)/../../include/net/9p \
	-I$(src)/../../include \
	-I$(src) \
	-include $(src)/../../compat.h \
	-include $(src)/../../include/net/9p/9p.h \
	-include $(src)/../../include/net/9p/client.h \
	-include $(src)/../../include/net/9p/transport.h

EXTRA_CFLAGS += -DCONFIG_9P_FSCACHE=1

obj-m := 9p.o

9p-objs := \
	vfs_super.o \
	vfs_inode.o \
	vfs_inode_dotl.o \
	vfs_addr.o \
	vfs_file.o \
	vfs_dir.o \
	vfs_dentry.o \
	v9fs.o \
	fid.o \
	cache.o
