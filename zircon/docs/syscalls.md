# Zircon System Calls

## Handles
+ [handle_close](syscalls/handle_close.md) - close a handle
+ [handle_close_many](syscalls/handle_close_many.md) - close several handles
+ [handle_duplicate](syscalls/handle_duplicate.md) - create a duplicate handle (optionally with reduced rights)
+ [handle_replace](syscalls/handle_replace.md) - create a new handle (optionally with reduced rights) and destroy the old one

## Objects
+ [object_get_child](syscalls/object_get_child.md) - find the child of an object by its koid
+ [object_get_info](syscalls/object_get_info.md) - obtain information about an object
+ [object_get_property](syscalls/object_get_property.md) - read an object property
+ [object_set_profile](syscalls/object_set_profile.md) - apply a profile to a thread
+ [object_set_property](syscalls/object_set_property.md) - modify an object property
+ [object_signal](syscalls/object_signal.md) - set or clear the user signals on an object
+ [object_signal_peer](syscalls/object_signal_peer.md) - set or clear the user signals in the opposite end
+ [object_wait_many](syscalls/object_wait_many.md) - wait for signals on multiple objects
+ [object_wait_one](syscalls/object_wait_one.md) - wait for signals on one object
+ [object_wait_async](syscalls/object_wait_async.md) - asynchronous notifications on signal change

## Threads
+ [thread_create](syscalls/thread_create.md) - create a new thread within a process
+ [thread_exit](syscalls/thread_exit.md) - exit the current thread
+ [thread_read_state](syscalls/thread_read_state.md) - read register state from a thread
+ [thread_start](syscalls/thread_start.md) - cause a new thread to start executing
+ [thread_write_state](syscalls/thread_write_state.md) - modify register state of a thread

## Processes
+ [process_create](syscalls/process_create.md) - create a new process within a job
+ [process_read_memory](syscalls/process_read_memory.md) - read from a process's address space
+ [process_start](syscalls/process_start.md) - cause a new process to start executing
+ [process_write_memory](syscalls/process_write_memory.md) - write to a process's address space
+ [process_exit](syscalls/process_exit.md) - exit the current process

## Jobs
+ [job_create](syscalls/job_create.md) - create a new job within a job
+ [job_set_policy](syscalls/job_set_policy.md) - modify policies for a job and its descendants

## Tasks (Thread, Process, or Job)
+ [task_create_exception_channel](syscalls/task_create_exception_channel.md) - create an exception channel on a task
+ [task_kill](syscalls/task_kill.md) - cause a task to stop running
+ [task_resume_from_exception](syscalls/task_resume_from_exception.md) - resume a task from a previously caught exception
+ [task_suspend](syscalls/task_suspend.md) - cause a task to be suspended

## Profiles
+ [profile_create](syscalls/profile_create.md) - create a new profile object

## Exceptions
+ [exception_get_thread](syscalls/exception_get_thread.md) - create a handle for the exception thread
+ [exception_get_process](syscalls/exception_get_process.md) - create a handle for the exception process

## Channels
+ [channel_call](syscalls/channel_call.md) - synchronously send a message and receive a reply
+ [channel_create](syscalls/channel_create.md) - create a new channel
+ [channel_read](syscalls/channel_read.md) - receive a message from a channel
+ [channel_read_etc](syscalls/channel_read_etc.md) - receive a message from a channel with handle information
+ [channel_write](syscalls/channel_write.md) - write a message to a channel
+ [channel_write_etc](syscalls/channel_write_etc.md) - _unimplemented_

## Sockets
+ [socket_create](syscalls/socket_create.md) - create a new socket
+ [socket_read](syscalls/socket_read.md) - read data from a socket
+ [socket_shutdown](syscalls/socket_shutdown.md) - prevent reading or writing
+ [socket_write](syscalls/socket_write.md) - write data to a socket

## Fifos
+ [fifo_create](syscalls/fifo_create.md) - create a new fifo
+ [fifo_read](syscalls/fifo_read.md) - read data from a fifo
+ [fifo_write](syscalls/fifo_write.md) - write data to a fifo

## Events and Event Pairs
+ [event_create](syscalls/event_create.md) - create an event
+ [eventpair_create](syscalls/eventpair_create.md) - create a connected pair of events
+ [system_get_event](syscalls/system_get_event.md) - retrieve a handle to a system event

## Ports
+ [port_create](syscalls/port_create.md) - create a port
+ [port_queue](syscalls/port_queue.md) - send a packet to a port
+ [port_wait](syscalls/port_wait.md) - wait for packets to arrive on a port
+ [port_cancel](syscalls/port_cancel.md) - cancel notifications from async_wait

## Futexes
+ [futex_wait](syscalls/futex_wait.md) - wait on a futex
+ [futex_wake](syscalls/futex_wake.md) - wake waiters on a futex
+ [futex_requeue](syscalls/futex_requeue.md) - wake some waiters and requeue other waiters

## Virtual Memory Objects (VMOs)
+ [vmo_create](syscalls/vmo_create.md) - create a new vmo
+ [vmo_read](syscalls/vmo_read.md) - read from a vmo
+ [vmo_write](syscalls/vmo_write.md) - write to a vmo
+ [vmo_create_child](syscalls/vmo_create_child.md) - creates a child of a vmo
+ [vmo_get_size](syscalls/vmo_get_size.md) - obtain the size of a vmo
+ [vmo_set_size](syscalls/vmo_set_size.md) - adjust the size of a vmo
+ [vmo_op_range](syscalls/vmo_op_range.md) - perform an operation on a range of a vmo
+ [vmo_replace_as_executable](syscalls/vmo_replace_as_executable.md) - add execute rights to a vmo
+ [vmo_create_physical](syscalls/vmo_create_physical.md) - create a VM object referring to a specific contiguous range of physical memory
+ [vmo_set_cache_policy](syscalls/vmo_set_cache_policy.md) - set the caching policy for pages held by a VMO.

## Virtual Memory Address Regions (VMARs)
+ [vmar_allocate](syscalls/vmar_allocate.md) - create a new child VMAR
+ [vmar_map](syscalls/vmar_map.md) - map a VMO into a process
+ [vmar_unmap](syscalls/vmar_unmap.md) - unmap a memory region from a process
+ [vmar_protect](syscalls/vmar_protect.md) - adjust memory access permissions
+ [vmar_destroy](syscalls/vmar_destroy.md) - destroy a VMAR and all of its children

## Userspace Pagers
+ [pager_create](syscalls/pager_create.md) - create a new pager object
+ [pager_create_vmo](syscalls/pager_create_vmo.md) - create a pager owned vmo
+ [pager_detach_vmo](syscalls/pager_detach_vmo.md) - detaches a pager from a vmo
+ [pager_supply_pages](syscalls/pager_supply_pages.md) - supply pages into a pager owned vmo

## Cryptographically Secure RNG
+ [cprng_add_entropy](syscalls/cprng_add_entropy.md)
+ [cprng_draw](syscalls/cprng_draw.md)

## Time
+ [nanosleep](syscalls/nanosleep.md) - sleep for some number of nanoseconds
+ [clock_get](syscalls/clock_get.md) - read a system clock
+ [clock_get_monotonic](syscalls/clock_get_monotonic.md) - read the monotonic system clock
+ [ticks_get](syscalls/ticks_get.md) - read high-precision timer ticks
+ [ticks_per_second](syscalls/ticks_per_second.md) - read the number of high-precision timer ticks in a second
+ [deadline_after](syscalls/deadline_after.md) - Convert a time relative to now to an absolute deadline
+ [clock_adjust](syscalls/clock_adjust.md) - 

## Timers
+ [timer_create](syscalls/timer_create.md) - create a timer object
+ [timer_set](syscalls/timer_set.md) - start a timer
+ [timer_cancel](syscalls/timer_cancel.md) - cancel a timer

## Hypervisor guests
+ [guest_create](syscalls/guest_create.md) - create a hypervisor guest
+ [guest_set_trap](syscalls/guest_set_trap.md) - set a trap in a hypervisor guest

## Virtual CPUs
+ [vcpu_create](syscalls/vcpu_create.md) - create a virtual cpu
+ [vcpu_resume](syscalls/vcpu_resume.md) - resume execution of a virtual cpu
+ [vcpu_interrupt](syscalls/vcpu_interrupt.md) - raise an interrupt on a virtual cpu
+ [vcpu_read_state](syscalls/vcpu_read_state.md) - read state from a virtual cpu
+ [vcpu_write_state](syscalls/vcpu_write_state.md) - write state to a virtual cpu
+ [interrupt_bind_vcpu](syscalls/interrupt_bind_vcpu.md) - Bind an interrupt object to a VCPU

## Global system information
+ [system_get_dcache_line_size](syscalls/system_get_dcache_line_size.md)
+ [system_get_features](syscalls/system_get_features.md) - get hardware-specific features
+ [system_get_num_cpus](syscalls/system_get_num_cpus.md) - get number of CPUs
+ [system_get_physmem](syscalls/system_get_physmem.md) - get physical memory size
+ [system_get_version](syscalls/system_get_version.md) - get version string

## Debug Logging
+ [debuglog_create](syscalls/debuglog_create.md) - create a kernel managed debuglog reader or writer
+ [debuglog_write](syscalls/debuglog_write.md) - write log entry to debuglog
+ [debuglog_read](syscalls/debuglog_read.md) - read log entries from debuglog
+ [debug_read](syscalls/debug_read.md) - TODO(ZX-3106)
+ [debug_write](syscalls/debug_write.md) - TODO(ZX-3106)
+ [debug_send_command](syscalls/debug_send_command.md) - TODO(ZX-3106)

## Multi-function
+ [vmar_unmap_handle_close_thread_exit](syscalls/vmar_unmap_handle_close_thread_exit.md) - three-in-one
+ [futex_wake_handle_close_thread_exit](syscalls/futex_wake_handle_close_thread_exit.md) - three-in-one

## System
+ [system_mexec](syscalls/system_mexec.md) - Soft reboot the system with a new kernel and bootimage
+ [system_mexec_payload_get](syscalls/system_mexec_payload_get.md) - Return a ZBI containing ZBI entries necessary to boot this system
+ [system_powerctl](syscalls/system_powerctl.md)

## DDK
+ [bti_create](syscalls/bti_create.md) - create a new bus transaction initiator
+ [bti_pin](syscalls/bti_pin.md) - pin pages and grant devices access to them
+ [bti_release_quarantine](syscalls/bti_release_quarantine.md) - releases all quarantined PMTs
+ [cache_flush](syscalls/cache_flush.md) - Flush CPU data and/or instruction caches
+ [interrupt_ack](syscalls/interrupt_ack.md) - Acknowledge an interrupt object
+ [interrupt_bind](syscalls/interrupt_bind.md) - Bind an interrupt object to a port
+ [interrupt_create](syscalls/interrupt_create.md) - Create a physical or virtual interrupt object
+ [interrupt_destroy](syscalls/interrupt_destroy.md) - Destroy an interrupt object
+ [interrupt_trigger](syscalls/interrupt_trigger.md) - Trigger a virtual interrupt object
+ [interrupt_wait](syscalls/interrupt_wait.md) - Wait on an interrupt object
+ [iommu_create](syscalls/iommu_create.md) - create a new IOMMU object in the kernel
+ [pmt_unpin](syscalls/pmt_unpin.md) - unpin pages and revoke device access to them
+ [resource_create](syscalls/resource_create.md) - create a resource object
+ [smc_call](syscalls/smc_call.md) - Make an SMC call from user space

## Display drivers
+ [framebuffer_get_info](syscalls/framebuffer_get_info.md)
+ [framebugger_set_range](syscalls/framebuffer_set_range.md)

## Tracing
+ [ktrace_control](syscalls/ktrace_control.md)
+ [ktrace_read](syscalls/ktrace_read.md)
+ [ktrace_write](syscalls/ktrace_write.md)
+ [mtrace_control](syscalls/mtrace_control.md)

## Others/Work in progress
+ [pc_firmware_tables](syscalls/pc_firmware_tables.md)
+ [pci_add_subtract_io_range](syscalls/pci_add_subtract_io_range.md)
+ [pci_cfg_pio_rw](syscalls/pci_cfg_pio_rw.md)
+ [pci_config_read](syscalls/pci_config_read.md)
+ [pci_config_write](syscalls/pci_config_write.md)
+ [pci_enable_bus_master](syscalls/pci_enable_bus_master.md)
+ [pci_get_bar](syscalls/pci_get_bar.md)
+ [pci_get_nth_device](syscalls/pci_get_nth_device.md)
+ [pci_init](syscalls/pci_init.md)
+ [pci_map_interrupt](syscalls/pci_map_interrupt.md)
+ [pci_query_irq_mode](syscalls/pci_query_irq_mode.md)
+ [pci_reset_device](syscalls/pci_reset_device.md)
+ [pci_set_irq_mode](syscalls/pci_set_irq_mode.md)


+ [syscall_test_0](syscalls/syscall_test_0.md)
+ [syscall_test_1](syscalls/syscall_test_1.md)
+ [syscall_test_2](syscalls/syscall_test_2.md)
+ [syscall_test_3](syscalls/syscall_test_3.md)
+ [syscall_test_4](syscalls/syscall_test_4.md)
+ [syscall_test_5](syscalls/syscall_test_5.md)
+ [syscall_test_6](syscalls/syscall_test_6.md)
+ [syscall_test_7](syscalls/syscall_test_7.md)
+ [syscall_test_8](syscalls/syscall_test_8.md)
+ [syscall_test_wrapper](syscalls/syscall_test_wrapper.md)


