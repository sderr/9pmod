# to build this KERNEL_HEADERS must be set.
KERNELDIR := $(KERNEL_HEADERS)

all:: options
	$(MAKE) -C $(KERNELDIR) M=`pwd` "$$@" modules

clean::
	$(MAKE) -C $(KERNELDIR) M=`pwd` "$$@" clean
	rm -f 9pconfig.make 9pconfig.h

install:: all
	$(MAKE) -C $(KERNELDIR) M=`pwd` "$$@" modules_install
	
localinstall:: all
	$(MAKE) -C $(KERNELDIR) M=`pwd` INSTALL_MOD_PATH=`pwd`/modules "$$@" modules_install

remoteinstall:: localinstall
	$(eval LOCALVERSION := $(shell $(MAKE) -s -C $(KERNELDIR) kernelrelease))
	$(eval REMOTEVERSION := $(shell ssh root@$(HOST) uname -r))
	@if [ "$(LOCALVERSION)" != "$(REMOTEVERSION)" ]; then \
	echo ;\
	echo '*********** WARNING: Installing modules for a '\
	'DIFFERENT version than running on host. ***************';\
	echo "build is for      $(LOCALVERSION)";\
	echo "remote is running $(REMOTEVERSION)";\
	echo '**********************************************'\
	'*******************************************************';\
	echo ;\
	fi
	cd modules && scp -r lib/modules/$(LOCALVERSION) root@$(HOST):/lib/modules/
	ssh root@$(HOST) depmod -a $(LOCALVERSION)

options: 9pconfig.make 9pconfig.h

9pconfig.make: config.9p
	(echo '# Generated from config.9p' ; cat config.9p ) > 9pconfig.make

9pconfig.h: config.9p
	(echo '// Generated from config.9p' ; cat config.9p  | sed -n 's/^\(CONFIG[^=]*\).*/#define \1 1/p' ) > 9pconfig.h
