# ADR 0002: Use GRUB instead of a custom bootloader for M1

- Status: Accepted

## Context

The project must reach a stable boot and debug loop early. Writing a complete custom bootloader before validating the kernel entry path would slow the first milestone and increase risk.

## Decision

Use GRUB with a Multiboot2-compatible kernel entry for the initial boot path.

## Consequences

### Positive

- Faster path to bootable kernel testing
- Standardized boot flow for x86 systems
- Easier integration with QEMU and GDB
- Allows the team to focus on the kernel itself rather than bootloader internals

### Negative

- Gorgon is not yet booting without a bootloader
- The project eventually may move toward a more custom boot path in later phases

## Notes

This ADR keeps the M1 milestone focused on the kernel boot and debug pipeline, which is the right foundation for the rest of the roadmap.
