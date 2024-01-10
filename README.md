Operating Systems Precode
==================================================

INF-2201 Operating Systems \
UiT The Arctic University of Norway

This repository contains skeleton code (aka "precode") for a simple operating
system. It is the basis for assignments in the Operating Systems course at UiT
The Arctic University of Norway.

For information about the assignments, see the appropriate project documents in
the `doc/` directory.

Repository Map
==================================================

```
.
├── src                 Source code
│   ├── boot                Bootloader code
│   ├── kernel              Kernel code
│   │   ├── hardware             Hardware abstractions
│   │   ├── lib                  Libraries used in the kernel
│   │   └── usb                  USB drivers
│   ├── process             Processes that run on our OS
│   └── lib                 Shared library code
│       ├── ansi_term           Terminal output to VGA memory
│       ├── libc                C Standard Library (subset) for our OS
│       ├── startfiles          C runtime for our OS
│       ├── syslib              System call library for our OS
│       ├── unittest            Simple C unit-testing framework
│       └── util                Other miscellaneous utilities
├── thirdparty          BIOS images and other files needed to run the OS
├── host                Output dir for code to run on the build host machine
└── target              Output dir for code to run on the OS target machine
```

Building and Running the OS
==================================================

System Requirements
--------------------------------------------------

Quick summary:

- The operating system itself is for an x86-32 (aka i386) target architecture.
- The build process expects to run on an x86-64 (aka amd64) Linux host machine.
- The preferred Linux distro is Ubuntu 22.04 LTS (Jammy Jellyfish).
- Other distros or Windows Subsystem for Linux (WSL) may work,
    but they are not officially supported.
- There is also a Dockerfile to set up a container for building the OS.

### Ubuntu

Required packages:

| Package           | Purpose                             |
|-------------------|-------------------------------------|
| `build-essential` | required compilers and utilities    |
| `gcc-multilib`    | GCC 32-bit support                  |
| `bochs`           | x86 PC emulator that can run the OS |

To install these packages on Ubuntu, run

    sudo apt install build-essential gcc-multilib bochs

Building and Running the OS
--------------------------------------------------

The build is controlled by Make.
To build the OS, simply run Make with no arguments:

    make

Or, to run make in a Docker container, use the `docker-run` script:

    docker-run make

This produces a disk image (filename `image`) that can be booted in an emulator
or on some old PCs.

To run in the Bochs emulator:

    make run

To run in the Bochs emulator's debugger:

    make debug
