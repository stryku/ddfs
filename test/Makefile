all: test

DDFS_DIR := $(shell mktemp -d)
DDFS_IMG := $(shell mktemp)

all: $(patsubst %.cpp, %.out, $(wildcard *.cpp))
%.out: %.cpp Makefile
	./test_setup.sh ${module_name} ${DDFS_DIR} ${DDFS_IMG}
	$(CXX) $(CFLAGS) -DDDFS_MOUNTED_DIR=\"${DDFS_DIR}\" $< -o $@ -lm
	./$@
	./test_teardown.sh ${module_name} ${DDFS_DIR}

cpptest: test_create_file.cpp
	$(CXX) -DDDFS_MOUNTED_DIR=

test: all



.PHONY: test
