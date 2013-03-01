# to build this KERNEL_HEADERS must be set.
KERNELDIR := $(KERNEL_HEADERS)

# If the infiniband modules where not built inside the kernel tree,
# set the env variable OFED_HEADERS point to their location
ifneq ($(OFED_HEADERS),)
EXTRA := KBUILD_EXTRA_SYMBOLS=$(OFED_HEADERS)/Module.symvers
endif

all::
	$(MAKE) -C $(KERNELDIR) M=`pwd` $(EXTRA) "$$@" modules

clean::
	$(MAKE) -C $(KERNELDIR) M=`pwd` "$$@" clean

install::
	$(MAKE) -C $(KERNELDIR) M=`pwd` "$$@" modules_install
	
localinstall::
	$(MAKE) -C $(KERNELDIR) M=`pwd` INSTALL_MOD_PATH=`pwd`/modules "$$@" modules_install
