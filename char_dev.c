#include<linux/module.h>     // Core header required for all kernel modules
#include<linux/kernel.h>     // Required for kernel logging (printk) and log levels (KERN_INFO)
#include<linux/fs.h>         // Required for the Virtual File System (VFS) and file_operations struct
#include<linux/uaccess.h>    // Required for the safe memory bridge: copy_to_user and copy_from_user
#include<linux/device.h>     // Required for dynamically creating physical device nodes in /dev/

/* 
 * BEGINNER NOTE: Kernel Modules vs Normal Programs
 * Normal C programs run in "User Space" and start at main().
 * Kernel modules run in "Kernel Space" (Ring 0). They don't have a main().
 * Instead, they load themselves into the operating system, register some functions,
 * and wait for the OS or a user to trigger them.
 */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ankur");
MODULE_DESCRIPTION("A begineer friendly character device driver");


#define DEVICE_NAME "vbox_char"   // The exact name of the file that will appear in the /dev/ folder
#define CLASS_NAME "vbox_class"	  // The category name used for grouping in sysfs (/sys/class/)

/* 
 * GLOBAL VARIABLES
 * We need these to remember the state of our device across different function calls.
 */
static int major_num;                                       // Store dynamically allocated device number
static struct class* char_class = NULL;			            // Device class pointer
static struct device* char_device = NULL;	                // Device driver structure pointer

/*
 * OUR HARDWARE SIMULATOR (Kernel Memory)
 * Since we aren't talking to real hardware (like a keyboard or mouse), 
 * we will use a simple character array to simulate a device's memory buffer.
 */
static char message[256] = {0};                 // Array in kernel memory to store user string
static short message_size = 0;                  // Tracks the actual length of stored string

/*
 * Function 1: OPEN
 * Triggered automatically when a user process opens the device node.(e.g., fopen("/dev/vbox_char"))
 * Example: Running `cat /dev/vbox_char` first calls this to open the door.
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
 *
 * BEGINNER NOTE ON SECURITY: 
 * The kernel is the most privileged part of the computer. It cannot trust User Space.
 * If a user provides a bad memory address, and the kernel tries to read it, the whole computer crashes (Kernel Panic).
 * We MUST use `copy_to_user()` to safely transfer data across the boundary.
 */
static ssize_t dev_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset) {
	int uncopied_bytes = 0;

	// EOF Check: If the caller has already read from us, return 0 (End of File)
	// to prevent infinite read loops when using tools like 'cat'
	if (*offset > 0) {
		return 0;
	}

	// copy_to_user(destination_user_ptr, source_kernel_ptr, byte_count)
	// Safely copy data from our kernel 'message' array to the 'buffer' provided by the user space program.
	// Returns the number of bytes that COULD NOT be copied (0 means complete success)
	uncopied_bytes = copy_to_user(buffer, message, message_size);

	if(uncopied_bytes == 0) {
		printk(KERN_INFO "vbox_char: Successfully sent %d bytes to user space\n", message_size);
		*offset = message_size;    // Advance offset to subsequent reads hit EOF(Move the internal file pointer forward so the next read knows we are done)
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
	// If the user tries to send 1000 bytes, we cap it at 255 so we have room for the '\0' terminator in our 256-byte array.
	size_t bytes_to_copy = (len < 256) ? len : 255;

	// Safely pull data from the untrusted User Space buffer into our secure Kernel Space 'message' array.
	// copy_from_user(destination_kernel_ptr, source_user_ptr, byte_count)
	if (copy_from_user(message, buffer, bytes_to_copy) != 0) {
		printk(KERN_ALERT "vbox_char: Failed to copy data from user space\n");
		return -EFAULT;
	}

	message[bytes_to_copy] = '\0';    // Manually append a null-terminator so it acts as a valid C string
	message_size = bytes_to_copy;	  // Save the length of the string for future reads

	printk(KERN_INFO "vbox_char: Received %zu bytes from user space\n", len);
	return len;     // Returning 'len' tells the user program "I successfully processed all the bytes you sent."
}

/*
 * Function 4: RELEASE / CLOSE
 * Triggered when the user process closes its connection to the file.
 */
static int dev_release(struct inode *inodep, struct file *filep) {
	printk(KERN_INFO "vbox_char: Device successfully closed\n");
	return 0;
}

/*
 * THE LOOKUP TABLE (File Operations)
 * This is how the Linux Virtual File System (VFS) knows which C function to run.
 * When a user types a command, VFS checks this table to map standard POSIX actions (open, read, write)
 * to the custom functions we just wrote above.
 */
static struct file_operations fops = {
	.open = dev_open,
    .read = dev_read,
	.write = dev_write,
	.release = dev_release,
};

/*
 * MODULE INITIALIZATION (The Entry Point)
 * Executed exactly once when you run `sudo insmod char_dev.ko`.
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
		unregister_chrdev(major_num, DEVICE_NAME); // Clean up the major number if this step fails!
		return PTR_ERR(char_class);
	}

	// 3. Create the physical node at /dev/vbox_char
	// This actually generates the magical file at /dev/vbox_char that the user will interact with.
	char_device = device_create(char_class, NULL, MKDEV(major_num, 0), NULL, DEVICE_NAME);
	if(IS_ERR(char_device)) {
		class_destroy(char_class);			// Clean up previous steps if this fails!
		unregister_chrdev(major_num, DEVICE_NAME);
		return PTR_ERR(char_device);		
	}

	printk(KERN_INFO "vbox_char: Registered with Major Number %d. Node created at /dev/%s\n", major_num, DEVICE_NAME);
	return 0;								// Initialization successful!
}

/*
 * MODULE EXIT (The Cleanup)
 * Executed exactly once when you run `sudo rmmod char_dev`.
 * BEGINNER NOTE: You MUST clean up your mess. If you don't release these resources,
 * the kernel will hold onto them forever (a memory leak), eventually crashing the system.
 * Cleanup must happen in the EXACT REVERSE order of initialization.
 */
static void __exit char_exit(void) {
	// Teardown steps must occur in exact reverse order of initialization
	// Step 3 reversed: Destroy the physical file in /dev/
	device_destroy(char_class, MKDEV(major_num, 0)); // 1. Destroy /dev/ node
	// Step 2 reversed: Destroy the category class
	class_destroy(char_class);			 // 2. Unregistered device class
	// Step 1 reversed: Give the Major Number back to the kernel so other drivers can use it
	unregister_chrdev(major_num, DEVICE_NAME);	 // 3. Release Major Number back to kernel
	printk(KERN_INFO "vbox_char: Driver successfully removed from kernel space\n");
}

module_init(char_init);
module_exit(char_exit);
