obj-m := scull.o 
scull-objs := scull_main.o
EXTRA_CFLAGS := -USCULL_DEBUG_TRACE -USCULL_FUNC_TRACE

all:
		make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
		make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
