

bits 16

; mov ax, bx
; mov ax, [BX+SI]
; mov al, [BX+SI]
; mov BL, [BX + 0x04]
; mov [BX+SI], ax
; mov [BX+SI], al
; mov [BX + 0x04], BL

; mov BYTE[BX], 0x01

; mov AX, 0x01
; mov AL, 0x01

mov AL, [0x1]
mov AX, [0x2]
mov [0x3], AL
mov [0x4], AX

; mov CS, [BX + SI + 0x1234]
; mov ES, [BX + SI + 0x12]
; mov SS, [0x1234]
; mov DS, [SI]
; mov DS, AX

; mov [BX + SI + 0x1234], CS
; mov [BX + SI + 0x12], ES
; mov [0x1234], SS
; mov [SI], DS
; mov AX, DS
