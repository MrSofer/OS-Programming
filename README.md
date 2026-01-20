# Linux Systems Programming & Kernel Modules

This repository contains a collection of low-level system tools and kernel modules implemented in **C**.
These projects were built to explore the Linux kernel API, process management, Inter-Process Communication (IPC), and concurrency.

## 📂 Project Overview

### 1. Custom Linux Shell (`/Shell`)
A custom command-line interpreter that interacts directly with the OS API.
* **Key Features:**
    * **Process Management:** Manages foreground/background processes using `fork()`, `execvp()`, and `waitpid()`.
    * **Piping & Redirection:** Implements multi-stage pipelines (`|`) and file redirection (`>`, `<`) by manipulating file descriptors (`dup2`, `pipe`).
    * **Signal Handling:** Custom handlers for `SIGINT` (Ctrl+C) to ensure shell stability while managing child processes.

### 2. Kernel Message Slot (`/Message_Slot`)
A character device driver loaded as a kernel module.
* **Key Features:**
    * Implemented `ioctl` interface for custom IPC.
    * Managed kernel memory and minor numbers to allow multiple message channels.

### 3. Client-Server Architecture (`/PCC`)
A Printer-Counting-Client server system.
* **Key Features:**
    * TCP/IP socket programming.
    * Multi-threaded server design to handle high-concurrency connections.

### 4. Concurrency & Synchronization (`/Mutex_Queue`)
* **Key Features:**
    * Thread-safe queue implementation using `pthread_mutex` and condition variables.

##
* **Languages:** C (C99 Standard)
* **OS Concepts:** Process Scheduling, Virtual Memory, Signals, Deadlocks, File Systems.
* **Tools:** GCC, GDB, Valgrind, Makefiles.
