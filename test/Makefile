all: test

DDFS_DIR := $(shell mktemp -d)
DDFS_IMG := $(shell mktemp)

cpptest: test_create_file.cpp
	$(CXX) -DDDFS_MOUNTED_DIR=

test:
	./test_setup.sh ${module_name} ${DDFS_DIR} ${DDFS_IMG}
	./test_teardown.sh ${module_name} ${DDFS_DIR}
	# ./test_dry_run.sh ${module_name} | tee test_dry_run.log

.PHONY: test
