EXTRA_CFLAGS += \
	-I$(src)/../../include/net/9p \
	-I$(src)/../../include \
	-include $(src)/../../compat.h \
	-include $(src)/../../include/net/9p/9p.h \
	-include $(src)/../../include/net/9p/client.h \
	-include $(src)/../../include/net/9p/transport.h

obj-m := 9pnet.o
#obj-m += 9pnet_virtio.o
obj-m += 9pnet_rdma.o

9pnet-objs := \
	mod.o \
	client.o \
	error.o \
	util.o \
	protocol.o \
	trans_fd.o \
	trans_common.o \
	stats.o	\

9pnet_virtio-objs := \
	trans_virtio.o \

9pnet_rdma-objs := \
	trans_rdma.o \
