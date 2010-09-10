[GLOBAL SetTSS]

SetTSS:
	; The first parameter is the GDT index I need to load
	mov ax, [esp + 0x4]
	ltr ax
	ret
