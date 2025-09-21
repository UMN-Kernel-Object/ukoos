# Targets

Because different hardware platforms have different requirements, ukoOS supports them as different _targets_.

## Milk-V Duo S (`milkv-duos`)

The [Milk-V Duo S](https://milkv.io/duo-s) is the board we're targetting this year (academic year 2025).
This board uses the Sophgo SG2000 SoC, which uses the T-Head C906 CPU.
The board has 512MiB of RAM and a 1GHz CPU.

The hardware kits for this year also include:

- a 480x320 touchscreen
- a 32GiB microSD card
- a USB UART, which allows connecting to the board's serial port
- jumper wires, to connect the touchscreen to the board

## Milk-V Jupiter (`milkv-jupiter`)

Some members also own devices based on the SpacemiT K1 SoC, which uses the SpacemiT X60 CPU.
This SoC has 8 CPU cores that run at 1.6GHz.

One such device is the [Milk-V Jupiter](https://milkv.io/jupiter).
This board can have the K1, or the closely related SpacemiT M1 SoC (which runs at 2GHz, but otherwise does not significantly differ).
This board can come with between 4GiB and 16GiB of RAM.

We probably won't focus on developing drivers for this system this year, but it might make an attractive target for the future.
It _is_ worth ensuring that the kernel doesn't do anything that breaks this board; it's far more standards-compliant than the Duo S, and future devices we use will hopefully be a lot closer to it.

## QEMU RISC-V (`qemu-riscv64`)

The QEMU-based emulator is its own target.
This uses an RVA22 CPU on the virt machine, with an RTL8139 NIC.
