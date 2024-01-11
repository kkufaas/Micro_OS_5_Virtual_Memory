  - [Bochs](#bochs)
      - [Installation](#installation)
          - [Built-in debugger](#built-in-debugger)
          - [GDB](#gdb)
      - [Run Configuration](#run-configuration)
  - [Running Bochs](#running-bochs)
  - [Debugging Examples](#debugging-examples)
      - [Bochs’s Built-in Debugger](#bochss-built-in-debugger)
          - [Loading symbols and breaking on
            functions](#loading-symbols-and-breaking-on-functions)
          - [Displaying data](#displaying-data)
          - [Break on general protection fault and examine the
            stack](#break-on-general-protection-fault-and-examine-the-stack)
      - [GDB debug](#gdb-debug)
          - [GDB init file](#gdb-init-file)
          - [16-bit debugging](#bit-debugging)
      - [Known-Good Bochs and BIOS
        Versions](#known-good-bochs-and-bios-versions)

# Bochs

For this course we will often use the Bochs emulator to test and run our
kernel. This document goes through the installation process,
configuration and how to run your kernel. You will also find some
examples for using either Bochs’ built-in debugger or instructions how
to attach GDB.

## Installation

Bochs binary package does not have gdb enabled by default. To enable
gdb-support see [GDB](#gdb), for built-in debugger see
[built-in](#built-in-debugger).

**Ubuntu**

`sudo apt install bochs-x`

**From source**

`Dependencies:`

Bochs uses C++ for some of their build process, so when compiling from
source we need a C++ compiler. We recommend the GNU C++ compiler: `g++`
in apt. We will be compiling for the X.org version of bochs (default) so
we need some X.org development libraries. We recommend the dev libs from
xorg: `xorg-dev` in apt. If your Linux distribution uses X11 instead of
X.org (e.g. Linux Mint), install `libx11-dev` instead.

In the case that this crashes your system, you can try using the SDL
version instead, see [Bochs
compiling](https://bochs.sourceforge.io/doc/docbook/user/compiling.html)
for alternatives.

If you want debugging enabled, change the configuration flags as shown
in [Built-in debugger](#built-in-debugger) or [GDB](#gdb).

1.  Download source: `wget
    https://sourceforge.net/projects/bochs/files/bochs/2.7/bochs-2.7.tar.gz/download
    -O bochs.tar.gz`
2.  Unpack the archive: `tar -xzvf bochs.tar.gz`
3.  Move into the Bochs directory: `cd bochs-2.7`
4.  Run the configuration script: `./configure --disable-docbook
    --enable-usb`
      - Disable docbook, it’s online on their website and github.
      - USB-forwarding
      - If you want to install bochs in a different directory you can
        use `--prefix=<path>`
5.  Compile: `make`
6.  Install (requires privileges): `sudo make install`

For Ubuntu:

    sudo apt update && sudo apt -y upgrade && sudo apt -y autoremove && sudo apt install -y xorg-dev && sudo apt install -y g++ && \
    git clone git@github.com:bochs-emu/Bochs.git && \
    cd Bochs/bochs && \
    git checkout REL_2_7_FINAL && \
    ./configure --disable-docbook --enable-usb && \
    make && \
    sudo make install && \
    cd - && \
    rm -rf Bochs

### Built-in debugger

The binaries downloads are not compiled with debugging options enabled
so for this you will have to compile Bochs from the source code but
change the configuration to:

`./configure --enable-debugger --disable-docbook --enable-usb`

### GDB

Bochs can be configured to use GDB as a remote stub to debug your
kernel. To enable gdb please change the configuration when compiling to:

`./configure --enable-gdb-stub --disable-docbook --enable-usb`

## Run Configuration

Config options
<https://bochs.sourceforge.io/doc/docbook/user/bochsrc.html>

Using the debugger
<https://bochs.sourceforge.io/doc/docbook/user/internal-debugger.html>

Using Bochs with GDB
<https://bochs.sourceforge.io/doc/docbook/user/debugging-with-gdb.html>

# Running Bochs

All run commands for Bochs are defined in `bochsrc` files we include in
the projects. Standing in your project’s root folder you should be able
to boot like:

`bochs -q`

\-q option for skipping the start menu after the configuration file is
loaded

**Debugger**

If you are running a debugger it will by default break upon boot, you
can continue emulation by typing `c` in your terminal.

If you have enabled GDB attachment you will now have to connect GDB at
your specified port, see: [GDB debug](#gdb-debug) or
<https://bochs.sourceforge.io/doc/docbook/user/debugging-with-gdb.html>
for more information.

# Debugging Examples

We tend to make mistakes when we write code, and operating systems
programming is no exception (rather the contrary). Often bugs we
introduce can be really hard to find, and this is where debuggers come
in handy\! For this course, we can support you in either using the
built-in Bochs debugger or attaching GDB to Bochs.

## Bochs’s Built-in Debugger

The Bochs built-in debugger gives you full control of the emulator and
execution of your code, which can be extremely useful. It resembles what
you are used to from GDB, with a slightly more ‘clunky’ feel.

  - Bochs Debugger documentation:
    <https://bochs.sourceforge.io/doc/docbook/user/internal-debugger.html>
  - You can also type `help` at the debugger prompt

### Loading symbols and breaking on functions

Bochs can load symbols from files. The files have a simple file format
with one symbol per line:

    00001000 _start16
    000015c7 _start32
    00001003 kernel_main

Our build system can generate these symbol files for you:

``` bash
make bochssyms      # Just generate symbol files
make bochsdebug     # Generate symbol files and launch debugger

# These will generate...
#   target/boot/bootblock.sym   for target/boot/bootblock
#   target/kernel/kernel.sym    for target/kernel/kernel
#   ...and so on with any processes that are active for the current project
```

At the Bochs debugger command line, you can load the symbol files with
the `ldsym` command:

    <bochs:1> ldsym "target/kernel/kernel.sym"
    <bochs:2> lbreak "_start16"
    <bochs:3> lbreak "_start32"
    <bochs:4> continue

### Displaying data

    # Examine (x) physical (p) memory:
    #   8 (/8) 32-bit words (w) in hex (x) starting at symbol "gdt"
    <bochs:3> xp /8wx "gdt"
    [bochs]:
    0x0000000000017c40 <gdt+0>:     0x00000000      0x00000000      0x0000ffff      0x00cf9a00
    0x0000000000017c50 <gdt+20>:    0x0000ffff      0x00cf9200      0x0000ffff      0x00cffa00
    
    # Examine (x) physical (p) memory:
    #   4 (/4) 64-bit "giant words" (g) in hex (x) starting at symbol "gdt"
    <bochs:4> xp /4gx "gdt"
    [bochs]:
    0x0000000000017c40 <gdt+0>:     0x00000000      0xcf9a000000ffff
    0x0000000000017c50 <gdt+20>:    0xcf92000000ffff        0xcffa000000ffff

### Break on general protection fault and examine the stack

    ldsym 'target/bootblock.sym'
    ldsym 'target/kernel.sym'
    ldsym 'target/process/process1.sym'
    ldsym 'target/process/process2.sym'
    show all
    break 'exception_13'
    continue
    # --- run until break on exception ---
    x /6wx esp      # E(x)amine 6 32-bit (w)ords starting from the value of ESP
    
    # --- Bochs output ---
    #[bochs]:                            #   Error code      EIP             CS              EFLAGS
    #0x000000000004bfe4 <_end+348a0>:        0x00000000      0x0100130f      0x0000001b      0x00010202
    #0x000000000004bff4 <_end+348c0>:        0x0100538c      0x00000023
    #                                    #   ESP             SS
    
    print-stack
    
    #Stack address size 4
    # | STACK 0x0004b2a8 [0x00000000] (BIOS_INT_TELETYPE_BH+0)  # error code
    # | STACK 0x0004b2ac [0x0000d701] (parse_spec+13)           # EIP
    # | STACK 0x0004b2b0 [0x00000008] (BIOS_DISK_GEOM_AH+0)     # CS
    # | STACK 0x0004b2b4 [0x00010002] (gdt_desc+2)              # EFLAGS
    # | STACK 0x0004b2b8 [0x0000e858] (__func__.0+18)           # ESP (if switch)
    # | STACK 0x0004b2bc [0x00000000] (BIOS_INT_TELETYPE_BH+0)  # SS  (if switch)
    # | STACK 0x0004b2c0 [0x00000001] (BIOS_INT_SET_CURSOR_AH+0)
    # | STACK 0x0004b2c4 [0x0000df03] (vfprintf+20)
    # | STACK 0x0004b2c8 [0x00000000] (BIOS_INT_TELETYPE_BH+0)
    # | STACK 0x0004b2cc [0x00000000] (BIOS_INT_TELETYPE_BH+0)
    # | STACK 0x0004b2d0 [0x00000000] (BIOS_INT_TELETYPE_BH+0)
    # | STACK 0x0004b2d4 [0x00000000] (BIOS_INT_TELETYPE_BH+0)
    # | STACK 0x0004b2d8 [0x00000000] (BIOS_INT_TELETYPE_BH+0)
    # | STACK 0x0004b2dc [0x00000000] (BIOS_INT_TELETYPE_BH+0)
    # | STACK 0x0004b2e0 [0x00000000] (BIOS_INT_TELETYPE_BH+0)
    # | STACK 0x0004b2e4 [0x00000000] (BIOS_INT_TELETYPE_BH+0)

## GDB debug

GDB requires your kernel to contain debug information, make sure you are
compiling using -g/-Og flags, this should be on by default.

1.  Uncomment the last line in your bochsrc file `gdbstub: enabled=1,
    port=1234, text_base=0, data_base=0, bss_base=0`
    
    Or, to enable the GDB stub for just a single invokation of Bochs,
    add the enable command to Bochs’s command line:
    
    ``` 
     bochs -q "gdbstub: enabled=1"
    ```

2.  Compile your kernel with debug flags and debug optimization levels.

3.  Run Bochs, i.e. `bochs -q`
    
    You should see a black Bochs window and a terminal output saying
    waiting for GDB connection on some port. By default, this is port
    `1234`.
    
    Depending on the project you are currently working on you might want
    to set your target architecture specific (16/32-bit). To do so you
    can run `set arch <architecture>` i.e. `set arch i8086` for 16-bit
    mode. In general this course your target architecture should be
    32-bit: `i386:intel` if it is not already set to that automatically.
    For 16-bit debugging, have a look [here](#16-bit-debugging).
    
    If you are unsure what architecture you are currently assuming you
    can check it with: `show arch`.

4.  Run GDB: `gdbtui <compiled target>` i.e. for project-1: `gdbtui
    target/kernel/kernel`
    
    This runs gdb in text user interface mode, but feel free to invoke
    it as you like.
    
    If you want to load in multiple symbol files for everything you link
    in your kernel. You can have them gathered in a file and use `set
    debug-file-directory <directory>` to let GDB know about them all. If
    you have files spread around you can add them individually with
    `add-symbol-file <file>`. This might become repetitive and you might
    want to check how to create command files for this:
    [instructions](https://sourceware.org/gdb/current/onlinedocs/gdb.html/Command-Files.html).
    
    Once we have confirmed the architecture we can connect to Bochs and
    set up a breakpoint.

5.  Connect to Bochs: `target remote localhost:1234`

6.  Setup a breakpoint: `b kernel_main` \<- stop at the start of the
    kernel code

7.  Continue emulation: `c`

For multiple breakpoints i.e. all start-functions you can use `b _start`
this sets multiple breaks, at all `_start` symbols.

By default, the source code window will be in focus when running GDB in
text user interface mode. You can swap focus with `fs next` to toggle,
or `fs <window>` to select a specific one. For scrolling in the GDB
history we would advise setting up logging and having another terminal
tailing the log output.

GDB Docs: [GDB](https://sourceware.org/gdb/current/onlinedocs/gdb.html/)

Bochs Debugger Docs:
[Built-in](https://bochs.sourceforge.io/doc/docbook/user/internal-debugger.html)

### GDB init file

You can put common GDB commands for this project into a `.gdbinit` file
in the project directory.

Helpful commands you may want to put in your `.gdbinit`:

    # Use split C + disassembly layout
    layout split
    
    # Kernel as main file
    file target/kernel/kernel
    
    # Additional symbol files
    add-symbol-file target/process/process1
    add-symbol-file target/process/process2
    
    # Connect to bochs
    target remote localhost:1234
    
    # Always break at kernel startup
    break _start32

Note that GDB will likely refuse to auto-load your `.gdbinit` file for
security reasons. To load the file explicitly, `source` the file from
the debugger command line:

    # At GDB debugger command line
    source .gdbinit

To enable auto-load, mark the project directory as safe in the
`~/.gdbinit` file in your home directory:

    # In home directory ~/.gdbinit file
    add-auto-load-safe-path ~/path/to/project/dir

See:

  - GDB Manual: [Initialization
    Files](https://sourceware.org/gdb/current/onlinedocs/gdb.html/Initialization-Files.html#Initialization-Files)
  - GDB Manual: [Security restriction for
    auto-loading](https://sourceware.org/gdb/current/onlinedocs/gdb.html/Auto_002dloading-safe-path.html)

### 16-bit debugging

For 16-bit mode, GDB is not straightforward as it does not do
`segment:offset` calculations. For 16-bit debugging, the internal Bochs
debugger might suit you better\! See [this
stack](https://stackoverflow.com/questions/32955887/how-to-disassemble-16-bit-x86-boot-sector-code-in-gdb-with-x-i-pc-it-gets-tr)
if you want to get into 16-bit GDB.

## Known-Good Bochs and BIOS Versions

  - **Bochs**: 2.7
  - **BIOS**: SeaBIOS 1.7.0. Compiled at UIO, 2012, with configuration
    lost to history.
  - **VGABIOS**: SeaVGABIOS 1.6.3. Compiled at UiT, 2023, with VBE
    support.
