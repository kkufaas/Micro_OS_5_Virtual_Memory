# Project 3: Preemptive Scheduling

INF-2201: Operating Systems Fundamentals, \
Spring 2024, \
UiT The Arctic University of Norway

  - [Assignment](#assignment)
      - [Extra Challenges](#extra-challenges)
      - [Precode Files](#precode-files)
      - [Detailed Requirements and
        Hints](#detailed-requirements-and-hints)
  - [Report](#report)
  - [Hand-in](#hand-in)
  - [Useful Resources](#useful-resources)

# Assignment

The previous project, P2, was a simple but useful non-preemptive,
multi-threaded operating system kernel. The main goal of the present
project, P3, is to transform the non-preemptive kernel into a preemptive
kernel. You will also change the system call mechanism from a simple
function call to an interrupt based mechanism as used in contemporary
operating systems. In addition, you will deal with some concurrent
programming issues: you will add the synchronization primitives
semaphores, barriers, and condition variables, and you will work on the
famous dining philosophers problem.

The OS kernel of this project and future projects supports both
processes and kernel threads. See the description for P2 on how the
processes and threads in our project differ from the traditional notion
of processes and threads.

In this project, everything still runs in the CPU’s kernel mode with a
flat address space. Also note that the `delay` function has been
removed, and replaced with `ms_delay`, which waits the specified number
of milliseconds. Though be aware that this delay may not be accurate in
an emulator.

You are required to do the following:

1.  Develop an interrupt handler that invokes the scheduler. This
    interrupt will be triggered once every 10 ms by a timer.

2.  Although this kernel is preemptive, the `yield` (system) call should
    still work correctly.

3.  Use a software interrupt to perform system calls. All of this code
    is actually given to you as a template. The system calls (available
    to processes) that are already implemented are `yield`, `getpid` and
    `exit`. You are required to ensure that these system calls perform
    correctly even in the presence of the timer interrupt.

4.  Write new implementations for locks (`lock_acquire` and
    `lock_release`) that continue to work in the presence of preemptive
    scheduling.

5.  Implement more synchronization abstractions: semaphores, condition
    variables, and barriers. The precode includes new in-kernel threads
    that will test these.

6.  Improve the precode’s solution to the Dining Philosophers problem.
    This solution is not fair. What makes it unfair? How does this
    relate to the concept of *starvation*. How would you make it more
    fair? Tell us about this in your report. Update the existing
    solution to make it more fair.

## Extra Challenges

It is not really possible to give extra credit on a pass/fail
assignment, but attempting these extra challenges will boost your
chances of passing and give you a deeper understanding which may shine
through during the exam.

1.  Implement priority scheduling and demonstrate it with the processes
    provided by us (`process1`, the plane).
    
      - The plane process (process1.c) uses the new `setpriority`
        syscall
      - Implement priority scheduling: you should then see the plane’s
        speed change

2.  Re-implement dining philosophers on Linux using the pthreads
    library.
    
      - There is no precode for this
      - Set up a simple C project and write your own dining philosphers
        simulation

## Precode Files

New and important precode files:

  - ‘+’ indicates that the file is new in this project.
  - **bold** indicates that you will need to edit the file as part of
    the assignment.

| n  | file                           | desc                                            |
| -- | ------------------------------ | ----------------------------------------------- |
|    | kernel/asm\_common.h.S         | Assembly macros reused by assembly files        |
|    | kernel/asm-offsets.c           | Export constants from C to assembly             |
| \+ | kernel/barrier\_test.c         | In-kernel threads to test barriers              |
| \+ | kernel/barrier\_test.h         |                                                 |
| \+ | kernel/hardware/intctl\_8259.c | API for Programmable Interrupt Controller (PIC) |
| \+ | kernel/hardware/intctl\_8259.h |                                                 |
| \+ | kernel/hardware/keyboard.c     | API for PC keyboard                             |
| \+ | kernel/hardware/keyboard.h     |                                                 |
| \+ | kernel/hardware/pit\_8235.c    | API for Programmable Interval Timer (PIT)       |
| \+ | kernel/hardware/pit\_8235.h    |                                                 |
| \+ | **kernel/interrupt\_asm.S**    | Low-level interrupt handler code in assembly    |
| \+ | kernel/interrupt.c             | Higher-level interrupt code in C                |
| \+ | kernel/interrupt.h             |                                                 |
| \+ | **kernel/philosophers.c**      | The Dining Philosophers                         |
| \+ | kernel/philosophers.h          |                                                 |
|    | kernel/scheduler\_asm.S        | Low-level scheduler code in assembly            |
|    | **kernel/scheduler.c**         | Higher-level scheduler code in C                |
|    | kernel/scheduler.h             |                                                 |
|    | **kernel/sync.c**              | Locks and sync abstractions                     |
|    | **kernel/sync.h**              |                                                 |
|    | kernel/syscall\_asm.S          | Low-level system call handler code in assembly  |
|    | kernel/syscall.c               | Higher-level system call handler code in C      |
|    | kernel/syscall.h               |                                                 |
| \+ | kernel/time.c                  | Timer calibration (buggy)                       |
| \+ | kernel/time.h                  |                                                 |

## Detailed Requirements and Hints

This project continues using the protected-mode of the CPU and runs all
code in the kernel-mode flat address space. To implement preemptive
scheduling, you need to use the timer interrupt mechanism. We have
prepared for you most of the code that is required to set up the
interrupt descriptor tables (IDT) and set up the timer chip
(Programmable Interval Timer - PIT). You need to understand how it is
done so that you can adjust the interrupt frequency. We have provided
you with an interrupt handler template. It is your job to decide what
goes into the interrupt handler.

You need to figure out where you can perform preemptive scheduling. You
need to review the techniques discussed in class. Note that in this
project, you can disable interrupts, but you should avoid disabling
interrupts for a long period of time (or indefinitely\!). In order for
processes to make system calls, you will note that the code in syslib.c
does not make a function call, rather it invokes a software interrupt.
The kernel threads should still work in the same way as before, calling
the functions in the kernel directly.

Implement the synchronization abstractions in the files `sync.h` and
`sync.c`, specifically the `_nointerrupt` version of each function.
Everything should work correctly in the presence of preemptive
scheduling. You need to review the techniques discussed in class. Note
that you can use less complex primitives to implement more complex ones.

To implement a fair solution to the dining philosophers, modify
`philosophers.c`. Document why your solution is fair. The Dining
Philosophers problem is a classic problem in synchronization and there
are many solutions to be found if you search for them. If you base your
solution on an existing solution, be sure to document it. The Dining
Philosophers are usually implemented using semaphores, but you are not
limited to semaphores. You may use locks, condition variables, barriers,
anything you like, if you think it will lead to a better solution.

# Report

You must write a short technical report about your work, to be handed in
with your code. The report should give an overview of how you solved
each task (including extra challenges), how you have tested your code,
and any known bugs or issues. In addition you need to describe the
methodology, results, and conclusions for your performance measurements.

  - The report should follow the format of a scientific paper:
    Introduction, Methods, Results, and Discussion
    ([IMRaD](https://en.wikipedia.org/wiki/IMRAD)).
  - The report should be around four pages long.
  - The report should be in PDF format.

We discourage the use of ChatGPT or other AI tools for your report. We
prefer that you practice writing for yourselves. But we do not ban these
tools. If you do use ChatGPT or some other AI tool, we expect you to
declare that in your report and explain how you used it.

# Hand-in

This project is a mandatory assignment. It will be graded pass/fail by
TAs, and you must pass in order to gain admittance to the exam.

Hand-in will be via Canvas. Zip up your code repository and your report
and upload it to the Canvas page for the assignment.

You should hand in:

1.  Your report. Place the report PDF in a subdirectory of your
    repository called `report/`

2.  Your code repository. Zip up and hand in the entire repository
    (working tree + `.git/` directory), not just a snapshot of the code.

# Useful Resources

  - x86 interrupt handling:
    
      - [Intel® 64 and IA-32 Architectures Software Developer
        Manuals](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html),
        especially Volume 3, System Programming Guide, Chapter 6,
        Interrupt and Exception Handling
    
      - [AMD64 Architecture Programmer’s
        Manuals](https://www.amd.com/en/search/documentation/hub.html),
        especially Volume 2, System Programming, Chapter 8, Exceptions
        and Interrupts

  - PC interrupt and timer chips:
    
      - [OSDev Wiki: 8259 PIC](https://wiki.osdev.org/PIC): overview of
        the Programmable Interrupt Controller, which communicates
        external hardware Interrupt Requests (IRQs) to the CPU
    
      - [OSDev Wiki: Programmable Interval
        Timer](https://wiki.osdev.org/Pit): overview of the timer that
        raises the IRQ we use for preemption

  - Assembly language:
    
      - [A Guide to Programming Pentium/Pentium Pro
        Processors](doc/x86/pentium-programming-kai-lee.pdf): A good
        introduction to get you started (or refreshed) on assembler.
        Covers both Intel and AT\&T syntax.
    
      - [x86 Assembly
        Guide](https://flint.cs.yale.edu/cs421/papers/x86-asm/asm.html):
        Basics of 32-bit x86 assembly language programming.
    
      - [GCC-Inline-Assembly-HOWTO](http://www.ibiblio.org/gferg/ldp/GCC-Inline-Assembly-HOWTO.html):
        A tutorial on GCC inline assembly
    
      - [GCC Manual: Extended
        Asm](https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html):
        Official documentation for GCC’s extended assembly syntax

  - POSIX threads library (pthreads):
    
      - [POSIX Threads
        Programming](https://hpc-tutorials.llnl.gov/posix/): a tutorial
        on pthreads
    
      - [pthreads(7) man
        page](https://man7.org/linux/man-pages/man7/pthreads.7.html)
