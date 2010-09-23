global interruptHandlers
extern interruptHandler

%macro ISRWithoutError 1
  [GLOBAL ISR%1]
  ISR%1:
    cli
    push 0
    push %1
    jmp commonInterruptHandler
%endmacro

%macro ISRWithError 1
  [GLOBAL ISR%1]
  ISR%1:
    cli
    push %1
    jmp commonInterruptHandler
%endmacro

ISRWithoutError 0
ISRWithoutError 1
ISRWithoutError 2
ISRWithoutError 3
ISRWithoutError 4
ISRWithoutError 5
ISRWithoutError 6
ISRWithoutError 7
ISRWithError   8
ISRWithoutError 9
ISRWithError 10
ISRWithError 11
ISRWithError 12
ISRWithError 13
ISRWithError 14

%assign i 15
%rep 241
	ISRWithoutError i
	%assign i i+1
%endrep

interruptHandlers:
	%assign i 0
	%rep 256
		dq ISR %+ i
		%assign i i+1
	%endrep

commonInterruptHandler:
	; Push the common registers
	push rax
	push rbx
	push rcx
	push rdx
	push rdi
	push rsi
	push rbp
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15

	mov ax, ds
	push rax

	; Load the kernel data segment descriptor
    mov ax, 0x10
    mov ds, ax
    mov ss, ax

	; Pass a pointer to the stack as a parameter
	mov rdi, rsp
	call interruptHandler

	pop rax
	mov ds, ax
	mov ss, ax

	; Restore the common state
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rbp
	pop rsi
	pop rdi
	pop rdx
	pop rcx
	pop rbx
	pop rax

    add rsp, 16				; Cleans up the pushed error code and pushed ISR number
    iretq					; Return to previous stack state
