CXXFLAGS += \
	-I../3rdparty \
	-I../../module \
	-DDDFS_BUILDING_TESTS

all: $(patsubst %.cpp, %.out, $(wildcard *.cpp)) 
%.out: %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@ -lm

test: all

.PHONY: test
