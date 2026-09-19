global gdt_load
global gdt_tss_descriptor

section .text
bits 32

gdt_load:
lgdt [gdt_pointer]
ret

; FIX: was section .rodata. Once tss.c patches the TSS
; descriptor's base-address fields at runtime (the address of
; the TSS struct isn't known until link/load time), this table
; is no longer truly read-only, so it now lives in .data.
section .data

gdt:
dq 0
dq (1 << 41) | (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53)
dq (1 << 41) | (1 << 44) | (1 << 47)

; FIX: TSS descriptor slot (selector 0x18). Unlike the code/data
; descriptors above, a system descriptor is 16 bytes in long
; mode, not 8 - it needs room for a full 64-bit base address.
;
; Limit is hardcoded to 0x67 (103), i.e. a 104-byte TSS with no
; I/O permission bitmap. tss.c's struct must be exactly 104
; bytes, packed, or this limit needs updating to match.
;
; Base address fields (bytes 2-3, 4, 7, 8-11) are left as 0
; here - tss.c fills them in at runtime with the real address
; of the TSS struct, since NASM can't compute "bits 16-23 of a
; linker symbol's address" as a static value.
gdt_tss_descriptor:
dw 0x0067          ; limit 15:0
dw 0x0000          ; base 15:0        (patched at runtime)
db 0x00            ; base 23:16       (patched at runtime)
db 0x89            ; present, DPL=0, type=1001 (avail 64-bit TSS)
db 0x00            ; limit 19:16 | flags
db 0x00            ; base 31:24       (patched at runtime)
dd 0x00000000      ; base 63:32       (patched at runtime)
dd 0x00000000      ; reserved

gdt_pointer:
dw $ - gdt - 1
dd gdt