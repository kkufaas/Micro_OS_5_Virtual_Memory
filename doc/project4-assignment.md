# Project 4: Inter-Process Communication

INF-2201: Operating Systems Fundamentals, \
Spring 2024, \
UiT The Arctic University of Norway

  - [Assignment](#assignment)
      - [Precode Files](#precode-files)
      - [“Given” Files](#given-files)
      - [Message passing](#message-passing)
      - [Process management](#process-management)
      - [Extra challenges](#extra-challenges)
          - [Go even further](#go-even-further)
  - [Detailed Requirements and Hints](#detailed-requirements-and-hints)
  - [Report](#report)
  - [Hand-in](#hand-in)

# Assignment

This project consists of two separate parts: message passing mechanism,
and process management.

In this project, we introduce the user mode: processes have their own
address spaces, and are locked from accessing kernel memory. For this,
the OS employs the segmenting and paging functionalities Intel provides
with its CPUs, and the CPUs protection features. You do not need to know
understand memory segmenting and paging yet, but you may browse the
source code since the next assignment will deal with Virtual Memory.

## Precode Files

New and important precode files:

  - ‘+’ indicates that the file is new in this project.
  - **bold** indicates that you will need to edit the file as part of
    the assignment.

| \+ | file                  | desc                           |
| -- | --------------------- | ------------------------------ |
| \+ | **kernel/keyboard.c** | Keyboard drivers               |
| \+ | kernel/keyboard.h     |                                |
| \+ | **kernel/mbox.c**     | IPC message passing            |
| \+ | kernel/mbox.h         |                                |
| \+ | kernel/memory.c       | Simple virtual memory          |
| \+ | kernel/memory.h       |                                |
|    | **kernel/pcb.c**      | Tasks: Processes and threads   |
|    | kernel/pcb.h          |                                |
| \+ | kernel/sleep.c        | Support for processes to sleep |
| \+ | kernel/sleep.h        |                                |
|    | **kernel/sync.c**     | Sync abstractions (locks, etc) |
|    | kernel/sync.h         |                                |
| \+ | kernel/usb/           | US drivers                     |
|    | process/process1.c    |                                |
|    | process/process2.c    |                                |
| \+ | process/process3.c    | IPC test process: producer     |
| \+ | process/process4.c    | IPC test process: consumer     |
| \+ | process/shell.c       | Interactive shell              |

## “Given” Files

Notice that only four files are *required* to be changed in this project
(you may of course change more, but four are part of the core
requirements). The precode also includes compiled binaries of working
solutions for each of these.

| Source file           | Normal object file       | Given object file              |
| --------------------- | ------------------------ | ------------------------------ |
| src/kernel/mbox.c     | target/kernel/mbox.o     | target/kernel/mbox.given.o     |
| src/kernel/keyboard.c | target/kernel/keyboard.o | target/kernel/keyboard.given.o |
| src/kernel/sync.c     | target/kernel/sync.o     | target/kernel/sync.given.o     |
| src/kernel/pcb.c      | target/kernel/pcb.o      | target/kernel/pcb.given.o      |

These files allow you to focus on one file at a time by developing and
testing it against a known-working version of the other files.

A section of code in `src/Makefile.common` includes lines that
substitute a given file into the list of kernel objects:

``` makefile
### "given" precompiled object files
#
# Given files are compiled, completed object files that in-progress work
# can be tested against. Uncomment these line in different combinations
# to use different combinations of given and in-progress code.
KERNEL_OBJS := $(KERNEL_OBJS:kernel/mbox.o=kernel/mbox.given.o)
KERNEL_OBJS := $(KERNEL_OBJS:kernel/keyboard.o=kernel/keyboard.given.o)
KERNEL_OBJS := $(KERNEL_OBJS:kernel/sync.o=kernel/sync.given.o)
KERNEL_OBJS := $(KERNEL_OBJS:kernel/pcb.o=kernel/pcb.given.o)
```

These lines start commented out, but you can uncomment one or more of
them to use any combination of your work and given files.

## Message passing

Message passing allows processes to communicate with each other by
sending and receiving messages. We will add support for the keyboard
using message passing.

You are required to do the following for this part of the assignment:

Implement a **mailbox** mechanism as the message passing mechanism. You
will have to implement 6 functions, of which 5 are system calls. The
syntax of these primitives can be found in the header file `mbox.h`.

| Function name | Description                                                                              |
| ------------- | ---------------------------------------------------------------------------------------- |
| mbox\_init    | Initializes the data structures of all mailboxes in the kernel                           |
| mbox\_open    | Opens a mailbox for either mbox\_send or mbox\_recv                                      |
| mbox\_close   | Closes a mailbox. When no process uses the mailbox, its data structure will be reclaimed |
| mbox\_send    | Sends a message to the mailbox                                                           |
| mbox\_recv    | Receives a message from a mailbox                                                        |
| mbox\_stat    | Returns the number of messages in the mailbox                                            |

Implement 3 functions to support **keyboard input** (in *keyboard.c*)

| Function name  | Description                                                                        |
| -------------- | ---------------------------------------------------------------------------------- |
| keyboard\_init | Initializes the data structures for managing keyboard input                        |
| getchar        | Gets the next keyboard character                                                   |
| putchar        | Called by the keyboard interrupt handler to put a character in the keyboard buffer |

We want interrupts to be disabled as infrequently as possible, and this
project requires you to **improve the implementation of mutexes and
condition variables** to achieve this. You need to add code to the file
`sync.c` only.

## Process management

This will make our OS feel more like a real one. We will add some
preliminary process management into the kernel including dynamic process
loading which will enable you to actually run a program from any storage
device. Also, for those who always have energy and time, there are
plenty of possibilities to do more in this part.

To load a program dynamically from a storage device, we will have to
first agree upon a really simple format for a file system. Our file
system is flat and of limited size. It’s flat, because there is no
directory structures. All files reside in the same root directory. It’s
of limited size because only one sector will be used as the root
directory, and therefore only a limited number of entries can be stored
there. A more detailed structure of this simple file system is given in
the section below.

We want to be able to tell the OS to “load” a program from a program
disk. The kernel will then read the root directory sector of the disk
and find the offset and length of the program, read it into a memory
area which is not used, and then allocate a new PCB entry for it. You
need to allocate memory for the process, but you do not need to worry
about the deallocation of memory when a process exits. (If you have the
time and energy to do more you can actually deallocate all the memories
used by a process when it exits.)

You will need to implement the following:

| Function name | Description                                            |
| ------------- | ------------------------------------------------------ |
| readdir       | Reads the program directory sector into a given buffer |
| loadproc      | Load a program and create a PCB entry for it           |

## Extra challenges

It is not really possible to give extra credit on a pass/fail
assignment, but attempting these extra challenges will boost your
chances of passing and give you a deeper understanding which may shine
through during the exam.

1.  **Memory reclamation**: After a process exits or gets killed (if you
    implement kill, see below), you should free all the memory the
    process uses, and the PCB entry as well, so that the next newly
    started process can reuse this memory. You can use any simple (and
    good) management algorithm, such as First Fit or Best Fit. Also, you
    can have a fixed-size PCB table in your kernel, so that the user can
    create no more than this number of processes. Note: You must
    implement your own memory manager and a malloc-like and a free-like
    functions. You can’t use the standard `malloc` function in C
    library, because we are not linking against any standard libraries.

2.  **Better memory management**: You can allocate each PCB entry
    dynamically, so that the only limit on the number of processes is
    the available memory.

3.  **`ps` command**: Implement a UNIX-like `ps` command, so that the
    user can type `ps` in the shell and view all the processes along
    with their pids, statuses, and/or any other information you can come
    up with.

4.  **`kill` command**: Implement a UNIX-like kill command, so that the
    user can type `kill` in the shell and kill a process specified by
    its pid. Note: It’s okay to not be able to kill a thread, because
    our threads are linked statically with the kernel. You should take
    care of locks and condition variables that a process being killed
    owns. You don’t really want to kill a process that already acquires
    10 locks. To solve this problem, you might use “delayed killing”.
    When you kill a process you only set its status to KILLED, and the
    scheduler will allow it continue to run until you are sure that all
    the locks it owns are release by it. A little hint: the only way
    that a process can access a lock is by calling mbox system calls.

5.  **Suspend/Resume**: Be able to suspend and resume a process.

### Go even further

  - You have keyboard input now. You could write simple game with ASCII
    graphics. Classic games like Snake, Breakout, or Tetris are good
    fits for this.

  - You can also try switching to VGA graphics mode and draw actual
    graphics.

  - Or you could write a more advanced shell.

# Detailed Requirements and Hints

To minimize your effort of debugging, we strongly suggest you first
finish the **message passing** part before dealing with **process
management**, because they are independent.

To **minimize interrupt disabling** in the implementation of mutexes and
condition variables, you should use the spinlock primitives provided to
perform atomic operations. Only when you make a call to a scheduler
function (like block and unblock) should you turn the interrupts off.

You will be implementing a simple **mailbox mechanism** that supports
variable sized messages. All messages will be delivered in FIFO order.
So, the implementation of the mailbox mechanism essentially involves
solving the bounded buffer problem discussed in the classes. You can use
a Mesa-style monitor to implement the bounded buffer.

Adding **keyboard support** requires writing an interrupt handler and a
`getchar()` system call. You can treat the interrupt handler as the
producer and the processes calling `getchar()` as the consumers. The
interrupt handler simply turns an input character into a message and
places it into the proper mailbox, as discussed in the class. The
`getchar()` system call can call `mbox_recv()` from the mailbox to read
the input character.

The **interrupt handler** needs to handle several interrupts because you
have multiple types of interrupts (timer and keyboard, for example). The
code in `interrupt.c` takes care of this, but you should read the code
carefully and understand what it is doing.

At this point, you should be able to run your OS. Everything should be
running, and running smoothly. For instance, the plane animation should
fire a bullet when you type “fire” in your shell while the other threads
and processes continue to have progress.

Now you can start doing **process management**. In the simple file
system one block is used to hold the process directory. This directory
contains one entry per process. Each entry records the offset and size
of a process. See the precept slides for a diagram of the new, modified
disk image structure.

To make your life a little bit easier, we will give you the solution
object files of those four source files that you will be modifying,
namely, `mbox.o`, `keyboard.o`, `sync.o`, and `kernel.o`. You can use
these given object files to test your program incrementally.

# Report

You must write a short technical report about your work, to be handed in
with your code. The report should give an overview of how you solved
each task (including extra challenges), how you have tested your code,
and any known bugs or issues. In addition you need to describe the
methodology, results, and conclusions for your performance measurements.

See the [How to Write a Report](how-to-write-a-report.md) document in
this directory for more detailed guidelines and advice.

# Hand-in

This project is a mandatory assignment. It will be graded pass/fail by
TAs, and you must pass in order to gain admittance to the exam.

Hand-in will be via Canvas. You should hand in:

1.  Your report. Place the report PDF in a subdirectory of your
    repository called `report/`

2.  Your code repository. Zip up and hand in the entire repository
    (working tree + `.git/` directory), not just a snapshot of the code.

3.  Also upload a second, separate copy of your report PDF. Having the
    report both in the zip and stand-alone on Canvas makes it easier for
    us to grade.
