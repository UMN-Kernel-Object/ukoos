# First Day

## Setting Up

Start with the [setting up guide](/getting-started/devcontainer.md) to download and setup the workspace and tools required. You will need VSCode and Docker, a devcontainer is provided in the git repository.

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

Which will be controlled by GDB once we start it here
```shell
make gdb
```

There is a [more detailed guide](/getting-started/using-gdb.md) on using gdb, but here are the basics:  
| Command  | Meaning                                                   |
|----------|-----------------------------------------------------------|
| n        | next line                                                 |
| s        | step (like next line, but enters function calls)          |
| c        | continue until next breakpoint (or end)                   |
| b        | add breakpoint at current point                           |
| b symbol | add breakpoint to symbol (symbol is a function name, etc.)|

**ASCIINEMA GDB**

