%ifndef Paging_S
%define Paging_S

; This file just contains the basic structures necessary to get the higher half operating
; It also stores the first page directory entries. The page directory entries don't have
; a recursive page mapping yet; that gets set up in C++ code

;VirtualAddressBase	equ 0xC0000000	; Not the virtual address where the kernel is loaded, just an offset to access the first GiB
VirtualAddressBase equ 0xF0000000
KernelPageNumber	equ (VirtualAddressBase >> 22)	; Page directory index

section .data
align 0x1000
BootPageTable:	; Contains the initial identity mapping of the first 4 MiB
	%assign i 0
	%rep 1024
		dd i | 111b
		%assign i i+4096
	%endrep

align 0x1000
; This page table is the same as the one above, but gets placed at a different page directory index
KernelPageTable:	; Contains the main kernel page table, mapping the first 4 MiB to 0xC000000
	%assign i 0
	%rep 1024
		dd i | 11b	; Note: at present this is set to be user-accessible. I don't want this when I load a program
		%assign i i+4096
	%endrep

align 0x1000
PageDirectory:
	dd (BootPageTable - VirtualAddressBase + 11b)
	times (KernelPageNumber - 1) dd 0
	dd (KernelPageTable - VirtualAddressBase + 111b)
	; There's a -1 because the reverse mapping goes at the end
	times (1024 - KernelPageNumber - 1 - 1) dd 0
	dd (PageDirectory - VirtualAddressBase + 11b)
%endif
