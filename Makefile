SUBDIRS = module test

random_module_name := ddfs_test_$(shell head /dev/urandom | tr -dc A-Za-z | head -c 20)

all: module

module:
	make -C module

test:
	make module_name=${random_module_name} module 
	make module_name=${random_module_name} -C test

runalltests: test .FORCE
	-cd test && ./run_all_tests.sh ${random_module_name}

clean:
	make -C module clean

.PHONY: all module test
.FORCE:
