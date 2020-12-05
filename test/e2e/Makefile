CXXFLAGS += -I../3rdparty

all: $(patsubst %.cpp, %.out, $(wildcard *.cpp)) 
%.out: %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@ -lm

test: all

.PHONY: test
