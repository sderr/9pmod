# If the infiniband modules where not built inside the kernel tree,
# set the env variable OFED_HEADERS point to their location
ifneq ($(OFED_HEADERS),)
KBUILD_EXTRA_SYMBOLS += $(OFED_HEADERS)/Module.symvers
# EXTRA_CFLAGS is included AFTER standard includes. Not good.
# To add include directories at the beginning of the list,
# we need to *modify* LINUXINCLUDE (wich already contains include, arch..)
OFEDINCLUDE = -I$(OFED_HEADERS)/include
endif

include $(M)/9p.config

obj-m  := fs/9p/ net/9p/
