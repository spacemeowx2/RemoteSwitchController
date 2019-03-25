ifneq ($(KERNELRELEASE),)
obj-m   := switch_pro.o
else
KDIR    := /lib/modules/$(shell uname -r)/build
PWD     := $(shell pwd)
all:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
install:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules_install
%:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) $@
endif
