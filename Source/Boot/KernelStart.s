GLOBAL kernelHeapStart
GLOBAL reservedGDTSpace
GLOBAL HigherHalfEntry
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
		stackStart:
			resb StackSize
		stackEnd:
	kernelHeapStart:
		resb 0x50	; This is the size of the heap class. I can't find any neater way to do it
