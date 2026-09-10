section .multiboot_header
header_start: ;start the header data
    ;magic number
    dd 0xe85250d6; multi boot2
    ;architecture
    dd 0 ;protected mode i386
    ;header lenght
    dd header_end - header_start
    ;check sum
    dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))
    ;end tag
    dw 0
    dw 0
    dd 8
header_end: ;end of the header data
;┌─────────────────────┐
;│ Magic               │
;├─────────────────────┤
;│ Architecture        │
;├─────────────────────┤
;│ Header length       │
;├─────────────────────┤
;│ Checksum            │
;├─────────────────────┤
;│ Tags...             │
;├─────────────────────┤
;│ End tag             │
;└─────────────────────┘