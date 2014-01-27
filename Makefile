# Makefile for hctest
# [15/01/2014]

SHELL = /bin/sh

prefix      = 
exec_prefix = 
bindir      = /usr/local/bin
mandir      = /usr/local/man

CC      = gcc
CFLAGS  = -g -Wall -DPROGRAM_VERSION=\"1.0\" -DPROGRAM_NAME=\"test\"
LDFLAGS = -lgd -lfftw3

OBJS  = test.o log.o parse.o src_v4l2.o fft.o butterworth.o
#OBJS += dec_rgb.o dec_yuv.o dec_grey.o dec_bayer.o dec_jpeg.o dec_png.o
#OBJS += dec_s561.o

all: test 

install: all
	mkdir -p ${DESTDIR}${bindir}
	mkdir -p ${DESTDIR}${mandir}/man1
	install -m 755 test ${DESTDIR}${bindir}

test: $(OBJS)
	$(CC) -o test $(OBJS) $(LDFLAGS)

.c.o:
	${CC} ${CFLAGS} -c $< -o $@

clean:
	rm -f core* *.o test

distclean: clean
	rm -rf *~

