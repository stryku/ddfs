SUBDIRS = module test

all: module test

module:
	make -C module

test:
	make -C test

.PHONY: all module test
