#include<linux/init.h>		// Reqd. for initialization and exit macros
#include<linux/module.h>	// Reqd. for all kernel modules
#include<linux/kernel.h>	// Reqd. for KERN_INFO and printk


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple Hello World kernel module");


// The initialization function runs when the module is loaded
static int __init hello_init(void) {
	// printk is the kernel's version of printf
	// KERN_INFO dictates the log level
	printk(KERN_INFO "Hello, Ring 0! Module loaded successfully.\n");
	return 0; // A non-zero return means initialization failed
}

// The exit function runs when the module is removed
static void __exit hello_exit(void) {
	printk(KERN_INFO "Goodbye, Ring 0! Module unloaded.\n");
}

// Registering the init and exit functions
module_init(hello_init);
module_exit(hello_exit);
