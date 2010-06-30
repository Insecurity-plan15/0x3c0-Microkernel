[GLOBAL SetGDT]
[GlOBAL SetIDT]
[GLOBAL SetTSS]

SetGDT:
	mov eax, [esp + 4]
	lgdt [eax]

	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	jmp 0x8:.codeFlush
.codeFlush:
	ret

SetIDT:
	mov eax, [esp+4]
	lidt [eax]
	ret

SetTSS:
	; The first parameter is the GDT index I need to load
	mov ax, [esp + 0x4]
	ltr ax
	ret
