SUBDIRS = module test

random_module_name := ddfs_test_$(shell head /dev/urandom | tr -dc A-Za-z | head -c 20)

all: module

module:
	make -C module

unittest:
	make -C test/unit

test:
	make module_name=${random_module_name} module
	make module_name=${random_module_name} -C test

rununittests: unittest .FORCE
	-set -e
	-cd test/unit && ./run_all_tests.sh

rune2etests: test .FORCE
	-set -e
	-cd test/e2e && ./run_all_tests.sh ${random_module_name}

runalltests: test .FORCE
	-cd test/unit && ./run_all_tests.sh && cd ../e2e && python3 run_all_tests.py ${random_module_name}

clean:
	make -C module clean
	make -C test clean

.PHONY: all module test
.FORCE:
