STATIC_LIB = ../libcoroutine.a

LDFLAGS = -ldl -lpthread -lnsl -lm -lrt -lutil 

OBJECT_PATH ?= ../build

INCLUDE = -I../common/ -I../coroutine/

CXX = g++
CXXFLAGS = -Wall $(INCLUDE) -g -D__linux__ -std=c++17 -m64

.PHONY : all clean

all:channel_test

channel_test:channel_test.o
	$(CXX) channel_test.o $(STATIC_LIB) $(LDFLAGS) -o $@

channel_test.o:channel_test.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<   

mutex_test:mutex_test.o
	$(CXX) mutex_test.o $(STATIC_LIB) $(LDFLAGS) -o $@

mutex_test.o:mutex_test.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<   

simple_test:simple_test.o
	$(CXX) simple_test.o $(STATIC_LIB) $(LDFLAGS) -o $@

simple_test.o:simple_test.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<   

clean:
	rm -rf mutex_test mutex_test.o;
	rm -rf simple_test simple_test.o;
	rm -rf channel_test channel_test.o;
