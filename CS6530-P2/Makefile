
ifdef D
   CXXFLAGS=-Wall -std=c++11 -g -pg -DDEBUG -fsanitize=address
else
   CXXFLAGS=-Wall -std=c++11 -g -O3 
endif



#CXXFLAGS=-Wall -std=c++11 -g -pg

CC=g++

all: test simple

unit_test: unit_test.cpp swap_space.hpp backing_store.o 

test: test.cpp betree.hpp swap_space.hpp backing_store.o

generate: generate.cpp

backing_store.o: backing_store.hpp backing_store.cpp

clean:
	$(RM) *.o test generate
