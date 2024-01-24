  - [Common Errors](#common-errors)
      - [Linker error: `searching for
        -lgcc`](#linker-error-searching-for--lgcc)
      - [Makefile error: `unterminated call to function
        'shell'`](#makefile-error-unterminated-call-to-function-shell)

## Common Errors

### Linker error: `searching for -lgcc`

    /usr/bin/ld: skipping incompatible /usr/lib/gcc/x86_64-linux-gnu/11/libgcc.a when searching for -lgcc

#### Problem

You are building on a 64-bit x86 PC. You need to install a 32-bit
version of this library.

#### Solution

On Ubuntu (or other `apt`-based distro):

    sudo apt install gcc-multilib

#### Explanation

`libgcc` is GCC’s low-level runtime library. It includes routines for C
operations that are not directly supported by the target CPU, such as
64-bit math on a 32-bit architecture.

Code emitted by GCC always expects to be linked with `-lgcc`, even if
other standard/default libraries are excluded. [OSDev’s libgcc
page](https://wiki.osdev.org/Libgcc) has a FAQ that is pretty funny to
read. It is a lot of “Do I really need this? / Yes. / What if I don’t
need it? / You really do.” and so on.

Since our target architecture is 32-bit x86, you will need to install
the 32-bit versions of this and other common libraries.

In this error, you can see the linker searching for `libgcc`, finding
only the 64-bit version, and giving up. Once the 32-bit version is
installed, you should be fine.

For more on `libgcc`, see the [libgcc chapter in the GCC Internals
Manual](https://gcc.gnu.org/onlinedocs/gccint/Libgcc.html).

### Makefile error: `unterminated call to function 'shell'`

    Makefile:50: *** unterminated call to function 'shell': missing ')'. Stop.

#### Problem

You are using an older version of GNU Make (before 4.3) that parses that
line incorrectly. This can happen if you are using an older distro, such
as Ubuntu 20.04.

#### Solution 1: Do the build in a container

Our code contains a Dockerfile describing a known-good development
environment and a helper script to spin up a container, map in the
source code directory, and run a command. If you have Docker set up on
your machine, try the script:

    ./docker-run make

#### Solution 2: Upgrade your development environment

As of Spring 2024, our code is developed on Ubuntu 22.04 LTS (Jammy
Jellyfish). If you upgrade your development machine / VM to use this
version, then you should not have any build errors caused by different
compiler versions.

#### Solution 3: Edit the offending line in the Makefile

This Makefile `shell` command includes a quoted hash/pound character
(`'#'`):

``` make
extractaddr = $(shell echo '$(1)' \
          | $(CPP) $(CPPFLAGS) -E -include syslib/addrs.h - \
          | grep -v '#')
```

Escape the `'#'` to `'\#'`:

``` make
extractaddr = $(shell echo '$(1)' \
          | $(CPP) $(CPPFLAGS) -E -include syslib/addrs.h - \
          | grep -v '\#')
```

Note that this only fixes this one error. Other errors due to version
mismatch may be lurking. It may be better to upgrade your development
environment or run the build in a container.

#### Explanation

This Makefile `shell` function call includes a quoted hash/pound
character (`'#'`):

``` make
extractaddr = $(shell echo '$(1)' \
          | $(CPP) $(CPPFLAGS) -E -include syslib/addrs.h - \
          | grep -v '#')
```

Make versions 4.2 and earlier do not recognize that the `#` is quoted as
part of a command. They interpret it as the start of a comment in the
Makefile, which clobbers the rest of the line.

``` 
                     This is interpreted as a comment
                     ---
          | grep -v '#')
          -----------
          which leaves this as an unterminated function call
```

Since the parser thinks that the closing paren is part of a comment, it
ignores it. And then it wonders why there is no closing paren in the
function call.

Make version 4.3 change this behavior so that the `'#'` is recognized as
part of the shell command. From the [Make 4.3 Release
Notes](https://lists.gnu.org/archive/html/info-gnu/2020-01/msg00004.html):

>   - WARNING: Backward-incompatibility\!
>     
>     Number signs (\#) appearing inside a macro reference or function
>     invocation no longer introduce comments and should not be escaped
>     with backslashes:
>     
>     thus a call such as:
>     
>     ``` 
>       foo := $(shell echo '#')
>     ```
>     
>     is legal. Previously the number sign needed to be escaped, for
>     example:
>     
>     ``` 
>       foo := $(shell echo '\#')
>     ```
>     
>     Now this latter will resolve to “`\#`”. If you want to write
>     makefiles portable to both versions, assign the number sign to a
>     variable:
>     
>     ``` 
>       H := \#
>       foo := $(shell echo '$H')
>     ```
>     
>     This was claimed to be fixed in 3.81, but wasn’t, for some reason.
>     To detect this change search for ‘nocomment’ in the .FEATURES
>     variable.
