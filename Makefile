ifneq ($(KERNELRELEASE),)
# kbuild part of makefile
obj-m  := fs/9p/ net/9p/

else
# Normal Makefile
# to build this KERNEL_HEADERS must be set.
KERNELDIR := $(KERNEL_HEADERS)

all::
	$(MAKE) -C $(KERNELDIR) M=`pwd` modules

clean::
	$(MAKE) -C $(KERNELDIR) M=`pwd` clean

install::
	$(MAKE) -C $(KERNELDIR) M=`pwd` modules_install
	
localinstall::
	$(MAKE) -C $(KERNELDIR) M=`pwd` INSTALL_MOD_PATH=`pwd`/modules modules_install
	
endif


