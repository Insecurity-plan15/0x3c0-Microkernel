GLOBAL asmEntry
GLOBAL kernelHeapStart
GLOBAL reservedGDTSpace
EXTERN Main
EXTERN invokeConstructors

%include "Multitasking/Paging.s"

; Basic GrUB information
ModuleAlign equ 1
MemoryMap	equ 2
Flags		equ ModuleAlign | MemoryMap
Magic		equ 0x1BADB002
Checksum	equ -(Magic + Flags)

; Stack setup
StackSize	equ 0x4000

section .text
align 0x4
MultibootHeader:
	dd Magic
	dd Flags
	dd Checksum

asmEntry:
	mov ecx, (PageDirectory - VirtualAddressBase)	; Load ECX with the physical address of the boot page directory
	mov cr3, ecx

	mov ecx, cr0
	or ecx, 10000000000000000000000000000000b	; Enable paging by setting the PG bit
	mov cr0, ecx

	lea ecx, [HigherHalfEntry]
	jmp ecx

HigherHalfEntry:
	; Unmap the first page table
	mov dword [PageDirectory], 0
	invlpg [0]

	; Now operating in the higher half. Setup the stack
	mov esp, stackEnd
	mov ebp, 0

	call invokeConstructors

	; Correct the multiboot address to make it virtual
	add ebx, VirtualAddressBase
UpdateMultibootFields:
	; CommandLine
	add dword [ebx+16], VirtualAddressBase
	; Modules
	add dword [ebx+24], VirtualAddressBase
	; MemoryMapAddress
	add dword [ebx+48], VirtualAddressBase
	; DrivesAddress
	add dword [ebx+56], VirtualAddressBase
	; ConfigTable
	add dword [ebx+60], VirtualAddressBase
	; BootloaderName
	add dword [ebx+64], VirtualAddressBase
	; ApmTable
	add dword [ebx+68], VirtualAddressBase
	; VBEControlInformation
	add dword [ebx+72], VirtualAddressBase
	; VBEModeInformation
	add dword [ebx+76], VirtualAddressBase

	push ebx

	push esp
	call Main

	xchg bx, bx

	cli
	hlt

section .bss
align 0x1000
stackStart:
	resb StackSize
stackEnd:
kernelHeapStart:
	resb 0x50	; This is the size of the heap class. I can't find any neater way to do it
reservedGDTSpace:
	resb 0x30	; This is the size of the GDT. Again, I can't think of a neater way
