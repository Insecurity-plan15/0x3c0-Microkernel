.global _start
.extern preMain
.extern main
.extern exit

_start:
    call preMain
    call main
    call exit
.wait:
    hlt
    jmp .wait
