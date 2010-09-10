[GlOBAL SetIDT]
[GLOBAL SetTSS]

SetIDT:
	mov eax, [esp+4]
	lidt [eax]
	ret

SetTSS:
	; The first parameter is the GDT index I need to load
	mov ax, [esp + 0x4]
	ltr ax
	ret
