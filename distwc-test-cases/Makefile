CC:=g++
WARN:=-Wall 
LIB:=-lm -pthread -O3
CCOPTS:=-std=c99 -ggdb -D_GNU_SOURCE
TARGET:=distwc

all: threadpool.o mapreduce.o

%.o: %.cc
	$(CC) $(WARN) $(CCOPTS) $< -c $(LIB)

clean-all:
	rm -rf *.o *.gch $(TARGET)

threadpool.o: threadpool.cc threadpool.h
mapreduce.o: mapreduce.cc mapreduce.h threadpool.h
