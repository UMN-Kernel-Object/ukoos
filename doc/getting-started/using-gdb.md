# GDB

GDB is a debugger that will allow us to debug the kernel under a virtual machine.

From the first terminal run qemu, but we need to add options to enable debugging
```shell
make qemu-debug
```


Which will be controlled by GDB once we start it here (from a new shell)
```shell
make gdb
```

## Commands

Here are some useful commands, note that commands can be abbreviated so `next` can become `n`. There are many other commands so make sure to read the docs and help menus.


### General Commands
| Command    | Meaning                                                   |
|------------|-----------------------------------------------------------|
| q          | quit gdb                                                  |
| kill       | kill debugged process (in our case ukoOS)                 |
| info r     | list register values                                      |
| file *file*| load symbols from *file*                                  |
| tui enable | enable tui (to split screen source and gdb)               |
| help *cmd* | help for *cmd*                                            |

### Running Commands
| Command  | Meaning                                                   |
|----------|-----------------------------------------------------------|
| n        | next line                                                 |
| s        | step (like next line, but enters function calls)          |
| c        | continue until next breakpoint (or end)                   |
| si       | step instruction                                          |
| ni       | next instruction                                          |


### Breakpoints
| Command   | Meaning                                                   |
|-----------|-----------------------------------------------------------|
| b         | add breakpoint at current point                           |
| b *symbol*| add breakpoint to symbol (symbol is a function name, etc.)|
| en  *n*   | enable breakpoint *n*                                     |
| dis *n*   | disable breakpoint *n*                                    |
| info b    | list breakpoints                                          |


### Printing
The `p` command can do quite a lot:
| Command     | Meaning                                                   |
|-------------|-----------------------------------------------------------|
| p \**addr*  | print value at address *addr*                             |
| p *expr*    | print out some c-like expression, e.g. p struct->name     |
| p/s \**addr*| print c-string at *addr*                                  |



## Demo

These are from the [first day](/tutorials/first-day.md) docs.

The first terminal, running `make qemu-debug` will look like this
<script src="https://asciinema.org/a/j0QKk3PSpPhwaB49N18fIqhU4.js" id="asciicast-j0QKk3PSpPhwaB49N18fIqhU4" async="true"></script>


And the second, running `make gdb`
<script src="https://asciinema.org/a/kto7njh7ulCJ62ueOpL5mFZ3d.js" id="asciicast-kto7njh7ulCJ62ueOpL5mFZ3d" async="true"></script>


