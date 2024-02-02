  - [Project 2: Non-Preemptive
    Scheduling](#project-2-non-preemptive-scheduling)
  - [Assignment](#assignment)
      - [Extra Challenges](#extra-challenges)
  - [Pre-code](#pre-code)
  - [Detailed Requirements and Hints](#detailed-requirements-and-hints)
  - [Design review](#design-review)
  - [Groups of two](#groups-of-two)
  - [Hand-in](#hand-in)
  - [Useful resources](#useful-resources)

# Project 2: Non-Preemptive Scheduling

INF-2201: Operating Systems Fundamentals  
UiT, The Arctic University of Norway

# Assignment

With the boot mechanism from the first project, we can start developing
an operating system kernel. The goal of this project is to design and
implement a simple multiprogramming kernel with a non-preemptive
scheduler. Although the kernel will be very primitive, you will be
dealing with several fundamental mechanisms and apply techniques that
are important for building operating systems supporting
multiprogramming.

The kernel of this project and future projects supports both processes
and threads. Note that the processes and threads in this project are a
bit different from those in traditional operating systems (like Unix).
All processes, threads and the kernel share a flat address space. The
main difference between processes and threads in this project is that a
process resides in a separate object file so that it cannot easily
address data in another process (and the kernel), whereas threads are
linked together with the kernel in a single object file. Letting each
process reside in its own object file provides it with some protection.
In the future, we will develop support for processes with separate
address spaces.

From now on, the kernel will use the protected mode of the x86
processors instead of the real mode. In this project the separation
between kernel mode and user mode is only logical, everything runs in
the processors highest privilege level (ring 0).

You are required to do the following:

1.  Initialize processes and threads, e.g. allocating stacks and setting
    up process control blocks (PCB).

2.  Implement a simple non-preemptive scheduler such that every time a
    process or thread calls yield or exit, the scheduler picks the next
    thread and schedules it. As discussed in the class, threads need to
    explicitly invoke one of these system calls in order to invoke the
    scheduler, otherwise a thread can run forever.

3.  Implement a simple system call mechanism. Since all code runs in the
    same address space and privilege mode, the system call mechanism can
    be a simple jump table. But the basic single-entry point system call
    framework should be in place. You are required to implement two
    system calls: yield and exit.

4.  Realize one synchronization primitives for mutual exclusion by
    implementing: lock\_acquire and lock\_release. Their semantics
    should be identical to that described in [Birrell’s
    paper](concurrency/programming-with-threads-andrew-birrell.pdf). For
    this the scheduler needs to support blocking and unblocking of
    threads. This should be implemented as the two functions block and
    unblock, but they are not system calls and should only be called
    from within the Kernel.

5.  Measure the overhead of context switches.

## Extra Challenges

These are by no means mandatory, but may prove helpful achieving top
grade in this course.

1.  To receive an extra credit, your implementation should keep track of
    user and system time used by each thread and process and print it to
    the screen (much like ‘time’ on a Unix system, when a process
    exits). The time should be be measuring in clock cycles using the
    `read_cpu_ticks` function that you also used to measure context
    switches.
2.  Add a fourth in-kernel thread to your OS
3.  Add a third user process to your OS

# Pre-code

New and important precode files:

  - ‘n’ indicates that the file is new in this project.
  - **bold** indicates that you will need to edit the file as part of
    the assignment.

| n | file                        | desc                                             |
| - | --------------------------- | ------------------------------------------------ |
| n | **kernel/asm\_common.h.S**  | Assembly macros common to scheduler and syscalls |
|   | **kernel/asm-offsets.c**    | Export constants from C to assembly              |
|   | **kernel/kernel.c**         | Kernel main                                      |
|   | kernel/kernel.h             |                                                  |
|   | kernel/lib/assertk.c        | In-kernel assertions library                     |
|   | kernel/lib/assertk.h        |                                                  |
|   | kernel/lib/printk.c         | In-kernel logging library                        |
|   | kernel/lib/printk.h         |                                                  |
|   | kernel/lib/todo.c           | In-kernel TODO macros                            |
|   | kernel/lib/todo.h           |                                                  |
| n | **kernel/pcb.c**            | The Process Control Block and operations on it   |
| n | **kernel/pcb.h**            |                                                  |
| n | **kernel/scheduler\_asm.S** | Low-level scheduler code written in asm          |
| n | **kernel/scheduler.c**      | Scheduler                                        |
| n | kernel/scheduler.h          |                                                  |
| n | **kernel/sync.c**           | Locks                                            |
| n | **kernel/sync.h**           |                                                  |
| n | **kernel/syscall\_asm.S**   | Low-level syscall dispatch code written in asm   |
| n | **kernel/syscall.c**        | Syscall dispatch                                 |
| n | kernel/syscall.h            |                                                  |
| n | kernel/th1.c                | Kernel thread: clock                             |
| n | kernel/th1.h                |                                                  |
| n | kernel/th2.c                | Kernel threads: lock test                        |
| n | kernel/th2.h                |                                                  |
| n | kernel/th3.c                | Kernel threads: Monte-Carlo pi calculator        |
| n | kernel/th3.h                |                                                  |
| n | lib/startfiles/crt0.S       | C Runtime Zero startup file for processes        |
|   | lib/syslib/addrs.h          | Important addresses                              |
|   | lib/syslib/common.h         | Syscall numbers and other important declarations |
|   | lib/syslib/screenpos.h      | Screen positions for thread/process output       |
| n | lib/syslib/syslib.c         | System call library for processes                |
| n | lib/syslib/syslib.h         |                                                  |
| n | process/process1.c          | Plane process                                    |
| n | process/process2.c          | Math process                                     |

# Detailed Requirements and Hints

You need to understand the PCB and figure out what you need to store in
it. The PCB provided only has fields for the previous and next process,
the rest is up to you. Since processes and threads share the same
address space in this project, the PCB can be very simple.

In the template file kernel.c, you need to implement the `kernel_main`
function to initialize the processes and threads you wish to run. For
example, you need to know at which location in memory the code for a
process or a thread starts and where the stack for that process or
thread is. Once the required information has been recorded in the
appropriate data structures you will need to start the first process or
thread. Note that a process is placed at a fixed location in memory.
This location is different for each process. You can figure this out by
looking in the Makefile.

The system call mechanism should use a single entry point as discussed
in the classes. The main difference is that you are not using the int
instruction to trap into the kernel, but simply make a normal procedure
call instead. In order to make this mechanism work, you need to
understand the stack frame and the calling convention.

The synchronization primitives are meant for in-kernel threads only and
need not be implemented as system calls. They need to deal with the PCB
data structures to block and unblock threads. An important step is to
figure out how to queue blocked threads. Although it is feasible to
implement the synchronization primitive in a way that would works with a
preemptive scheduler, it is not required in this project.

Finally, once all this is up and running you should measure the overhead
(time used) of context switches in your system. To do this you can use
the provided function `read_cpu_ticks` in `util.h`. This function
contains inline assembly that returns the number of clock cycles that
have passed since the processor was booted. Note that the value returned
is 64 bit (unsigned long long int). This function will only run on
Pentium or newer processors. Also, notice that exact measurements can
only be done when running on real hardware.

# Design review

For the design review, you need to have figured out all the details
including the PCB data structure, the system call mechanism, function
prototypes, how to switch from one thread to another and so on. In other
words, you should have a design that is ready to be implemented.

Before you come to meet the reviews, you should prepare a design
document. This document should contain detailed descriptions of the
various parts of your design. An oral only presentation is not
acceptable.

# Groups of two

You must work and hand-in in groups of two.

# Hand-in

This project is a mandatory assignment. It will be graded pass/fail by
TAs, and you must pass in order to gain admittance to the exam.

Hand-in will be via Canvas. Zip up your code repository and your report
and upload it to the Canvas page for the assignment.

You should hand in:

1.  A report, maximum 4 pages that gives overview of how you solved each
    task (including extra credits), how you have tested your code, and
    any known bugs or isseues. In addition you need to describe the
    methodology, results, and conclusions for your performance
    measurements.
    
      - The report should include your name and UiT e-mail.
      - The four page limit includes everything.
      - The report should be in PDF format.
      - The report should be in a `report/` directory in your
        repository.

2.  Your code repository. Zip up and hand in the entire repository
    (working tree + `.git/` directory), not just a snapshot of the code.

# Useful resources

  - [An Introduction to Programming with
    Threads](concurrency/programming-with-threads-andrew-birrell.pdf),
    Andrew D. Birrell

  - [Protected mode - a brief
    introduction](x86/protected-mode-slides-erlend-graff.pdf), Erlend
    Graff

  - [GCC Inline Assembly
    Howto](http://www.ibiblio.org/gferg/ldp/GCC-Inline-Assembly-HOWTO.html)
