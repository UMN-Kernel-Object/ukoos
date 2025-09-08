# First Day

## Setting Up

Start by setting up a dev container for [Linux](/getting-started/linux.md), [MacOS](/getting-started/macos.md), [Windows](/getting-started/windows.md). You will need VSCode and Docker to use the provided devcontainer (in the git repo).

## Checkout out first day code

The kernel is hosted on [GitHub](https://github.com/UMN-Kernel-Object/ukoos) so checkout the code
```shell
git clone https://github.com/UMN-Kernel-Object/ukoos.git
cd ukoos
```

## Compiling

Once we have the code and tools, the [compiling guide](/getting-started/compiling.md) will show how to build and test the kernel. The basics (from the ukoos folder).
```shell
mkdir build
cd build
../configure
make
```

## Running

Once the kernel is built, we can run the code inside a virutal machine (qemu)
```shell
make qemu
```

**Include asciinema**

It should spit out a bunch of debug information, look for  XXXXX
XXXXXXXX


## Debugging

From the first terminal run qemu, but we need to add options to enable debugging
```shell
make qemu QEMUFLAGS="-s -S"
```

Which will be controlled by GDB once we start it here (from a new shell)
```shell
make gdb
```

From the gdb window
```
target remote :1234
```

There is a [more detailed guide](/getting-started/using-gdb.md) on using gdb, but here are the basics:  
| Command    | Meaning                                                   |
|------------|-----------------------------------------------------------|
| n          | next line                                                 |
| s          | step (like next line, but enters function calls)          |
| c          | continue until next breakpoint (or end)                   |
| b          | add breakpoint at current point                           |
| b symbol   | add breakpoint to symbol (symbol is a function name, etc.)|
| file *file*| load symbols from *file*                                  |
| tui enable | enable tui (show both source and gdb prompt               |


**ASCIINEMA GDB**

