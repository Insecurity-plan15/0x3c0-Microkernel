GLOBAL MultibootHeader
GLOBAL asmEntry
GLOBAL multibootPointer
EXTERN textStart
EXTERN bssStart
EXTERN bssEnd
EXTERN HigherHalfEntry
EXTERN pml4Base

VirtualAddressBase				equ 0xFFFF800000000000

; Basic GrUB information
ModuleAlign equ 1
MemoryMap	equ 2
AOUTKludge	equ (1 << 16)
Flags		equ ModuleAlign | MemoryMap | AOUTKludge
Magic		equ 0x1BADB002
Checksum	equ -(Magic + Flags)

[BITS 32]

section .multiboot
align 0x4
	MultibootHeader:
		dd Magic
		dd Flags
		dd Checksum
		dd (MultibootHeader)	; header_addr
		dd (textStart)			; load_addr
		dd (bssStart - VirtualAddressBase)			; load_end_addr
		dd (bssEnd - VirtualAddressBase)			; bss_end_addr
		dd (asmEntry)			; entry_addr

section .pmode.gdt

align 0x8
	gdt:
		dq 0x0000000000000000	; Standard NULL entry
		dq 0x00AF9A000000FFFF	; 64-bit kernel code
		dq 0x00AF92000000FFFF	; 64-bit kernel data
		dq 0x00AFFA000000FFFF	; 64-bit user code
		dq 0x00AFF2000000FFFF	; 64-bit user data
	gdtEnd:
align 0x8
gdtPtr:
	dw gdtEnd - gdt - 1
	dq gdt

section .pmode.bss
multibootPointer:
	dw 0

section .pmode.text
; GrUB will jump here, so it needs to be 32-bit code
asmEntry:
	; First, make certain that an interrupt won't arrive and mess up the boot code
	cli

	; Next, save the multiboot pointer for later
	lea ecx, [multibootPointer]
	mov [ecx], ebx

	; Disable paging
	mov ecx, cr0
	and ecx, 0x7FFFFFFF
	mov cr0, ecx

	; Validation that the CPU supports 64-bit mode
		; Function supported must be greater than 0x80000000
		mov eax, 0x80000000
		cpuid
		cmp eax, 0x80000000
		jbe incompatible

		; Bit 29 must be set to indicate long mode compatibility
		mov eax, 0x80000001
		cpuid
		bt edx, 29
		jnc incompatible

		; Bits 19, 24, 25 and 26 of EDX must be set for SSE compatibility
		; See Intel Volume 3A, section 9.6
		mov eax, 1
		cpuid
		bt edx, 19
		jnc incompatible
		bt edx, 24
		jnc incompatible
		bt edx, 25
		jnc incompatible
		bt edx, 26
		jnc incompatible

	mov ebx, cr0

	; Enable bit 5 (PAE) in CR4
	mov ecx, cr4
	bts ecx, 5

	; Enable bit 9 (OSFXSR) in CR4
	bts ecx, 9

	; Enable bit 10 (OSXMMEXCPT) in CR4
	bts ecx, 10

	; Enable bit 1 (MP) in CR0
	bts ebx, 1

	; Disable bit 2 (EM) in CR0
	btc ebx, 2

	; Apply these changes and rejoice
	mov cr4, ecx
	mov cr0, ebx

	; Switch on long mode in the necessary MSR
	mov ecx, 0xC0000080
	rdmsr
	bts eax, 8
	wrmsr

	lea ecx, [multibootPointer]
	mov ebx, [ecx]

	mov ecx, (pml4Base - VirtualAddressBase)	; Load ECX with the physical address of the boot page directory
	mov cr3, ecx

	mov ecx, cr0
	bts ecx, 31
	mov cr0, ecx

	lgdt [gdtPtr]
	; After this jump, the CPU is in long mode
	jmp 0x8:.codeFlush
.codeFlush:

[BITS 64]
	jmp (HigherHalfEntry - VirtualAddressBase)

incompatible:
	mov eax, 0xDEAD
	mov ebx, 0xDEAD
	mov ecx, 0xDEAD
	mov edx, 0xDEAD

	hlt
