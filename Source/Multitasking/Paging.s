%ifndef Paging_S
%define Paging_S

; This file just contains the basic structures necessary to get the higher half operating
; It also stores the first page directory entries  (including recursive page mapping)

VirtualAddressBase	equ 0xF0000000
KernelPageNumber	equ (VirtualAddressBase >> 22)	; Page directory index

section .data
align 0x1000
BootPageTable:	; Contains the initial identity mapping of the first 4 MiB, and the mapping between VirtualAddressBase and 0
	%assign i 0
	%rep 1024
		dd i | 11b
		%assign i i+4096
	%endrep

align 0x1000
PageDirectory:
	dd (BootPageTable - VirtualAddressBase + 11b)
	times (KernelPageNumber - 1) dd 0
	dd (BootPageTable - VirtualAddressBase + 111b)
	; There's a -1 because the reverse mapping goes at the end
	times (1024 - KernelPageNumber - 1 - 1) dd 0
	dd (PageDirectory - VirtualAddressBase + 11b)
%endif
