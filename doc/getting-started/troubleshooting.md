# Troubleshooting

## Linux

- If you are not using the dev container, and your distribution's `tftp` times out, install `busybox`, and try again. If Busybox conflicts with your current tftp package, remove that and then install Busybox again. Then, just running `tftp` should work. This is an issue we are having with Fedora's tftp package, and possibly more.

## macOS

## Windows 11

- If you open ukoOS in the dev container, and it does not recognize `./configure` and/or `sh`, run `git reset --hard`. **NOTE THIS WILL ERASE ALL YOUR LOCAL CHANGES**.

## Dev Container
