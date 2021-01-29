KDIR := /lib/modules/`uname -r`/build

CFLAGS_pebs_addr_profiler.o := -DTRACE_INCLUDE_PATH=${M}

obj-m := pebs_addr_profiler.o

all:
	make -C ${KDIR} M=`pwd`

install:
	make -C ${KDIR} M=`pwd` install

clean:
	make -C ${KDIR} M=`pwd` clean
