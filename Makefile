#CC = gcc
CC = g++
CFLAGS ?= -Iinclude -I/usr/include/libdrm -I/home/fred/dependencies/gst-plugins-base/gst-libs/ -I/usr/local/include/opencv4/ -W -Wall -Wextra -g -O2 -std=c11 `pkg-config --cflags --libs gstreamer-1.0`
LDFLAGS	?= -L/home/fred/dependencies/gst-plugins-base/build/gst/app/ -L/usr/local/lib/
LIBS	:= -lopencv_core -lopencv_highgui -lopencv_imgproc

#export PKG_CONFIG_PATH := /home/fred/dependencies/gst-plugins-base/build/gst/app/:$(PKG_CONFIG_PATH)

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<
	
all: gstTest

gstTest: gstTest.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS) `pkg-config --cflags --libs gstreamer-1.0` /home/fred/dependencies/gst-plugins-base/build/gst-libs/gst/app/libgstapp-1.0.so.0.1603.0
	-rm *.o
	

clean:
	-rm -f *.o
	-rm -f gstTest
