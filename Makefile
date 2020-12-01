SUBDIRS = module

all: module

module:
	make -C module


.PHONY: all module