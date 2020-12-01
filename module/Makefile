KERNELDIR = /lib/modules/`uname -r`/build

ifndef module_name
	module_name=ddfs
endif

modules = ddfs_$(module_name)_test.ko
obj-m += ddfs_$(module_name)_test.o

# CFLAGS += -DDDFS_MODULE_NAME=\"$(module_name)\"

all:
	-echo "DDFS_MODULE_NAME = ${module_name}"
	-cp ddfs.c ddfs_$(module_name)_test.c
	make KCPPFLAGS='-DDDFS_MODULE_NAME=${module_name}' -C $(KERNELDIR) M=$(CURDIR) modules

clean:
	make -C $(KERNELDIR) M=$(CURDIR) clean
	-rm ddfs_*_test.*

insert: all rmmod
	sudo insmod ddfs.ko

rmmod:
	sudo rmmod ddfs

