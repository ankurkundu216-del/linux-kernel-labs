# This tells Kbuild to compile hello.o into a module (hello.ko)

obj-m += hello.o hello_param.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
