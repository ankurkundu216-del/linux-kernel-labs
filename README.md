# 🐧 Linux Kernel Labs: A Beginner's Guide to the Engine Room

Welcome to **Linux Kernel Labs**! If you are a high school student, a beginner programmer, or just someone curious about how operating systems work, you are in the right place. 

This repository contains introductory C programs that run directly inside the Linux Kernel. 

## 🍔 The Restaurant Analogy: User Space vs. Kernel Space
To understand what this code does, imagine your computer is a restaurant:
* **User Space (The Dining Area):** This is where you usually hang out. It's safe. You order from a menu (using regular programs like your web browser or Python scripts). If you drop your fork, the restaurant doesn't burn down.
* **Kernel Space (The Kitchen):** This is where the actual cooking happens. It has sharp knives, hot stoves, and raw ingredients (memory, CPU, hardware). If you make a bad mistake here, the whole restaurant shuts down (a "Kernel Panic"). 

Normally, programmers stay in the dining area. The code in this repository takes you directly into the kitchen! We are writing **Kernel Modules**—pieces of code that can be dynamically loaded into the OS kernel while it's running.

---

## 📂 What's in this Repository?

We have three main experiments, increasing in complexity.

### 1. `hello.c` — Your First Kitchen Pass
This is the "Hello World" of kernel programming. 
* **The Concept:** Normal C programs start at a `main()` function. Kernel modules don't. Instead, they have two special doors: an **init** function (runs when the module is loaded) and an **exit** function (runs when it is removed).
* **`printk` vs `printf`:** We can't use standard `printf` because we don't have access to standard user terminal libraries in the kernel. Instead, we use `printk` (Print Kernel), which writes messages to the kernel's internal secret log book.

### 2. `hello_param.c` — Passing Notes to the Chef
What if we want to give our kernel module some instructions when we load it? 
* **The Concept:** This module introduces **Kernel Parameters**. We use a special macro called `module_param()` that allows us to pass variables (like an integer or a string) from the terminal directly into our kernel code at the exact moment we load it. 
* **Why it matters:** It proves that we can dynamically control kernel behavior from the outside without having to recompile the code every time.

### 3. `char_dev.c` — Building a Magic File
In Linux, there is a famous rule: *"Everything is a file."* Your keyboard, your mouse, and your hard drive are all represented as files in the `/dev` folder.
* **The Concept:** This code creates a **Character Device Driver**. It registers a brand new, fake hardware device (like `/dev/vbox_char`) in the system.
* **User/Kernel Bridge:** When a normal user tries to "read" or "write" text to this fake file from their terminal, the kernel intercepts that action. The code uses `copy_from_user` to safely pull text from the dining area into the kitchen, and `copy_to_user` to send data back out. 
* **Why it matters:** This is exactly how your operating system talks to real physical hardware!

---

## 🛠️ How to Build and Run the Code

To test these modules, you need a Linux environment (like Ubuntu) and a few tools. 

### Step 1: Install Prerequisites
Open your terminal and install the build tools and the map of the kitchen (kernel headers):
```bash
sudo apt update
sudo apt install build-essential linux-headers-$(uname -r)
```

### Step 2: Compile the Code

We use a Makefile to compile kernel modules. Just run:
```bash
make
```
This generates .ko (Kernel Object) files. These are your compiled modules ready to be injected!

### Step 3: Injecting and Testing
Let's use char_dev.ko as an example.

1. Load the module into the kernel:
```bash
sudo insmod char_dev.ko
```

2. Check the kernel's secret logbook to see if it worked:
```bash
sudo dmesg | tail -n 10
```

3. Test the Device Driver (Write and Read):
```bash
# Write a message into the kernel
echo "Hello from User Space!" | sudo tee /dev/vbox_char

# Read the message back from the kernel
sudo cat /dev/vbox_char
```

4. Unload the module and clean up:
```bash
sudo rmmod char_dev
make clean
```

### ⚠️ A Quick Warning

Because this code runs in Ring 0 (the deepest privilege level of your computer), a bad pointer or an infinite loop won't just crash your program—it will freeze your entire operating system. Always test kernel code inside a Virtual Machine (like VirtualBox) first!

Happy Hacking! 💻✨
