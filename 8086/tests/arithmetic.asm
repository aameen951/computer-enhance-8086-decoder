
bits 16


ADD cx, bx
ADD cl, bl
ADD cx, [BX + 12]
ADD cl, [BX + 12]
ADD [BX + 12], cx
ADD [BX + 12], cl
ADD cx, 1
ADD cl, 1
ADD BYTE [BX], 1
ADD WORD [BX], 1
ADD cx, -1
ADD cl, -1
ADD cx, 500
ADD ax, 1
ADD al, 2

ADC cx, bx
ADC cl, bl
ADC cx, [BX + 12]
ADC cl, [BX + 12]
ADC [BX + 12], cx
ADC [BX + 12], cl
ADC cx, 1
ADC cl, 1
ADC cx, -1
ADC cl, -1
ADC cx, 500
ADC ax, 1
ADC al, 2
