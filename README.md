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

### Required Packages

To build and run the OS, you need the GCC compiler toolchain, GCC 32-bit x86
libraries, and the Bochs x86 emulator.

On Ubuntu (or other `apt`-based distros), these are in the following packages:

| Purpose                             | Ubuntu package    |
|-------------------------------------|-------------------|
| required compilers and utilities    | `build-essential` |
| GCC 32-bit support                  | `gcc-multilib`    |
| x86 PC emulator that can run the OS | `bochs`           |

To install these packages on Ubuntu, run

    sudo apt install build-essential gcc-multilib bochs

Building the OS
--------------------------------------------------

The build is controlled by Make.
To build the OS, simply run Make with no arguments:

    make

Or, to run make in a Docker container, use the `docker-run` script:

    ./docker-run make

This produces a disk image (filename `image`) that can be booted in an emulator
or on some old PCs.

If you run into trouble, check the [build FAQ](doc/build/build-faq.md).

Running in the Bochs Emulator
--------------------------------------------------

The supported emulator is Bochs.

Bochs has an internal debugger and it can also be configured to work with
GDB. How you run the emulator depends on your configuration.

### Bochs with internal debugger (`apt` default)

Bochs with internal debugger is the default when Bochs is installed from
`apt`. The Makefile has targets for that:

```bash
# Run: Launch Bochs and immediately send a 'continue' command to the debugger
make run

# Debug: Build debug symbol files and launch Bochs with debugger active
make debug
```

### Bochs with GDB (requires reinstalling from source)

If you have reconfigured Bochs to use GDB, it is best to ignore the `make
run` and `make debug` targets and launch GDB in the way that works best for
your configuration. It might look something like this:

```bash
# Run: Build, then launch Bochs
make
bochs -q

# Debug: Build, launch Bochs with debugger active, and connect GDB
make
bochs -q "gdbstub: enabled=1"
# (in a separate terminal window)
gdb target/kernel/kernel -ex "target remote localhost:1234"
```

See [doc/tools/bochs.md](doc/tools/bochs.md) for more information.
