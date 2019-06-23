.CODE

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; rcx = dst, rdx = src, r8 = size
fast_memcpy PROC

_DST	EQU rdi
_SRC	EQU rsi
_LEN	EQU r8
_DELTA	EQU rcx

	mov			r9, rsi
	mov			r10, rdi

	mov		_DST, rcx
	mov		_SRC, rdx

	prefetchnta	[_SRC +   0]
	prefetchnta	[_SRC +  64]
	prefetchnta	[_SRC + 128]
	prefetchnta	[_SRC + 192]
	prefetchnta	[_SRC + 256]

	cld
	cmp			_LEN, 40h				; 64-byte blocks
	jb			L_tail
				
	mov			_DELTA, _DST
	and			_DELTA, 63
	jz			L_next
	xor			_DELTA, -1
	add			_DELTA, 64
	sub			_LEN, _DELTA
	rep			movsb
L_next:
	mov			_DELTA, _LEN
	and			_LEN, 63
	shr			_DELTA, 6				; len / 64
	jz			L_tail

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

	sfence
	emms

L_tail:
	mov			_DELTA, _LEN
	rep			movsb

	mov			rsi, r9
	mov			rdi, r10
	ret
fast_memcpy ENDP

END