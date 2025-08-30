# Threads and Harts

In ukoOS, there are three related concepts that are important to keep separate.

- **harts**, or hardware threads.
  Colloquially, we might call these "cores" or "CPUs."
  This terminology comes from RISC-V, but the concept applies to any architecture.
- **kthreads**, or kernel threads.
  The kernel manages these automatically, and will create and destroy them at various times.
- **uthreads**, or user threads.
  These are the threads that userspace programmers talk about.
  They are created only in response to userspace syscalls.

Each of these notions of threads also has its own notion of "thread-locals."
These are stored in various places.

- Hart-locals are pointed to by the `sscratch` CSR, so we can get to them in trap handlers, regardless of whether the trap handler interrupted kernel-space or user-space execution.
- Kernel and user thread-locals are pointed to by the `tp` register (`x4`).
