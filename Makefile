obj-m := vtfs.o
vtfs-y := source/vtfs.o source/vtfs_init.o source/vtfs_ops.o source/vtfs_fops.o

PWD := $(CURDIR) 
KDIR = /lib/modules/`uname -r`/build
EXTRA_CFLAGS = -Wall -g

all:
	make -C $(KDIR) M=$(PWD) modules 

clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -rf .cache
