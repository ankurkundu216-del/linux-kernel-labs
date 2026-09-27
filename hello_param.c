#include<linux/init.h>
#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/moduleparam.h>  // Reqd. for passing parameters


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A parameterized Hello World kernel module");

// Define the variables with default values
static char *whom = "Ring 0";
static int howmany = 1;

// Register the variables as module parameters
// Format: module_param(name, type, permissions)
// charp = character pointer. 0644 exposes the parameter in sysfs with read/write access for root.
module_param(whom, charp, 0644);
MODULE_PARM_DESC(whom, "A string representing whom to greet");

module_param(howmany, int, 0644);
MODULE_PARM_DESC(howmany, "An integer for the number of greetings");

static int __init hello_param_init(void) {
	int i;
	for(i=0; i < howmany; i++) {
		printk(KERN_INFO "Hello, %s! (Greetings %d of %d)\n", whom, i+1, howmany);
	}
	return 0;
}

static void __exit hello_param_exit(void) {
	printk(KERN_INFO "Goodbye, %s! Parameter module unloaded.\n", whom);
}

module_init(hello_param_init);
module_exit(hello_param_exit);
