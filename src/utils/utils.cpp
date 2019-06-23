/**
 * Debugmode Frameserver
 * Copyright (C) 2002-2019 Satish Kumar, All Rights Reserved
 * http://www.debugmode.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 * http://www.gnu.org/copyleft/gpl.html
 */

#include "utils.h"

int FPU_enabled, MMX_enabled;
#define CPUF_SUPPORTS_CPUID      (0x00000001L)
#define CPUF_SUPPORTS_FPU      (0x00000002L)
#define CPUF_SUPPORTS_MMX      (0x00000004L)

// MMX doesn't work with the amd64 build we are making.
#if defined(_M_X64)
void CpuDetect() {
  FPU_enabled = 0;
  MMX_enabled = 0;
}
void mmx_ConvertRGB32toYUY2(unsigned char *src,unsigned char *dst,int src_pitch, int dst_pitch,int w, int h) {
  // Shouldn't be called.
}
#else  // defined(_M_X64)
long __declspec(naked) CpuDetectFlag(void) {
  __asm {
    push  ebp
    push  edi
    push  esi
    push  ebx

    xor    ebp,ebp      ;cpu flags - if we don't have CPUID, we probably
                        ;won't want to try FPU optimizations.

    ;check for CPUID.

    pushfd               ;flags -> EAX
    pop    eax
    or    eax,00200000h  ;set the ID bit
    push  eax            ;EAX -> flags
    popfd
    pushfd               ;flags -> EAX
    pop    eax
    and    eax,00200000h  ;ID bit set?
    jz    done           ;nope...

    ;CPUID exists, check for features register.

    mov    ebp,00000003h
    xor    eax,eax
    cpuid
    or    eax,eax
    jz    done           ;no features register?!?

    ;features register exists, look for MMX, SSE, SSE2.

    mov    eax,1
    cpuid
    mov    ebx,edx
    and    ebx,00800000h  ;MMX is bit 23
    shr    ebx,21
    or    ebp,ebx         ;set bit 2 if MMX exists

done:
    mov    eax,ebp

    pop    ebx
    pop    esi
    pop    edi
    pop    ebp
    ret
  }
}

void CpuDetect() {
  long g_lCPUExtensions = CpuDetectFlag();

  MMX_enabled = !!(g_lCPUExtensions & CPUF_SUPPORTS_MMX);
  FPU_enabled = !!(g_lCPUExtensions & CPUF_SUPPORTS_FPU);
}

void mmx_ConvertRGB32toYUY2(unsigned char *src,unsigned char *dst,int src_pitch, int dst_pitch,int w, int h) {
  static const __int64 rgb_mask = 0x00ffffff00ffffff;
  static const __int64 fraction = 0x0000000000084000;    //= 0x108000/2 = 0x84000
  static const __int64 add_32 = 0x000000000000000020;    //= 32 shifted 15 up
  static const __int64 rb_mask = 0x0000ffff0000ffff;    //=Mask for unpacked R and B
  static const __int64 y1y2_mult = 0x0000000000004A85;
  static const __int64 fpix_add =  0x0080800000808000;
  static const __int64 fpix_mul =  0x000000282000001fb;
  static const __int64 chroma_mask =  0x000000000ff00ff00;
  static const __int64 low32_mask =  0x000000000ffffffff;
  //  const int cyb = int(0.114*219/255*32768+0.5);
  //  const int cyg = int(0.587*219/255*32768+0.5);
  //  const int cyr = int(0.299*219/255*32768+0.5);
  //  __declspec(align(8)) const __int64 cybgr_64 = (__int64)cyb|(((__int64)cyg)<<16)|(((__int64)cyr)<<32);
  const __int64 cybgr_64 = 0x000020DE40870c88;

  int lwidth_bytes = w<<2;    // Width in bytes
  src+=src_pitch*(h-1);       // ;Move source to bottom line (read top->bottom)


#define SRC eax
#define DST edi
#define RGBOFFSET ecx
#define YUVOFFSET edx

  for (int y=0;y<h;y++) {
  __asm {
    mov SRC,src
    mov DST,dst
    mov RGBOFFSET,0
    mov YUVOFFSET,0
    cmp       RGBOFFSET,[lwidth_bytes]
    jge       outloop    ; Jump out of loop if true (width==0?? - somebody brave should remove this test)
    movq mm0,[SRC+RGBOFFSET]    ; mm0= XXR2 G2B2 XXR1 G1B1
    punpcklbw mm1,mm0            ; mm1= 0000 R100 G100 B100
    movq mm4,[cybgr_64]
    align 16
re_enter:
    punpckhbw mm2,mm0        ; mm2= 0000 R200 G200 B200
    movq mm3,[fraction]
    psrlw mm1,8              ; mm1= 0000 00R1 00G1 00B1
    psrlw mm2,8              ; mm2= 0000 00R2 00G2 00B2 
    movq mm6,mm1             ; mm6= 0000 00R1 00G1 00B1 (shifter unit stall)
    pmaddwd mm1,mm4          ; mm1= v2v2 v2v2 v1v1 v1v1   y1 //(cyb*rgb[0] + cyg*rgb[1] + cyr*rgb[2] + 0x108000)
    movq mm7,mm2             ; mm7= 0000 00R2 00G2 00B2     
    movq mm0,[rb_mask]
    pmaddwd mm2,mm4          ; mm1= w2w2 w2w2 w1w1 w1w1   y2 //(cyb*rgbnext[0] + cyg*rgbnext[1] + cyr*rgbnext[2] + 0x108000)
    paddd mm1,mm3            ; Add rounding fraction
    paddw mm6,mm7            ; mm6 = accumulated RGB values (for b_y and r_y) 
    paddd mm2,mm3            ; Add rounding fraction (lower dword only)
    movq mm4,mm1
    movq mm5,mm2
    pand mm1,[low32_mask]
    psrlq mm4,32
    pand mm6,mm0             ; Clear out accumulated G-value mm6= 0000 RRRR 0000 BBBB
    pand mm2,[low32_mask]
    psrlq mm5,32
    paddd mm1,mm4            ;mm1 Contains final y1 value (shifted 15 up)
    psllq mm6, 14            ; Shift up accumulated R and B values (<<15 in C)
    paddd mm2,mm5            ;mm2 Contains final y2 value (shifted 15 up)
    psrlq mm1,15
    movq mm3,mm1
    psrlq mm2,15
    movq mm4,[add_32]
    paddd mm3,mm2            ;mm3 = y1+y2
    movq mm5,[y1y2_mult]
    psubd mm3,mm4            ; mm3 = y1+y2-32     mm0,mm4,mm5,mm7 free
    movq mm0,[fpix_add]      ; Constant that should be added to final UV pixel
    pmaddwd mm3,mm5          ; mm3=scaled_y (latency 2 cycles)
    movq mm4,[fpix_mul]      ; Constant that should be multiplied to final UV pixel    
    psllq mm2,16             ; mm2 Y2 shifted up (to clear fraction) mm2 ready
    punpckldq mm3,mm3        ; Move scaled_y to upper dword mm3=SCAL ED_Y SCAL ED_Y 
    psubd mm6,mm3            ; mm6 = b_y and r_y (stall)
    psrld mm6,9              ; Shift down b_y and r_y (>>10 in C-code) 
    por mm1,mm2              ; mm1 = 0000 0000 00Y2 00Y1
    pmaddwd mm6,mm4          ; Mult b_y and r_y 
    pxor mm2,mm2
    movq mm7,[chroma_mask]
    paddd mm6, mm0           ; Add 0x800000 to r_y and b_y 
    add RGBOFFSET,8
    psrld mm6,9              ; Move down, so fraction is only 7 bits
    cmp       RGBOFFSET,[lwidth_bytes]
    jge       outloop        ; Jump out of loop if true
    packssdw mm6,mm2         ; mm6 = 0000 0000 VVVV UUUU (7 bits fraction) (values above 0xff are saturated)
    psllq mm6,1              ; Move up, so fraction is 8 bit
    movq mm0,[SRC+RGBOFFSET] ; mm0= XXR2 G2B2 XXR1 G1B1  [From top (to get better pairing)]
    pand mm6,mm7             ; Clear out fractions
    por mm6,mm1              ; Or luma and chroma together      
    movq mm4,[cybgr_64]      ;                           [From top (to get better pairing)]
    movd [DST+YUVOFFSET],mm6 ; Store final pixel            
    punpcklbw mm1,mm0        ; mm1= 0000 R100 G100 B100
    add YUVOFFSET,4      // Two pixels (packed)
    jmp re_enter
outloop:
    // Do store without loading next pixel
    packssdw mm6,mm2         ; mm6 = 0000 0000 VVVV UUUU (7 bits fraction) (values above 0xff are saturated)
    psllq mm6,1              ; Move up, so fraction is 8 bit
    pand mm6,mm7             ; Clear out fractions
    por mm1,mm6              ; Or luma and chroma together
    movd [DST+YUVOFFSET],mm1 ; Store final pixel
    } // end asm
    src -= src_pitch;
    dst += dst_pitch;
  } // end for y
  __asm emms

#undef SRC
#undef DST
#undef RGBOFFSET
#undef YUVOFFSET
}

extern "C" void fast_memcpy(void *_dst, const void *_src, size_t _size) {
#define _SRC	esi
#define _DST	edi
#define _LEN	ebx
#define _DELTA	ecx
  __asm {
    mov		_DST, [_dst]
    mov		_SRC, [_src]
    mov		_LEN, [_size]

    prefetchnta[_SRC + 0]
    prefetchnta[_SRC + 64]
    prefetchnta[_SRC + 128]
    prefetchnta[_SRC + 192]
    prefetchnta[_SRC + 256]

    cld
    cmp			_LEN, 40h; 64 - byte blocks
    jb			L_tail

    mov			_DELTA, _DST
    and			_DELTA, 63
    jz			L_next
    xor			_DELTA, -1
    add			_DELTA, 64
    sub			_LEN, _DELTA
    rep			movsb
    L_next :
      mov			_DELTA, _LEN
      and			_LEN, 63
      shr			_DELTA, 6; len / 64
      jz			L_tail

      ALIGN 8
      L_loop:
        prefetchnta[_SRC + 320]
        movq		mm0, [_SRC + 0]
        movq		mm1, [_SRC + 8]
        movq		mm2, [_SRC + 16]
        movq		mm3, [_SRC + 24]
        movq		mm4, [_SRC + 32]
        movq		mm5, [_SRC + 40]
        movq		mm6, [_SRC + 48]
        movq		mm7, [_SRC + 56]
        movntq[_DST + 0], mm0
        movntq[_DST + 8], mm1
        movntq[_DST + 16], mm2
        movntq[_DST + 24], mm3
        movntq[_DST + 32], mm4
        movntq[_DST + 40], mm5
        movntq[_DST + 48], mm6
        movntq[_DST + 56], mm7
        add		_SRC, 64
        add		_DST, 64
        dec		_DELTA
        jnz		L_loop

      sfence
      emms

      L_tail :
        mov			_DELTA, _LEN
        rep			movsb
  }
}
#endif  // defined(_WIN64)
