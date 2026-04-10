# Time Travel Debugging

UkoOS supports time travel debugging with qemu.
Time travel debugging allows moving both backwards and forwards when debugging.
It is only supported on the `qemu-riscv64` target.
Make sure that target is selected before using time travel debugging.

## Record

Make sure to configure the `qemu-riscv64` like in the [First Day Tutorial](../tutorials/first-day.md).

To record an execution to play back later, use `make qemu-record`.
Making a recording is required for time travel debugging.

## Replay

Once a recording has been created, `make qemu-replay` is roughly equivalent to
`make qemu-debug`. Run `make qemu-replay`, then `make gdb` in another
terminal. The replay should be the same as the execution recorded earlier.

`rc` or `reverse-continue` will find the most recent breakpoint in the past.

`rsi` or `reverse-stepi` will step one instruction back.
