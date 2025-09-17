# First Day

# Setting Up

Start by setting up a dev container for [Linux](../getting-started/linux.md), [MacOS](../getting-started/macos.md), [Windows](../getting-started/windows.md). You will need VSCode and Docker to use the provided devcontainer (in the git repo).


## Troubleshooting

If there are any issues, check out the [troubleshooting guide](../getting-started/troubleshooting.md) and ask us questions.

## Checkout code

The kernel is hosted on [GitHub](https://github.com/UMN-Kernel-Object/ukoos) so if you haven't already downloaded the code, here is how:
```shell
git clone https://github.com/UMN-Kernel-Object/ukoos.git
cd ukoos
```

## Compiling

Once we have the code and tools, we can compile the code from the dev container
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

## Debugging

From the first terminal run qemu, but we need to add options to enable debugging
```shell
make qemu-debug
```

Which will be controlled by GDB once we start it here (from a new shell)
```shell
make gdb
```

The first terminal, running `make qemu-debug` will look similar to the video below. However, it won't do anything until you run `make gdb` in the second window.
<script src="https://asciinema.org/a/j0QKk3PSpPhwaB49N18fIqhU4.js" id="asciicast-j0QKk3PSpPhwaB49N18fIqhU4" async="true"></script>


And the second, running `make gdb`
<script src="https://asciinema.org/a/kto7njh7ulCJ62ueOpL5mFZ3d.js" id="asciicast-kto7njh7ulCJ62ueOpL5mFZ3d" async="true"></script>

The gdb window will be how you will control the OS (with the controls below) but the output will show up in the first. There is a [more detailed guide](../getting-started/using-gdb.md) on using gdb, but here are the basics:
| Command     | Meaning                                                   |
|-------------|-----------------------------------------------------------|
| n           | next line                                                 |
| s           | step (like next line, but enters function calls)          |
| c           | continue until next breakpoint (or end)                   |
| b           | add breakpoint at current point                           |
| b *symbol*  | add breakpoint to symbol (symbol is a function name, etc.)|
| p *variable*| print the value of the C variable                         |


# Fixing a bug

## Checking out the first day code

Switch to the tutorial code
```shell
git checkout tutorials/first-day
```
and rebuild
```shell
cd build
make
```

Once you have the code and are able to run and debug, you'll notice that "Hello, World!" is misspelled! Try using the debugger to step through and find the line that prints this out incorrectly, then go to the file, edit it, and re-run.
