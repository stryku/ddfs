SUBDIRS = module test

all: module test

module:
	make -C module

test: module_name=ddfs_$(shell head /dev/urandom | tr -dc A-Za-z | head -c 20)
test:
	make module_name=$(module_name) module 
	make -C test

clean:
	make -C module clean

.PHONY: all module test
