# /**
#  * Name: Liyao Jiang
#  * ID: 1512446
#  */
CC = g++
CFLAGS = -Wall -O2 -Werror -pthread

HEADERS = $(wildcard *.h)
C_SOURCES = $(wildcard *.c)
CC_SOURCES = $(wildcard *.cc)

C_OBJECTS = $(C_SOURCES:%.c=%.o)
CC_OBJECTS = $(CC_SOURCES:%.cc=%.o)
OBJECTS = $(C_OBJECTS) $(CC_OBJECTS)

SUBMISSION_FILES = distwc.c threadpool.h mapreduce.h threadpool.cc mapreduce.cc Makefile readme.md
.PHONY: all clean

wc: $(OBJECTS) $(HEADERS)
	$(CC) $(CFLAGS) -o wordcount $(OBJECTS)

compile: $(OBJECTS) $(HEADERS)

$(C_OBJECTS): %.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

$(CC_OBJECTS): %.o: %.cc
	$(CC) $(CFLAGS) -c $^ -o $@

all: wc

clean:
	rm *.o wordcount
	
compress: $(SUBMISSION_FILES)
	zip -q -r mapreduce.zip $(SUBMISSION_FILES)