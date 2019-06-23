.CODE

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; rcx = src, rdx = dst, r8 = height, r9 = dstRowBytes
;; dword ptr [rsp + 28h] = srcRowBytes
mmx_CopyVideoFrame PROC

_DST	EQU rdi
_SRC	EQU rsi
_LEN	EQU rax
_DELTA	EQU rcx
_T		EQU rdx
_H		EQU r8
_W		EQU r9

	; save rdi, rsi
	mov		r10, _SRC
	mov		r11, _DST

	mov		_SRC, rcx
	mov		_DST, rdx

	; calculate and save result (height * dstRowBytes)
	mov		rax, _H
	imul	rax, _W
	mov		[rsp + 20h], rax

	; _T = srcRowBytes - dstRowBytes
	movsxd	_T, dword ptr [rsp + 28h]
	sub		_T, _W
	cld

	ALIGN 16
L_row:
	mov		_LEN, _W

	prefetchnta	[_SRC +   0]
	prefetchnta	[_SRC +  64]
	prefetchnta	[_SRC + 128]
	prefetchnta	[_SRC + 192]
	prefetchnta	[_SRC + 256]

	cmp		_LEN, 40h			; 64-byte blocks
	jb		L_tail
				
	mov		_DELTA, _DST
	and		_DELTA, 63
	jz		L_next
	xor		_DELTA, -1
	add		_DELTA, 64
	sub		_LEN, _DELTA
	rep		movsb
L_next:
	mov		_DELTA, _LEN
	and		_LEN, 63
	shr		_DELTA, 6			; len / 64
	jz		L_tail

	ALIGN 8
L_loop:
		prefetchnta	[_SRC + 320]
		movq		mm0, [_SRC +  0]
		movq		mm1, [_SRC +  8]
		movq		mm2, [_SRC + 16]
		movq		mm3, [_SRC + 24]
		movq		mm4, [_SRC + 32]
		movq		mm5, [_SRC + 40]
		movq		mm6, [_SRC + 48]
		movq		mm7, [_SRC + 56]
		movntq		[_DST +  0], mm0
		movntq		[_DST +  8], mm1
		movntq		[_DST + 16], mm2
		movntq		[_DST + 24], mm3
		movntq		[_DST + 32], mm4
		movntq		[_DST + 40], mm5
		movntq		[_DST + 48], mm6
		movntq		[_DST + 56], mm7
	add		_SRC, 64
	add		_DST, 64
	dec		_DELTA
	jnz		L_loop

L_tail:
	mov		_DELTA, _LEN
	rep		movsb

	add		_SRC, _T
	dec		_H
	jnz		L_row

	sfence
	emms

	; restore rdi, rsi
	mov		_SRC, r10
	mov		_DST, r11

	mov		rax, [rsp + 20h]
	ret
mmx_CopyVideoFrame ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; rcx = src, rdx = dst, r8 = height, r9 = width
;; [rsp + 28h] = rowSize
mmx_VUYx_to_YUY2 PROC

; params
_SRC	equ rsi
_DST	equ rdi
_HEIGHT	equ r8
_WIDTH	equ r9
_RS		equ r10

; vars
_SRCE	equ r11
_W		equ rdx
_RET	equ rbx

	mov			[rsp +  8], rsi
	mov			[rsp + 16], rdi
	mov			[rsp + 24], rbx

	mov			_SRC, rcx
	mov			_DST, rdx
	movsxd		_RS, dword ptr [rsp + 28h]

	mov			rax, _HEIGHT
	dec			rax
	imul		rax, _RS
	add			_SRC, rax					; src = ((height - 1) * rowSize) + src

	mov			_W, _WIDTH
	inc			_W
	shr			_W, 1
	shl			_W, 3						; _W = ((width + 1) / 2) * 2 * 4
	add			_W, _RS
	
	mov			_SRCE, _WIDTH
	shl			_SRCE, 2
	and			_SRC, -30
	add			_SRCE, _SRC					; srce = src + (width * 4) & ~0x1F

	mov			_RET, _WIDTH
	imul		_RET, _HEIGHT
	shl			_RET, 1

	ALIGN 16
L_begin:
	test		_HEIGHT, _HEIGHT
	jz			L_end
		dec			_HEIGHT
		ALIGN 16
@@:
		cmp			_SRC, _SRCE
		jge			@F
			prefetchnta	[_SRC]
			movq		mm0, [_SRC +  0]		; [A1Y1U1V1][A0Y0U0V0]
			movq		mm1, [_SRC +  8]		; [A3Y3U3V3][A2Y2U2V2]
			movq		mm2, [_SRC + 16]		; [A5Y5U5V5][A4Y4U4V4]
			movq		mm3, [_SRC + 24]		; [A7Y7U7V7][A6Y6U6V6]

			pextrw		eax, mm0, 3
			and			eax, 0FFh
			punpcklbw	mm0, mm0
			psrlw		mm0, 8
			pinsrw		mm0, eax, 3				; [00Y1 00Y0 00U0 00V0]
			pextrw		eax, mm1, 3
			and			eax, 0FFh
			punpcklbw	mm1, mm1
			psrlw		mm1, 8
			pinsrw		mm1, eax, 3				; [00Y3 00Y2 00U2 00V2]
			pextrw		eax, mm2, 3
			and			eax, 0FFh
			punpcklbw	mm2, mm2
			psrlw		mm2, 8
			pinsrw		mm2, eax, 3				; [00Y5 00Y4 00U4 00V4]
			pextrw		eax, mm3, 3
			and			eax, 0FFh
			punpcklbw	mm3, mm3
			psrlw		mm3, 8
			pinsrw		mm3, eax, 3				; [00Y7 00Y4 00U4 00V4]

			pshufw		mm0, mm0, 00110110B		; [00V0 00Y1 00U0 00Y0]
			pshufw		mm1, mm1, 00110110B		; [00V2 00Y3 00U2 00Y2]
			pshufw		mm2, mm2, 00110110B		; [00V4 00Y5 00U4 00Y4]
			pshufw		mm3, mm3, 00110110B		; [00V6 00Y7 00U6 00Y6]

			packuswb	mm0, mm1
			packuswb	mm2, mm3
			movq		[_DST + 0], mm0
			movq		[_DST + 8], mm2

			add			_SRC, 32
			add			_DST, 16
		jmp			@B
@@:
		mov			rcx, _WIDTH
		and			rcx, 7
		jz			@F
			ALIGN 16
L_loop:
			movzx		eax, word ptr [_SRC + 0]	; eax = [00 00 U0 V0]
			mov			[_DST + 1], ah				; _DST[1] = U0
			mov			[_DST + 3], al				; _DST[3] = V0
			movzx		eax, byte ptr [_SRC + 2]	; eax = [00 00 00 Y0]
			mov			[_DST + 0], al				; _DST[0] = Y0
			movzx		eax, byte ptr [_SRC + 6]	; eax = [00 00 00 Y1]
			mov			[_DST + 2], al				; _DST[2] = Y1
			add			_SRC, 8
			add			_DST, 4
		dec			rcx
		loopnz		L_loop
@@:
		sub			_SRCE, _RS
		sub			_SRC, _W
	jmp			L_begin
L_end:
	emms
	mov			rax, _RET
	mov			rsi, [rsp +  8]
	mov			rdi, [rsp + 16]
	mov			rbx, [rsp + 24]
	ret
mmx_VUYx_to_YUY2 ENDP


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; RGB32 -> RGB24
mRGB24 MACRO I
	mov		eax, [_SRC + I * 4]
	mov		word ptr [_DST + I * 3], ax
	shr		rax, 8
	mov		byte ptr [_DST + 2 + I * 3], ah
ENDM

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; rcx = src, rdx = dst, r8 = height, r9 = width
;; [rsp + 28h] = rowSize
mmx_BGRx_to_RGB24 PROC

_SRC	EQU	rsi
_DST	EQU rdi
_HEIGHT	EQU r8
_WIDTH	EQU r9

_I		EQU rcx
_RS		EQU rdx
_W1		EQU r10		; (width & 3)
_W2		EQU r11		; (width / 4)
_SCB	EQU r12		; rowSize - (width * 4)
_DCB	EQU r13		; ((width * 3 + 3) & (~3)) - (width * 4)

	mov			[rsp +  8], rdi
	mov			[rsp + 16], rsi
	mov			[rsp + 24], r12
	mov			[rsp + 32], r13

	mov			_SRC, rcx
	mov			_DST, rdx

	; _SCB = rowSize - (_WIDTH * 4)
	movsxd		_SCB, dword ptr [rsp + 28h]
	mov			rax, _WIDTH
	shl			rax, 2
	sub			_SCB, rax

	; _RS = (_WIDTH * 3 + 3) & (~3)
	; _DSB = _RS - (_WIDTH * 3)
	mov			_DCB, _WIDTH
	imul		_DCB, 3
	mov			rax, _DCB
	add			_DCB, 3
	and			_DCB, -4
	mov			_RS, _DCB
	sub			_DCB, rax

	; _RS = _RS * _HEIGHT
	imul		_RS, _HEIGHT

	; _W1 = _WIDTH & 7
	; _W2 = _WIDTH / 8
	mov			_W1, _WIDTH
	mov			_W2, _WIDTH
	and			_W1, 7
	shr			_W2, 3

	xor			_I, _I

	ALIGN 16
L_begin:
		or		_I, _W1
		jz		L_endLoop1

		ALIGN 16
@@:
		mRGB24(0)
		add		_SRC, 4
		add		_DST, 3
		dec		_I
		jnz		@B

		ALIGN 16
L_endLoop1:
		or		_I, _W2
		jz		L_endLoop2

		ALIGN 16
@@:
			prefetchnta	[_SRC]
			movq		mm0, [_SRC +  0]
			movq		mm2, [_SRC +  8]
			movq		mm4, [_SRC + 16]
			movq		mm6, [_SRc + 24]

			punpckhbw	mm1, mm0
			punpcklbw	mm0, mm0
			punpckhbw	mm3, mm2
			punpcklbw	mm2, mm2
			punpckhbw	mm5, mm4
			punpcklbw	mm4, mm4
			punpckhbw	mm7, mm6
			punpcklbw	mm6, mm6

			pextrw		eax, mm1, 0
			pinsrw		mm0, eax, 3
			psrlq		mm1, 16
			punpckldq	mm1, mm2
			pextrw		eax, mm2, 2
			psllq		mm3, 16
			pinsrw		mm3, eax, 0

			pextrw		eax, mm5, 0
			pinsrw		mm4, eax, 3
			psrlq		mm5, 16
			punpckldq	mm5, mm6
			pextrw		eax, mm6, 2
			psllq		mm7, 16
			pinsrw		mm7, eax, 0

			psrlw		mm0, 8
			psrlw		mm1, 8
			psrlw		mm3, 8
			psrlw		mm4, 8
			psrlw		mm5, 8
			psrlw		mm7, 8

			packuswb	mm0, mm1
			packuswb	mm3, mm4
			packuswb	mm5, mm7

			movq		[_DST +  0], mm0
			movq		[_DST +  8], mm3
			movq		[_DST + 16], mm5

			add			_SRC, 32
			add			_DST, 24
		dec		_I
		jnz		@B
L_endLoop2:
	add		_SRC, _SCB
	add		_DST, _DCB
	dec		_HEIGHT
	jnz		L_begin
L_exit:
	mov		rax, _RS
	emms

	mov			rdi, [rsp +  8]
	mov			rsi, [rsp + 16]
	mov			r12, [rsp + 24]
	mov			r13, [rsp + 32]
	ret
mmx_BGRx_to_RGB24 ENDP

END