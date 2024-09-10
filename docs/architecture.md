# Architecture

## Userspace APIs

### Syscalls

Bekos aims to have a minimal set of well-designed syscalls. Where possible, syscalls will have a well-defined but fairly
flexible API, such that they are applicable to a large class of Kernel APIs/functions.

A number of syscalls will expose an IPC-system interface (see below).

### Filesystem

bekOS does not adopt an 'everything is a file' approach; in general the only nodes accessible by filesystem paths will
be objects genuinely committed to the disc-based filesystem.

bekOS does, however, generalise the concept of a _file descriptor_ (fd) to an _entity handle_. Open entities can be
files, streams, socket connections, device interfaces, etc., but the mechanism by which such entities are located and
opened will depend on the precise nature of the entity.

#### Key Terms
Entity
: A user-accessible object, for example an open file, socket, or pipe.

Entity Handle
: A handle on an open entity. Equivalent to POSIX's _open file description_. For file entities, contains the current
seek() offset. Can be shared between processes.

Entity Handle Slot
: A numeric index into the process's table of entity handles. Equivalent to POSIX's _file descriptor_.

Note that:

- Different processes may use different slots to refer to the same entity handle, and a single process may use multiple
  slots to do so.
- (Only) when all slots referring to a handle are closed, the handle itself is closed.
- Each slot has a group (a number $\in [0, 32)$). Slot groups can be closed _en masse_. This is how bekOS implements
  `O_CLOEXEC` --- all slots of group `1` are typically closed on exec().

### Drivers and devices

bekOS does not permit userspace drivers, and will generally try to manage the currently connected devices in-kernel.
Device drivers can expose _interfaces_ which are searchable and accessible via specific syscalls. Interfaces will comply
with the relevant IPC protocol specification (thus interactions with kernel-managed devices will be very similar to
interactions with userspace processes, and so in future userspace drivers will be easily introduced).

### Inter-process Communication (IPC)

Whilst bekOS supports binary-blob-level IPC, there is an officially defined API/code-generator for message formatting
and passing, which processes are encouraged to use.

Drivers and some kernel interfaces will also use this message format.

The message format does not permit storage or transmission to other machines; it makes no portability guarantees beyond
the C ABI.

## Kernel APIs

### Drivers and devices

There is a system-wide `DeviceRegistry`, with which all identified devices must be registered. This allows:

- the kernel to automatically expose information and debug information about detected devices, and (in future) manage
  nested probing in a seamless manner
- devices to expose interfaces which are seamlessly exposed to userspace applications.