all: test

cpptest: test_create_file.cpp
	$(CXX) -DDDFS_MOUNTED_DIR=

test:
	./test_dry_run.sh ${module_name} | tee test_dry_run.log

.PHONY: test
