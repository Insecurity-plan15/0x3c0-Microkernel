%macro ISRWithoutError 1
  [GLOBAL ISR%1]
  ISR%1:
    cli
    push byte 0
    push byte %1
    jmp commonExceptionHandler
%endmacro

%macro ISRWithError 1
  [GLOBAL ISR%1]
  ISR%1:
    cli
    push byte %1
    jmp commonExceptionHandler
%endmacro

%macro IRQ 2
  [GLOBAL IRQ%1]
  IRQ%1:
    cli
    push byte 0
    push byte %2
    jmp commonIRQHandler
%endmacro

%assign i 0
%rep 8
	ISRWithoutError i
	%assign i i+1
%endrep
ISRWithError   8
ISRWithoutError 9
%assign i 10
%rep 5
	ISRWithError i
	%assign i i+1
%endrep
%assign i 15
%rep 17
	ISRWithoutError i
	%assign i i+1
%endrep

%assign i 0
%rep 16
	IRQ i, i+32
	%assign i i+1
%endrep

; Both system call interfaces - native and POSIX
ISRWithoutError	48
ISRWithoutError 64

extern exceptionHandler
extern irqHandler

commonExceptionHandler:
;    pusha					; Pushes edi, esi, ebp, esp, ebx, edx, ecx, eax

;    mov ax, ds				; Lower 16-bits of eax = ds.
;    push eax				; save the data segment descriptor

;    mov ax, 0x10			; load the kernel data segment descriptor
;    mov ds, ax
;    mov es, ax
;    mov fs, ax
;    mov gs, ax

;	mov ebp, esp
;	push ebp
;    call exceptionHandler
;    mov esp, eax

;    pop eax					; reload the original data segment descriptor
;    mov ds, ax
;    mov es, ax
;    mov fs, ax
;    mov gs, ax

;    popa					; Pops edi, esi, ebp, esp, ebx, edx, ecx, eax
;    add esp, 8				; Cleans up the pushed error code and pushed ISR number
    iret					; pops 5 things at once: CS, EIP, EFLAGS, SS, and ESP

commonIRQHandler:
;	pusha					; Pushes edi, esi, ebp, esp, ebx, edx, ecx, eax

;	mov ax, ds				; Lower 16-bits of eax = ds.
;	push eax				; save the data segment descriptor

;	mov ax, 0x10			; load the kernel data segment descriptor
;	mov ds, ax
;	mov es, ax
;	mov fs, ax
;	mov gs, ax

;	mov ebp, esp
;	push ebp
;	call irqHandler
;	mov esp, eax

;	pop ebx					; reload the original data segment descriptor
;	mov ds, bx
;	mov es, bx
;	mov fs, bx
;	mov gs, bx

;	popa					; Pops edi, esi, ebp, esp, ebx, edx, ecx, eax
;	add esp, 8				; Cleans up the pushed error code and pushed ISR number
;	iret					; pops 5 things at once: CS, EIP, EFLAGS, SS, and ESP
