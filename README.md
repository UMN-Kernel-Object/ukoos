# uko OS

A small RISC-V operating system for learning OS development.

## Getting Started

View the full [ukoOS Docs](https://umn-kernel-object.github.io/ukoos/).  

### Using Dev Containers

Clone the repo, and open it in vs code  
```shell
git clone https://github.com/UMN-Kernel-Object/ukoos.git
code ukoos
```
From a shell in the dev container, configure and build  
```shell
mkdir build
cd build
../configure
make
```

To host documentation
```shell
cd build
make doc-serve
```

### Without a Dev Container (Linux / MacOS)

You will need a riscv cross compiler, such as gcc + binutils (compile target riscv64-none-elf), qemu-system-riscv64, and mdbook (for docs). Make sure these tools are in your PATH.

Clone the repo, and open it in vs code  
```shell
git clone https://github.com/UMN-Kernel-Object/ukoos.git
cd ukoos
mkdir build
../configure
make
```
