# Compiling

In oder to compile the kernel, we will the code and tooling. The easiest way is to use the [dev container](./getting-started/devcontainer.md). After following the dev container instructions, you will have VS Code with a dev container setup. Open a terminal in the dev container and continue to the build instructions.

## Seting up build environment

The complete set of commands
```shell
mkdir build
cd build
../configure
```

<script src="https://asciinema.org/a/Ta3wVqNbd35v7XEIH2a6LiJXh.js" id="asciicast-Ta3wVqNbd35v7XEIH2a6LiJXh" async="true"></script>

## Building the operating system

The OS can be built by simply running

```shell
cd build
make
```

## Running the kernel

```shell
cd build
make qemu
```

## Running documentation server
```shell
cd build
make doc-serve
```

Visit [localhost:3000](http://localhost:3000) to view the locally hosted docs.
