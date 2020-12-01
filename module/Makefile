KERNELDIR = /lib/modules/`uname -r`/build

ifndef module_name
	module_name=ddfs
endif

obj-m += $(module_name).o

# CFLAGS += -DDDFS_MODULE_NAME=\"$(module_name)\"

all:
	-echo "Building module with name: ${module_name}"
	-if [ "${module_name}" != "ddfs" ]; then  cp ddfs.c $(module_name).c; fi
	make KCPPFLAGS='-DDDFS_MODULE_NAME=${module_name}' -C $(KERNELDIR) M=$(CURDIR) modules

clean:
	make -C $(KERNELDIR) M=$(CURDIR) clean
	-rm ddfs_test_*

insert: all rmmod
	sudo insmod ddfs.ko

rmmod:
	sudo rmmod ddfs

