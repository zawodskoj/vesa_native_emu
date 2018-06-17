org 7c00h

mov ax, 4f02h
mov bx, 101h
int 10h
mov ax, 0x0f0f
mov bx, 0xa000
mov es, bx
mov [es:0x300], ax

jmp $

times 1feh - ($ - $$) db 0
db 55h, 0aah
times 10000000h - 200h db 0
