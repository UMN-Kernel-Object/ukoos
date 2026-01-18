# Overview

In ukoOS, there are three related concepts that are important to keep separate.

- **harts**, or hardware threads.
  Colloquially, we might call these "cores" or "CPUs."
  This terminology comes from RISC-V, but the concept applies to any architecture.
- **tasks**, or kernel threads.
  The kernel manages these automatically, and will create and destroy them at various times.
- **threads**, or user threads.
  These are the threads that userspace programmers talk about.
  They are created only in response to userspace syscalls.

Each of these notions of threads also has its own notion of "thread-locals."
These are stored in various places.

- Hart-locals are pointed to by the `sscratch` CSR, so we can get to them in trap handlers, regardless of whether the trap handler interrupted kernel-space or user-space execution.
  - The hart-locals are defined as `struct hart_locals` in `src/kernel/include/hart_locals.h`.
- Task-locals and thread-locals are pointed to by the `tp` register (`x4`).
  - These are not yet implemented, but probably require linker magic.

## Lifecycle

When the kernel boots, there is one "root task."
This task needs to bootstrap itself a bit to end up with all the right state for a task and start the scheduler.
Once this is done, that task isn't treated specially by ukoOS.

Other tasks are created with the `task_create` function.
This creates a task, including allocating and mapping a stack, but does not initialize the registers or schedule the task.

The `task_set_pc` function sets the program counter and arguments for the task, sets up the stack, etc.

The `task_spawn` function marks the task as schedulable and adds it to the scheduler's queues.

If a task calls the `task_exit` function, or another task calls the `task_kill` function and passes it as an argument, the task exits.

When a task exits:

- Any spinlocks or sleeplocks held by the task are poisoned.
- Any `refcount_guard`s held by the task are `decref`ed.
- Any links or monitors attached to the task will be triggered and removed.
- The task becomes unschedulable and is removed from the scheduler's queues.
  (All the previous tasks are scheduled as normal, though the task's priority is boosted to 14 while cleanup is occurring.)

Once the task's reference count reaches zero, it will be freed.

## Links and Monitors

In ukoOS, tasks that communicate with tasks often need to be aware of when those tasks exit.
This can be achieved with **links** and **monitors**.
These are closely inspired by the Erlang concepts with the same names.

Links are bidirectional; if task `A` is linked to task `B`, then task `B` is linked to task `A`.
Links ensure that if a task exits, other tasks do as well.

A link can be created between the current task and another task with the `task_link` function.
A link can be removed between the current task and another task with the `task_unlink` function.
Only one link can exist at a time between two tasks: if a link already exists, `task_link` is a no-op; if a link does not exist, `task_unlink` is a no-op.

Monitors, on the other hand, are unidirectional.
Unlike in Erlang, a monitor in ukoOS connects a channel to a task.
When the task exits, the channel gets sent a `struct monitor_status`.

## Inter-task communication
