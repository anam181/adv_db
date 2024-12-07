
ifdef D
   CXXFLAGS=-Wall -std=c++11 -g -pg -DDEBUG
else
   CXXFLAGS=-Wall -std=c++11 -g -O3 
endif



#CXXFLAGS=-Wall -std=c++11 -g -pg

CC=g++

all: test test_logging_restore generate

test: test.cpp betree.hpp swap_space.o backing_store.o logger.o

test_logging_restore: test_logging_restore.cpp betree.hpp swap_space.o backing_store.o logger.o

generate: generate.cpp

swap_space.o: swap_space.cpp swap_space.hpp backing_store.hpp

backing_store.o: backing_store.hpp backing_store.cpp

logger.o: logger.cpp logger.hpp

clean:
	$(RM) *.o test test_logging_restore generate tmpdir/* version_map.txt kv_store.log output.txt
	touch version_map.txt kv_store.log output.txt

cleanfiles:
	$(RM) tmpdir/* version_map.txt kv_store.log output.txt
	touch version_map.txt kv_store.log output.txt
