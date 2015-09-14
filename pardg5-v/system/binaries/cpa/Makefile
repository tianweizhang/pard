#
# Makefile for the PARDg5-V ControlPanel Adaptor driver
#

obj-m := cpa.o
ALL_CTRLPLANE := cache.o memcntrl.o
cpa-objs := init.o main.o subsystem.o ctrlplane.o generic.o $(ALL_CTRLPLANE)

KDIR := $(shell pwd)/../linux-2.6.28.4-GM/
PWD := $(shell pwd)
EXTRA_CFLAGS := -std=gnu99

init-objs := init.c cpa.h
main-objs := main.c cpa.h cpa_ioctl.h
subsystem-objs := subsystem.c cpa.h

ctrlplane-objs := ctrlplane.c ctrlplane.h cpa.h
generic-objs := generic.c ctrlplane.h cpa.h cpa_ioctl.h
cache-objs := cache.c ctrlplane.h cpa_ioctrl.h
memcntrl-objs := memcntrl.c ctrlplane.h

all: mmaptest ioctltest
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -rf modules.order mmaptest ioctltest

mmaptest: mmaptest.c

ioctltest: ioctltest.c
	gcc $< -o $@ -static -std=c99

install: install-swap
	sudo mount -o loop,offset=32256 ../../disks/PARDg5GM.img ./.mnt
	sudo cp -rf cpa.ko .mnt/lib/modules/2.6.28.4-gc395911-dirty/cpa.ko
	sudo umount .mnt

install-swap: mmaptest ioctltest
	sudo mount -o loop,offset=32256 ../../disks/linux-smallswap2.img ./.mnt
	sudo mkdir -p .mnt/2.6.28.4-gc395911-dirty
	sudo cp -rf cpa.ko .mnt/2.6.28.4-gc395911-dirty/cpa.ko
	sudo cp mmaptest .mnt/2.6.28.4-gc395911-dirty/
	sudo cp ioctltest .mnt/2.6.28.4-gc395911-dirty/
	sudo umount .mnt
