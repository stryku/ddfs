# all: test

DDFS_DIR := $(shell mktemp -d)
DDFS_IMG := $(shell mktemp)

all: $(patsubst %.cpp, %.out, $(wildcard *.cpp)) 
%.out: %.cpp
	$(CXX) $(CFLAGS) -DDDFS_MOUNTED_DIR=\"${DDFS_DIR}\" $< -o $@ -lm

test: all

.PHONY: test .FORCE
