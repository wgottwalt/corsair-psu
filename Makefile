ccflags-y = -DEXPORT_SYMTAB
obj-m := corsair-psu.o corsair-psu-axi.o

KDIR = /lib/modules/$(shell uname -r)/build/
PWD = $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	

.PHONY: all clean

-include $(KDIR)/Rules.make
