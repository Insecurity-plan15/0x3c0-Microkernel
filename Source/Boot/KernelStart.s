GLOBAL kernelHeapStart
GLOBAL reservedGDTSpace
GLOBAL HigherHalfEntry
GLOBAL EscrowPage1
GLOBAL EscrowPage2
GLOBAL EscrowPage3
GLOBAL EscrowPage4
EXTERN Main
EXTERN invokeConstructors

; Stack setup
StackSize	equ 0x4000

HigherHalfEntry:
	; Now operating in the higher half. Setup the stack
	mov rsp, stackEnd
	mov rbp, 0

	call invokeConstructors

	push qword rbx

	push qword rsp
	call Main

	xchg bx, bx

	cli
	hlt

section .bss
	align 0x1000
		; The following parts are all neatly aligned. All the page-aligned stuff must be together,
		; or NASM will attempt to fill the gaps, making it think my code is trying to initialise a BSS
		; section with a value
		stackStart:
			resb StackSize
		stackEnd:
	align 0x1000
		EscrowPage1:
			resb 0x1000
	align 0x1000
		EscrowPage2:
			resb 0x1000
	align 0x1000
		EscrowPage3:
			resb 0x1000
	align 0x1000
		EscrowPage4:
			resb 0x1000
	kernelHeapStart:
		resb 0x60	; This is the size of the heap class. I can't find any neater way to do it
