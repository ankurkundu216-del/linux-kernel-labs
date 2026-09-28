#include<linux/module.h>     // Reqd. for all kernel modules
#include<linux/kernel.h>     // Reqd. for printk and log levels
#include<linux/fs.h>         // Reqd. for character driver headers and file operations
#include<linux/uaccess.h>    // Reqd. for copy_to_user and copy_from_user
#include<linux/device.h>     // Reqd. for dynamically creating device nodes in /dev/


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ankur");
MODULE_DESCRIPTION("A begineer friendly character device driver");


#define DEVICE_NAME "vbox_char"   // Device name as it will appear in /dev/
#define CLASS_NAME "vbox_class"	  // Class name used for sysfs groupings

// Global variables for tracking device state
static int major_num;                                       // Store dynamically allocated device number
static struct class* char_class = NULL;			    // Device class pointer
static struct device* char_device = NULL;	            // Device driver structure pointer

// Internal Kernel Storage Buffer
static char message[256] = {0};                 // Array in kernel memory to store user string
static short message_size = 0;                  // Tracks the actual length of stored string

/*
 * Function 1: OPEN
 * Called whenever a user process opens the device node (e.g., fopen("/dev/vbox_char"))
 */
static int dev_open(struct inode *inodep, struct file *filep) {
	printk(KERN_INFO "vbox_char: Device opened by user process\n");
	return 0;
}

/*
 * Function 2: READ
 * Called when a user process reads from the device node (e.g., cat /dev/vbox_char)
 *
 * Parameters:
 * - filep: Pointer to the file structure
 * - buffer: Target user-space memory buffer where data will be delivered
 * - len: Maximum number of bytes requested by the user process
 * - offset: Current reading offset in the file
 */
static ssize_t dev_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset) {
	int uncopied_bytes = 0;

	// EOF Check: If the caller has already read from us, return 0 (End of File)
	// to prevent infinite read loops when using tools like 'cat'
	if (*offset > 0) {
		return 0;
	}

	// copy_to_user(destination_user_ptr, source_kernel_ptr, byte_count)
	// Returns the number of bytes that COULD NOT be copied (0 means complete success)
	uncopied_bytes = copy_to_user(buffer, message, message_size);

	if(uncopied_bytes == 0) {
		printk(KERN_INFO "vbox_char: Successfully sent %d bytes to user space\n", message_size);
		*offset = message_size;    // Advance offset to subsequent reads hit EOF
		return message_size;       // Return total bytes successfully read
	} else {
		printk(KERN_ALERT "vbox_char: Failed to copy %d bytes to user space\n", uncopied_bytes);
		return -EFAULT;            // Return "Bad address" system error code
	}
}

/*
 * Function 3: WRITE
 * Called when a user process writes data into the device node (e.g., echo "test" > /dev/vbox_char)
 *
 * Parameters:
 * - buffer: Source user-space memory buffer holding incoming data
 * - len: Number of bytes being written by the user process
 */
static ssize_t dev_write(struct file *filep, const char __user *buffer, size_t len, loff_t *offset) {
	// Prevent array overflow: limit copy length to buffer capacity minus 1 for null-terminator
	size_t bytes_to_copy = (len < 256) ? len : 255;

	// copy_from_user(destination_kernel_ptr, source_user_ptr, byte_count)
	if (copy_from_user(message, buffer, bytes_to_copy) != 0) {
		printk(KERN_ALERT "vbox_char: Failed to copy data from user space\n");
		return -EFAULT;
	}

	message[bytes_to_copy] = '\0';    // Manually null-terminate string
	message_size = bytes_to_copy;	  // Update active length

	printk(KERN_INFO "vbox_char: Received %zu bytes from user space\n", len);
	return len;     // Return total bytes successfully written
}

/*
 * Function 4: RELEASE / CLOSE
 * Called when the user process close the file handle
 */
static int dev_release(struct inode *inodep, struct file *filep) {
	printk(KERN_INFO "vbox_char: Device successfully closed\n");
	return 0;
}

/*
 * System Call Mapping Table
 * Link standard POSIX file operation hooks to our custom C functions
 */
static struct file_operations fops = {
	.open = dev_open,
        .read = dev_read,
	.write = dev_write,
	.release = dev_release,
};

/*
 * Module Initialization Function
 * Executed once upon 'insmod'
 */
static int __init char_init(void) {
	printk(KERN_INFO "vbox_char: Initializing character driver...\n");

	// 1. Dynamically request an available Major Number from the kernel
	// Setting parameter 1 to 0 tells kernel to auto-assign the number
	major_num = register_chrdev(0, DEVICE_NAME, &fops);
	if(major_num < 0) {
		printk(KERN_ALERT "vbox_char: Failed to register Major Number\n");
		return major_num;
	}

	// 2. Register device class under /sys/class/
	// (In Linux Kernels 6.4+, class_craete takes only 1 argument: the name)
	char_class = class_create(CLASS_NAME);
	if(IS_ERR(char_class)) {
		unregister_chrdev(major_num, DEVICE_NAME);
		return PTR_ERR(char_class);
	}

	// 3. Create the physical node at /dev/vbox_char
	char_device = device_create(char_class, NULL, MKDEV(major_num, 0), NULL, DEVICE_NAME);
	if(IS_ERR(char_device)) {
		class_destroy(char_class);
		unregister_chrdev(major_num, DEVICE_NAME);
		return PTR_ERR(char_device);
	}

	printk(KERN_INFO "vbox_char: Registered with Major Number %d. Node created at /dev/%s\n", major_num, DEVICE_NAME);
	return 0;
}

/*
 * Module Exit Function
 * Executed once uopn 'rmmod'
 */
static void __exit char_exit(void) {
	// Teardown steps must occur in exact reverse order of initialization
	device_destroy(char_class, MKDEV(major_num, 0)); // 1. Destroy /dev/ node
	class_destroy(char_class);			 // 2. Unregistered device class
	unregister_chrdev(major_num, DEVICE_NAME);	 // 3. Release Major Number back to kernel
	printk(KERN_INFO "vbox_char: Driver successfully removed from kernel space\n");
}

module_init(char_init);
module_exit(char_exit);
