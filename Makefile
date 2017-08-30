obj-m := scull.o 
scull-objs := scull_main.o
EXTRA_CFLAGS := -DSCULL_DEBUG_TRACE -DSCULL_FUNC_TRACE

all:
		make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
		make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
