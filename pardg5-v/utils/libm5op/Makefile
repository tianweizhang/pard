CC=gcc
AS=as
LD=ld
AR=ar
RANLIB=ranlib

CFLAGS=-O2 -DM5OP_ADDR=0xFFFF0000
ARFLAGS=cr
OBJS=libm5op.o m5op_x86.o

all: libm5op.a

%.o: %.S
	$(CC) $(CFLAGS) -o $@ -c $<

%.o: %.c
	$(CC)  $(CFLAGS) -o $@ -c $<

libm5op.a: $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)
	$(RANLIB) $@

clean:
	rm -f *.o libm5op.a
