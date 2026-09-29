#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio/driver.h> /* Kernel GPIO Controller API */

#define VIRT_GPIO_COUNT 8

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ankur Kundu");
MODULE_DESCRIPTION("Software Virtual GPIO Chip Driver");
MODULE_VERSION("1.0");

/* Virtual pin state storage (0 = LOW, 1 = HIGH) */
static int pin_states[VIRT_GPIO_COUNT] = {0};

/* Callback: Read pin state */
static int virt_gpio_get(struct gpio_chip *gc, unsigned int offset) {
    if (offset >= VIRT_GPIO_COUNT) return 0;
    return pin_states[offset];
}

/* Callback: Write pin state (Updated to return int) */
static int virt_gpio_set(struct gpio_chip *gc, unsigned int offset, int value) {
    if (offset < VIRT_GPIO_COUNT) {
        pin_states[offset] = value;
        pr_info("virt_gpio: Pin %u set to state %d\n", offset, value);
    }
    return 0;
}

static int virt_gpio_dir_in(struct gpio_chip *gc, unsigned int offset) {
    return 0;
}

static int virt_gpio_dir_out(struct gpio_chip *gc, unsigned int offset, int value) {
    return virt_gpio_set(gc, offset, value);
}

/* Kernel GPIO Chip Registration Structure */
static struct gpio_chip virt_chip = {
    .label            = "virtual-gpio-chip",
    .owner            = THIS_MODULE,
    .base             = -1, /* Dynamically assign base GPIO number */
    .ngpio            = VIRT_GPIO_COUNT,
    .get              = virt_gpio_get,
    .set              = virt_gpio_set,
    .direction_input  = virt_gpio_dir_in,
    .direction_output = virt_gpio_dir_out,
};

static int __init virt_gpio_init(void) {
    int ret;

    /* Register virtual GPIO controller with kernel */
    ret = gpiochip_add_data(&virt_chip, NULL);
    if (ret < 0) {
        pr_err("virt_gpio: Failed to register virtual GPIO chip (%d)\n", ret);
        return ret;
    }

    pr_info("virt_gpio: Successfully registered virtual chip with %d pins (Base ID: %d)\n", 
            VIRT_GPIO_COUNT, virt_chip.base);
    return 0;
}

static void __exit virt_gpio_exit(void) {
    gpiochip_remove(&virt_chip);
    pr_info("virt_gpio: Unregistered virtual GPIO chip\n");
}

module_init(virt_gpio_init);
module_exit(virt_gpio_exit);
