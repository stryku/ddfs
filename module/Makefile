KERNELDIR = /lib/modules/`uname -r`/build
modules = ddfs.ko
obj-m += ddfs.o

# ifndef module_name
#     module_name=ddfs
# endif

# CFLAGS += -DDDFS_MODULE_NAME=$module_name

all:
	-echo $(CURDIR)
	make -C $(KERNELDIR) M=$(CURDIR) modules

clean:
	make -C $(KERNELDIR) M=$(CURDIR) clean

insert: all rmmod
	sudo insmod ddfs.ko

rmmod:
	sudo rmmod ddfs

