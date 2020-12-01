SUBDIRS = module test

all: module

module:
	make -C module

test: module_name=ddfs_test_$(shell head /dev/urandom | tr -dc A-Za-z | head -c 20)
test:
	make module_name=$(module_name) module 
	make -C test

clean:
	make -C module clean

.PHONY: all module test
