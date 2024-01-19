  - [WSL2 on Windows 10/11](#wsl2-on-windows-1011)
      - [Installation](#installation)
          - [Windows build \>= 19041](#windows-build-19041)
          - [Windows build \< 19041](#windows-build-19041-1)
          - [Troubleshooting](#troubleshooting)
      - [Best practices](#best-practices)
      - [Graphics](#graphics)
      - [Setting up your new WSL
        environment](#setting-up-your-new-wsl-environment)
          - [SSH](#ssh)
          - [Source code on GitHub](#source-code-on-github)
          - [VSCode in WSL](#vscode-in-wsl)

# WSL2 on Windows 10/11

Windows facilitates to run thin virtual machines on top of an optimized
subset of hyper-v, we know it as Windows Subsystem for Linux.

## Installation

There’s two paths for installing WSL2, depending on your Windows build
and version.

### Windows build \>= 19041

For newer builds of Windows the installation steps should be a one-liner
from Powershell.

1.  Open your preferred version of Powershell as administrator
2.  List the available distributions
    1.  `wsl --list --online`
3.  Install WSL
    1.  `wsl --install <distribution name>` i.e.
    2.  `wsl --install Ubuntu`(default)

If you have troubles, check out [Troubleshooting](#troubleshooting)

### Windows build \< 19041

Before you can go ahead and install WSL we need to enable the optional
Windows subsystem for Linux feature.

**Note:** Builds lower than 18362 does not support WSL version 2, and we
recommend you to check out other options such as VMWare or renting
virtual machines in azure.

1.  Open powershell as administrator
    1.  `dism.exe /online /enable-feature
        /featurename:Microsoft-Windows-Subsystem-Linux /all /norestart`
2.  Enable Virtual Machine Platform
    1.  `dism.exe /online /enable-feature
        /featurename:VirtualMachinePlatform /all /norestart`
3.  Restart your machine
4.  Install WSL2 Linux kernel update package
    1.  [x64](https://wslstorestorage.blob.core.windows.net/wslblob/wsl_update_x64.msi)
    2.  [arm](https://wslstorestorage.blob.core.windows.net/wslblob/wsl_update_arm64.msi)
5.  Set WSL 2 as your default version
    1.  wsl –set-default-version 2
6.  Install the distribution of Linux you want from the Microsoft Store
    1.  [Store](https://aka.ms/wslstore)
    2.  **Note**: The build system we have for our projects is tested on
        Ubuntu 22.04.
    3.  **Note**: Install the distribution without specified version
        number, this will allow you to update it later

If you have troubles, check out [Troubleshooting](#troubleshooting)

### Troubleshooting

If you have troubles with the installation after the steps above check
out MS’
[troubleshooting](https://learn.microsoft.com/en-us/windows/wsl/troubleshooting#installation-issues).

## Best practices

Microsoft has put together a decent guide with information how to set up
your environment. Things they’ll cover:

1.  Creating your user in your new WSL2 distribution
2.  How to update and upgrade packages
3.  Setting up [Windows Terminal](https://github.com/microsoft/terminal)
4.  What to do with your file system and much more, check it out\!

<!-- end list -->

  - [MS best
    practice](https://learn.microsoft.com/en-us/windows/wsl/setup/environment)

## Graphics

Microsoft has since build 2021 conference and Windows 10 insider build
21364 included support for Linux graphical applications using X11 and
Wayland called Windows Subsystem for Linux GUI (WSLg). With Windows 11
this was put into production builds of Windows, bringing support for
both graphic and audio in WSL applications.

If you have older versions of Windows you will have to configure an
X-server on the host side and route graphics to this server from inside
WSL. We will not go in depth on that here, contact one of the staff
members for help on setting this up or check out the links below.

  - [The stack on
    VcXsrc](https://stackoverflow.com/questions/61110603/how-to-set-up-working-x11-forwarding-on-wsl2)
  - [x410](https://x410.dev/cookbook/wsl/using-x410-with-wsl2/)

## Setting up your new WSL environment

This takes you through the basic steps of setting up dependencies for
this course. Have a look at [Best practices](#best-practices) to set up
a nice visual environment using Windows Terminal.

We will assume you chose Ubuntu as your distribution, and apt as your
package manager in this section.

We will need to install a few dependencies to not have issues later down
the line in this course. List of dependencies:

| Package           | Purpose                                  |
| ----------------- | ---------------------------------------- |
| `git`             | We use git for distributing project code |
| `build-essential` | GCC, make, et al.                        |
| `gcc-multilib`    | compiling x86-32 code on x86-64          |
| `bochs`           | Emulator for i32                         |

Update your system and install dependencies.

    sudo apt update && sudo apt -y upgrade && sudo apt -y autoremove && \
    sudo apt install -y git build-essential gcc-multilib

Bochs can be installed through apt with `built-in debugger` included, if
you want to compile it from scratch to include `gdb-debugging`
possibilities, please have a look at our Bochs-guide found in the
project-os repository under `/doc/tools/bochs.md`.

Installing using apt with X11 Window System:

    sudo apt install bochs-x

Since we will be using GitHub for our projects, we recommend having a
look at GitHub’s
[Quickstart](https://docs.github.com/en/get-started/quickstart/about-github-and-git)
on how to set up an account and get started with their hosting services.

### SSH

We recommend setting up a pair of ssh keys instead of using
username/password to authenticate when using git.

1.  Generate your keys
    1.  `ssh-keygen -t ed_25519 -C
        <a-useful-comment-about-this-key-pair>`
2.  Upload your `public` key to GitHub. **NOT YOUR PRIVATE**
    1.  The key-pair is usually found in `~/.ssh/` as
        `id_ed25519`(private) and `id_ed25519.pub`(public)
    2.  Copy the content of `id_ed25519.pub` and go to [GitHub settings
        keys](https://github.com/settings/keys)
    3.  Press `new SSH key` and add your public key

You should now be able to authenticate with SSH instead of
username/password when interacting with GitHub.

**Note**: You have to redo this process for each machine you want access
on.

### Source code on GitHub

To retrieve the source code for our projects, we recommend setting up a
private repository where you and your group can work together with
version control.

1.  Create a new repository for your private use
      - We won’t be able to use the course repository for our code.
      - Recommend a hosting platform, such as GitHub.
          - Create a new repository and copy the remote URL
              - Either SSH:
                `git@github.com:<username>/<repository-name>.git` or
                HTTPS:
                `https://github.com/<username>/<repository-name>.git`
2.  Now we need to get it down to your local machine, you have two
    choices. Choose ONE path
    1.  Clone the repository
        1.  `cd <path-to-a-good-location>`
        2.  `git clone <git@github.com:username/repository-name.git`
    2.  Initialize the repository locally and configure a remote
        1.  `mkdir <name-of-your-liking>`
        2.  `cd <name-of-your-liking>`
        3.  `git init`
        4.  `git remote add <name> <url>` i.e.
            1.  `git remote add origin
                <git@github.com:username/my-awesome-OS.git`
        5.  You can verify that you have both push and fetch(pull)
            1.  `git remote -v`
            2.  Should give you an output on the format
                1.  `<name> <url> (fetch)`
                2.  `<name> <url> (push)`

We should be able to work with our repository like we’re used to.

In this course, we will distribute patches and push new starting code
for the different projects under our GitHub organization; we need to add
that remote to our repository to get the updates.

1.  Add remote to our repo:
    1.  `git remote add <name> <url>` i.e.
          - `git remote add source
            git@github.com:uit-inf-2201-s24/project-os.git`

We only want to pull down updates and new starting code from this
repository, so we’ll remove the possibility of pushing to it.

2.  `git remote set-url --push <name> <anything-but-a-valid-link>` i.e.
      - `git remote set-url --push source disabled_push`

You can see that the remote link for source is disabled with `git remote
-v`, or try `git push source main`, and you should expect an output
like:

    origin <url> (fetch)
    origin <url> (push)
    source <url> (fetch)
    source disabled_push (push)

After these steps, you can try merging in the source code from
project-os with: - `git pull <remote> <branch>` i.e. - `git pull source
main`

And to finally sync it up with our private remote: - `git push -u <name>
<branch>` i.e. - `git push -u origin main`

The `-u` flag will add the upstream(GitHub) reference as default for our
local branch, so we don’t have to specify our remote target for every
`pull` and `push`.

### VSCode in WSL

Many of us use VSCode as editor, and the process of installing it in WSL
is a bit different than normal. With WSL we will need to have VSCode
installed on the `host`(Windows).

1.  Go to [VSCode](https://code.visualstudio.com/) and install VSCode
    for Windows(host)
    1.  **Important**: make sure you select **Add to path** during the
        installation, just to simplify using VSCode later.
2.  After installing VSCode, open up a folder you would like to open in
    VSCode, i.e. home: `cd ~`
3.  Open the folder
    1.  `code .`
4.  Recommended base extensions for VSCode:(ctrl+shit+X)
    1.  Remote Development: open folders in remote machines/containers
    2.  C/C++: IntelliSense, debugging and code browsing
    3.  MakefileTools: IntelliSense, build, debug/run

Contact any of the staff if you run into problems\!
