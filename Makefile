CC=gcc
CFLAGS=-Wall -g
LIBCLAW_H_PATH=/usr/local/include
LIBCLAW_SO_PATH=/usr/local/lib

all: libclaw.so

libclaw.o: libclaw.c libclaw.h
	gcc -fPIC -c libclaw.c -o libclaw.o

libclaw.so: libclaw.o
	gcc -shared -o libclaw.so libclaw.o

install: all
	mkdir -p /usr/local/include /usr/local/lib
	cp libclaw.h  $(LIBCLAW_H_PATH)
	cp libclaw.so $(LIBCLAW_SO_PATH)
	chmod 644     $(LIBCLAW_H_PATH)/libclaw.h
	chmod 755     $(LIBCLAW_SO_PATH)/libclaw.so
	ldconfig
