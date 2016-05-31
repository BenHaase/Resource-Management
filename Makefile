CC = gcc
CFLAGS = -g
TARGET1 = oss
TARGET2 = uproc
OBJS1 = oss.o rdesc.h
OBJS2 = uproc.o rdesc.h
LIBOBJS = rdesc.o
LIBS = -lrdesc -lpthread
MYLIBS = librdesc.a
LIBPATH = .
CLFILES = oss uproc uproc.o oss.o rdesc.o librdesc.a osstest
.SUFFIXES: .c .o

all: oss uproc $(MYLIBS)
	

$(TARGET1): $(OBJS1) $(MYLIBS)
	$(CC) -o $@ -L. $(OBJS1) $(LIBS)

$(TARGET2): $(OBJS2) $(MYLIBS)
	$(CC) -o $@ -L. $(OBJS2) $(LIBS)

$(MYLIBS): $(LIBOBJS)
	ar -rs $@ $(LIBOBJS)

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	/bin/rm -f $(CLFILES) cstest
