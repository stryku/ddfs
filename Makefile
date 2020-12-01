SUBDIRS = module test

all: module test

module:
	make -C module

test:
	module_name=$(head /dev/urandom | tr -dc A-Za-z | head -c 20)
	make module_name=$(module_name) module 
	make -C test

.PHONY: all module test
