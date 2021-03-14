ccflags-y = -DEXPORT_SYMTAB
obj-m := corsair-psu.o

KDIR = /lib/modules/$(shell uname -r)/build/
PWD = $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	

.PHONY: modules clean

-include $(KDIR)/Rules.make
