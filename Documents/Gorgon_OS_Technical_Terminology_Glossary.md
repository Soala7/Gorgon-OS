# Gorgon OS — Technical Terminology Glossary

*A plain-language reference to the concepts, components and tools used in the project so far.*

## Purpose

This glossary explains the technical words used while developing Gorgon OS. It focuses on what each term means and how it relates to the project, rather than giving implementation instructions.

Some terms describe features already implemented; others describe planned parts of the operating system.

---

## 1. Operating-system foundations

- **Operating System (OS):** System software that manages a computer's hardware and provides services and an environment in which applications run.
- **Kernel:** The core part of an operating system. It manages memory, CPU execution, hardware access and communication between software components.
- **Hybrid Kernel:** A kernel design that combines ideas from monolithic and microkernel designs. Some services operate in kernel space while the architecture can keep other components more separated.
- **Kernel Space:** The privileged memory area in which the kernel executes and can access protected machine resources.
- **User Space:** The less-privileged environment where ordinary applications run. A user-space program normally requests kernel services through system calls.
- **Userspace:** The collection of programs and libraries that run outside the kernel, such as a shell, desktop and applications.
- **Native Userspace:** Programs built specifically for Gorgon OS and designed to use its own interfaces and system services.
- **Guest OS:** An operating system running inside a virtual machine. In the Gorgon plan, Linux can run as a guest while Gorgon is the host.
- **Host OS:** The operating system that controls the physical machine and runs virtual machines. Gorgon is intended to become the host OS in the longer-term design.
- **Boot Process:** The sequence of steps that takes a computer from power-on to a running operating system.
- **Bootloader:** A program that prepares the machine and loads the kernel so that the kernel can begin executing.
- **GRUB:** A widely used bootloader. It can load a kernel and pass boot information to it.
- **Multiboot2:** A boot protocol that defines how a compatible bootloader, such as GRUB, loads a kernel and provides information about memory and the machine.
- **Multiboot Information:** Data supplied by the bootloader. It can include the memory map and other details the kernel needs during startup.

## 2. CPU architecture and low-level code

- **x86-64:** The 64-bit extension of the x86 processor architecture. It is the target CPU architecture for Gorgon's kernel.
- **CPU (Central Processing Unit):** The processor that executes machine instructions.
- **Register:** A small, very fast storage location inside the CPU. Registers hold values such as addresses, counters and control information.
- **Instruction Set Architecture (ISA):** The defined set of instructions, registers and behaviours that software uses when communicating with a processor.
- **Assembly Language:** A human-readable representation of CPU instructions. Gorgon uses x86-64 assembly for low-level startup and processor-specific operations.
- **NASM:** Netwide Assembler, an assembler used to turn assembly source code into machine-code object files.
- **C:** A low-level programming language commonly used for kernels because it provides direct control over memory and data representation.
- **C++:** A systems programming language that adds features such as classes and templates to C. It is planned for parts of Gorgon's higher-level system software.
- **Compiler:** A program that translates source code, such as C, into object code or machine code.
- **Cross-Compiler:** A compiler that runs on one system but produces programs for a different target environment. Gorgon's `x86_64-elf` toolchain is used to build kernel code independently of the host OS.
- **GCC:** GNU Compiler Collection, a compiler suite used to compile Gorgon's C code.
- **Binutils:** GNU binary utilities, including tools for assembling, linking, inspecting and manipulating object files and executable binaries.
- **ELF:** Executable and Linkable Format, a common file format for object files, executables and kernels.
- **Linker:** A tool that combines object files, resolves symbol references and arranges code and data into the final kernel image.
- **Linker Script:** A file that tells the linker where sections of a program should appear in memory and how they should be arranged.
- **Symbol:** A named item in compiled code, such as a function or variable, that the linker and debugger can refer to.
- **GDB:** GNU Debugger, a tool for inspecting program execution, registers, memory and breakpoints.

## 3. Memory and address translation

- **Physical Memory:** The actual RAM installed in the computer.
- **Physical Address:** The address of a location in physical RAM.
- **Virtual Memory:** An address-space system that lets software use virtual addresses which the CPU translates to physical memory.
- **Virtual Address:** An address used by a running program or kernel. It does not necessarily equal the physical RAM address.
- **Address Space:** The set of virtual addresses available to a particular execution environment or process.
- **Paging:** The hardware mechanism that divides memory into fixed-size pages and translates virtual addresses into physical addresses.
- **Page:** A fixed-size block of memory. Gorgon currently uses standard 4 KiB pages and also works with 2 MiB large pages.
- **Page Frame:** A physical block of RAM that can hold a page. A physical page frame is the physical-memory counterpart of a virtual page.
- **Page Table:** A set of data structures used by the CPU to translate virtual addresses into physical addresses and enforce access permissions.
- **Page-Table Hierarchy:** The multiple levels of page tables that the CPU walks through to translate a virtual address.
- **PML4:** Page Map Level 4. The top-level page table in the four-level x86-64 paging layout used by Gorgon.
- **PDPT:** Page Directory Pointer Table. The level below the PML4; it points to page directories or, when using a 1 GiB page, maps a large page directly.
- **PD:** Page Directory. A page-table level below the PDPT. Its entries can point to page tables or map 2 MiB pages directly.
- **PT:** Page Table. The lowest level in the standard four-level layout. Its entries describe individual 4 KiB pages.
- **PML4E:** A PML4 entry. It describes the next paging level or a mapping, and contains permission and status bits.
- **PDPTE:** A PDPT entry. It normally points to a page directory, but can map a 1 GiB page when the large-page bit is set.
- **PDE:** A page-directory entry. It normally points to a page table, or maps a 2 MiB page directly when the large-page bit is set.
- **PTE:** A page-table entry. It describes the mapping and permissions for one 4 KiB virtual page.
- **Page-Table Entry Flags:** Bits in a paging entry that describe whether a mapping exists and what access is allowed.
- **Present Bit (P):** A flag indicating that a page-table entry is valid and present for address translation.
- **Writable Bit (RW):** A flag controlling whether writes are permitted through a mapping, subject to CPU privilege rules.
- **User/Supervisor Bit (U/S):** A flag controlling whether user-mode code may access a mapping. Supervisor-only mappings are restricted to privileged code.
- **NX Bit (No Execute):** A permission bit that prevents instruction execution from a page when enabled. It helps separate executable memory from data.
- **Huge/Large Page:** A page larger than the normal 4 KiB size. Gorgon currently uses 2 MiB mappings in its bootstrap layout; x86-64 can also support 1 GiB pages when configured.
- **Page Splitting:** Replacing one large-page mapping with a lower-level page table so that smaller pages can have individual mappings and permissions.
- **CR3:** An x86-64 control register that holds the physical address of the active top-level page table (PML4). Loading CR3 switches the active address space.
- **TLB (Translation Lookaside Buffer):** A CPU cache that stores recently used virtual-to-physical address translations so the processor does not need to walk page tables every time.
- **INVLPG:** An x86 instruction that invalidates the TLB translation for a specified virtual address.
- **Identity Mapping:** A mapping where a virtual address is equal to its physical address. Gorgon's bootstrap uses an identity map so early kernel code can access physical addresses directly.
- **Higher-Half Kernel:** A kernel mapped at high virtual addresses while its physical image is loaded at lower physical addresses. Gorgon uses a higher-half kernel layout.
- **VMA (Virtual Memory Address):** The virtual address at which a section or symbol is intended to appear when the kernel runs.
- **LMA (Load Memory Address):** The physical address at which a section is loaded into memory.
- **Physmap (Physical Memory Map):** A region of virtual address space that maps physical memory, allowing the kernel to access physical addresses through a predictable virtual-address offset.
- **VMM (Virtual Memory Manager):** The kernel subsystem responsible for creating, changing and inspecting virtual-memory mappings and page tables.
- **Address-Space Clone:** Creating a separate set of page-table structures for another address space. The structures can be independent even when they map some of the same physical memory.
- **Memory Protection:** Rules enforced by the CPU and kernel that restrict which code can read, write or execute particular memory regions.

## 4. Physical memory and allocation

- **PMM (Physical Memory Manager):** The kernel subsystem that tracks physical RAM and allocates or frees physical page frames.
- **Physical Memory Map:** A description, usually provided by the bootloader, of which physical-memory ranges are usable, reserved or unavailable.
- **Memory Region:** A continuous range of physical addresses with a particular purpose or status, such as usable RAM or reserved firmware memory.
- **Bitmap:** A compact data structure made of bits. Gorgon's PMM uses a bitmap to track whether physical pages are free or allocated.
- **Bit:** A binary value of either 0 or 1. In a page-allocation bitmap, each bit represents the state of a page frame.
- **Allocation:** Reserving a resource for use. A PMM allocation reserves a physical page frame.
- **Freeing:** Marking a previously allocated resource as available for reuse.
- **`pmm_alloc_page()`:** The Gorgon PMM function that requests one physical page frame and returns its physical address.
- **`pmm_free_page()`:** The Gorgon PMM function that returns a physical page frame to the allocator.
- **Reserved Memory:** Physical memory that must not be given to the general allocator because it is used by the kernel, boot information, firmware or another component.
- **Kernel Image:** The loaded binary containing the kernel's code and data.
- **Kernel Sections:** Named regions of the kernel image. Common sections include `.text`, `.rodata`, `.data` and `.bss`.
- **`.text`:** The section that contains executable machine code.
- **`.rodata`:** The section that contains read-only data, such as constant tables and strings.
- **`.data`:** The section that contains initialized global or static variables.
- **`.bss`:** The section reserved for uninitialized or zero-initialized global and static variables.

## 5. Kernel memory and execution structures

- **Heap:** A region of memory used for dynamic allocations whose size or lifetime is not known in advance.
- **Heap Allocator:** Kernel code that manages allocations and releases within the heap.
- **Stack:** A memory region used for function calls, local variables, return addresses and saved execution state.
- **Boot Stack:** The temporary stack established during early startup before the kernel's normal memory and execution systems are fully initialized.
- **GDT (Global Descriptor Table):** An x86 structure that defines segment descriptors and, in 64-bit mode, provides information such as the kernel code segment and TSS descriptor.
- **IDT (Interrupt Descriptor Table):** An x86 structure that tells the CPU which handler to run when an interrupt or exception occurs.
- **TSS (Task State Segment):** An x86 structure that holds task-related information. In 64-bit mode, it is especially important for privilege-level stack switching and the Interrupt Stack Table.
- **Interrupt:** A signal or event that causes the CPU to pause its current instruction flow and run a handler.
- **Exception:** A CPU-generated event caused by something such as an invalid instruction, divide error or page fault.
- **Interrupt Handler:** A function or assembly routine that responds to an interrupt or exception.
- **ISR (Interrupt Service Routine):** Another name for an interrupt handler, especially one that services a hardware interrupt.
- **IRQ (Interrupt Request):** A hardware request for processor attention, commonly generated by devices or timers.
- **PIC (Programmable Interrupt Controller):** Hardware that collects and prioritizes interrupt requests and signals the CPU. Gorgon's current setup initializes the legacy PIC.
- **PIT (Programmable Interval Timer):** A legacy timer device that can generate periodic interrupts. Gorgon currently uses it to test timer interrupts and tick counting.
- **Page Fault (#PF):** A CPU exception raised when a memory access cannot be completed under the current paging rules, for example because a page is absent or permissions are violated.
- **CR2:** An x86 control register that records the virtual address that caused the most recent page fault.
- **Serial Output:** Text sent through a serial port, commonly used for kernel diagnostics before a graphical console exists.
- **Kernel Log:** Diagnostic messages produced by the kernel to report startup stages, errors and test results.

## 6. Virtualization and development tools

- **Virtualization:** Running a virtual computer in software so that an operating system can execute without directly owning the physical hardware.
- **Virtual Machine (VM):** An emulated or hardware-assisted computer environment with virtual CPUs, memory and devices.
- **Hypervisor / Virtual Machine Monitor (VMM):** The software layer that creates and manages virtual machines. The abbreviation VMM can also mean Virtual Memory Manager in Gorgon code; context determines which meaning is intended.
- **QEMU:** A machine emulator and virtualizer used to run and test Gorgon OS without rebooting the physical computer.
- **KVM (Kernel-based Virtual Machine):** A Linux kernel facility that uses CPU virtualization extensions to run virtual machines with hardware assistance.
- **VMX:** Intel's CPU virtualization extension, also known as Intel VT-x.
- **SVM:** AMD's CPU virtualization extension, also known as AMD-V.
- **EPT (Extended Page Tables):** Intel hardware-assisted second-level address translation used by virtual machines.
- **NPT (Nested Page Tables):** AMD's hardware-assisted second-level address translation used by virtual machines.
- **VirtIO:** A standard interface for efficient virtual devices, such as virtual disks, network adapters and graphics devices.
- **ISO Image:** A file containing a disc-image layout. Gorgon is packaged as a bootable ISO for testing in QEMU.
- **WSL (Windows Subsystem for Linux):** A Windows feature that provides a Linux environment. It is the current development environment used for Gorgon.
- **Docker:** A container platform used to package development tools and build environments consistently.
- **Build:** The process of compiling and linking source files into a bootable kernel or disk image.
- **Boot Test:** Running the built operating-system image to check that startup and kernel features behave as expected.
- **Debugging:** Finding and diagnosing defects by inspecting program behaviour, memory, registers and execution flow.

## 7. Project-specific names and milestones

- **Gorgon OS:** The user's operating-system project. The long-term goal is an independent x86-64 operating system with its own kernel, memory management, userspace, desktop and virtualization capabilities.
- **Phase 1:** The earlier graphical virtual-OS prototype. It established design ideas and user-facing concepts such as the desktop, file explorer, terminal and window management.
- **Phase 2:** The move toward a real operating system, beginning with the bootable kernel and low-level foundations.
- **M4 / M4B:** The current memory-management milestone focused on giving Gorgon an independent higher-half address-space hierarchy rather than sharing bootstrap page-table structures.
- **Bootstrap:** The small early-startup code and structures that prepare the CPU and memory environment before the main kernel takes over.
- **Bootstrap Page Tables:** The initial page tables created by the boot assembly. They establish the mappings needed to enter 64-bit mode and start the kernel.
- **Kernel Main:** The main C entry point called after low-level startup has prepared the CPU and stack.
- **VOS (Virtual Operating System):** The name used for the earlier Gorgon graphical OS-like environment. It served as a prototype and design reference, distinct from the current bare-metal kernel work.

## Quick reference: how the paging levels connect

**Virtual address → PML4 → PDPT → PD → PT → Physical page frame**

A page-table walk follows these levels for a normal 4 KiB mapping. A large-page entry can stop the walk earlier: a PDPTE can map a 1 GiB page, and a PDE can map a 2 MiB page.

## Important distinction: PMM vs VMM

PMM and VMM are different subsystems:

- **PMM** manages physical page frames: which RAM pages are free, allocated or reserved.
- **VMM** manages virtual addresses and page tables: which virtual address maps to which physical frame and with what permissions.

The VMM often asks the PMM for physical frames to hold page-table structures or mapped memory.

---

*This is a working glossary of the terminology encountered in Gorgon OS development so far. It is not a claim that every listed subsystem has already been implemented; planned architecture and future virtualization terms are included for reference.*
