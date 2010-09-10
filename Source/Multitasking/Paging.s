GLOBAL pml4Base

; This file just contains the basic structures necessary to get the higher half operating
; It also stores the first page directory entries (including recursive page mapping)

VirtualAddressBase				equ 0xFFFF800000000000
VirtualAddressBasePML4			equ (VirtualAddressBase >> 40) & 0x1FF
VirtualAddressBasePDPT			equ (VirtualAddressBase >> 31) & 0x1FF
VirtualAddressBasePageDirectory	equ (VirtualAddressBase >> 22) & 0x1FF
VirtualAddressBasePageTable		equ (VirtualAddressBase >> 13) & 0x1FF

section .data

; All this just creates a simple mapping for the bootstrap code
; pageTable gets reused so that it can create another mapping later

align 4096
pml4Base:
	dq (pdpt - VirtualAddressBase + 11b)
	times 255 dq 0
	dq (pdpt - VirtualAddressBase + 11b)
	times 255 dq 0

align 4096
pdpt:
	dq (pageDirectory - VirtualAddressBase + 11b)
	times 511 dq 0

align 4096
pageDirectory:
	dq pageTable - VirtualAddressBase + 11b
	times 511 dq 0

align 4096
; This maps 2 MB from address 0x0 to another address
pageTable:
	%assign i 0
	%rep 512
		dq i + 11b
		%assign i i+0x1000
	%endrep
