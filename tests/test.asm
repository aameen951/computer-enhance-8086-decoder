

bits 16

push word [BX + 0x12]
push AX
push BX
push DS
POP word [BX + 0x12]
POP AX
POP BX
POP DS

xchg AX, [0x12]
xchg BX, CX
xchg AL, [BX+0x44]
xchg [BX+0x77], AL

xchg ax, bx
xchg al, bl

in al, 10
in AX, 11
IN AL, DX
IN AX, DX
OUT 12, al
OUT 13, AX
OUT DX, AL
OUT DX, AX

xlat

lea ax, [BX+DI+12]

lds ax, [BX + 123]
les ax, [BX + 123]

lahf
sahf
pushf
popf

mov ax, bx
mov ax, [BX+SI]
mov al, [BX+SI]
mov BL, [BX + 0x04]
mov [BX+SI], ax
mov [BX+SI], al
mov [BX + 0x04], BL

mov BYTE[BX], 0x01

mov AX, 0x01
mov AL, 0x01

mov AL, [0x1]
mov AX, [0x2]
mov [0x3], AL
mov [0x4], AX

mov CS, [BX + SI + 0x1234]
mov ES, [BX + SI + 0x12]
mov SS, [0x1234]
mov DS, [SI]
mov DS, AX

mov [BX + SI + 0x1234], CS
mov [BX + SI + 0x12], ES
mov [0x1234], SS
mov [SI], DS
mov AX, DS

;;

mov cx, bx
; ;;

mov cx, bx
mov ch, ah
mov dx, bx
mov si, bx
mov bx, di
mov al, cl
mov ch, ch
mov bx, ax
mov bx, si
mov sp, di
mov bp, ax


; Register-to-register
mov si, bx
mov dh, al

; 8-bit immediate-to-register
mov cx, 12
mov cx, -12

; 16-bit immediate-to-register
mov dx, 3948
mov dx, -3948

; Source address calculation
mov al, [bx + si]
mov bx, [bp + di]
mov dx, [bp]

; Source address calculation plus 8-bit displacement
mov ah, [bx + si + 4]

; Source address calculation plus 16-bit displacement
mov al, [bx + si + 4999]

; Dest address calculation
mov [bx + di], cx
mov [bp + si], cl
mov [bp], ch


; Signed displacements
mov ax, [bx + di - 20]
mov ax, [bx + di - 120]
mov ax, [bx + di - 220]
mov ax, [bx + di - 320]
mov ax, [bx + di - 420]
mov ax, [bx + di - 520]
mov ax, [bx + di - 1520]
mov ax, [bx + di - 255]
mov ax, [bx + di - 32768]
mov ax, [bx + di + 20]
mov ax, [bx + di + 120]
mov ax, [bx + di + 220]
mov ax, [bx + di + 320]
mov ax, [bx + di + 420]
mov ax, [bx + di + 520]
mov ax, [bx + di + 1520]
mov ax, [bx + di + 255]
mov ax, [bx + di + 32767]
mov [si - 300], cx
mov dx, [bx - 32]

; Explicit sizes
mov [bp + di], byte 7
mov [di + 901], word 347

; Direct address
mov bp, [5]
mov bx, [3458]

; Memory-to-accumulator test
mov ax, [2555]
mov ax, [16]

; Accumulator-to-memory test
mov [2554], ax
mov [15], ax
