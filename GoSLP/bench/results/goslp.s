	.build_version macos, 13, 3	sdk_version 13, 3
	.section	__TEXT,__literal16,16byte_literals
	.p2align	4, 0x0                          ; -- Begin function main
lCPI0_0:
	.long	0                               ; 0x0
	.long	1                               ; 0x1
	.long	2                               ; 0x2
	.long	3                               ; 0x3
	.section	__TEXT,__text,regular,pure_instructions
	.globl	_main
	.p2align	2
_main:                                  ; @main
Lfunc_begin0:
	.cfi_startproc
	.cfi_personality 155, ___gxx_personality_v0
	.cfi_lsda 16, Lexception0
; %bb.0:
	sub	sp, sp, #224
	stp	d9, d8, [sp, #112]              ; 16-byte Folded Spill
	stp	x28, x27, [sp, #128]            ; 16-byte Folded Spill
	stp	x26, x25, [sp, #144]            ; 16-byte Folded Spill
	stp	x24, x23, [sp, #160]            ; 16-byte Folded Spill
	stp	x22, x21, [sp, #176]            ; 16-byte Folded Spill
	stp	x20, x19, [sp, #192]            ; 16-byte Folded Spill
	stp	x29, x30, [sp, #208]            ; 16-byte Folded Spill
	add	x29, sp, #208
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w21, -40
	.cfi_offset w22, -48
	.cfi_offset w23, -56
	.cfi_offset w24, -64
	.cfi_offset w25, -72
	.cfi_offset w26, -80
	.cfi_offset w27, -88
	.cfi_offset w28, -96
	.cfi_offset b8, -104
	.cfi_offset b9, -112
	cmp	w0, #2
	b.lt	LBB0_3
; %bb.1:
	ldr	x26, [x1, #8]
	cmp	w0, #2
	b.ne	LBB0_31
; %bb.2:
	mov	w24, #8192                      ; =0x2000
	b	LBB0_4
LBB0_3:
	mov	w24, #8192                      ; =0x2000
Lloh0:
	adrp	x26, l_.str@PAGE
Lloh1:
	add	x26, x26, l_.str@PAGEOFF
LBB0_4:
	mov	w19, #2000                      ; =0x7d0
LBB0_5:
	cmp	w19, #1
	csinc	w28, w19, wzr, gt
	mov	w8, #8                          ; =0x8
	cmp	w24, #8
	csel	w19, w24, w8, gt
	and	w25, w19, #0x7ffffff8
	ubfiz	x27, x25, #2, #32
	mov	x0, x27
	bl	__Znwm
	mov	x21, x0
	mov	x1, x27
	bl	_bzero
Ltmp0:
	mov	x0, x27
	bl	__Znwm
Ltmp1:
; %bb.6:
	mov	x22, x0
	mov	x1, x27
	bl	_bzero
Ltmp3:
	mov	x0, x27
	bl	__Znwm
Ltmp4:
; %bb.7:
	mov	x23, x0
	mov	x1, x27
	bl	_bzero
	mov	x8, #0                          ; =0x0
	mov	w9, #32431                      ; =0x7eaf
	movk	w9, #20944, lsl #16
	dup.4s	v0, w9
	movi.4s	v1, #97
	mov	x9, #5243                       ; =0x147b
	movk	x9, #18350, lsl #16
	movk	x9, #31457, lsl #32
	movk	x9, #16260, lsl #48
	dup.2d	v2, x9
	mov	w9, #737                        ; =0x2e1
	movk	w9, #47127, lsl #16
	dup.4s	v3, w9
	mov	x9, #5243                       ; =0x147b
	movk	x9, #18350, lsl #16
	movk	x9, #31457, lsl #32
	movk	x9, #16276, lsl #48
	dup.2d	v4, x9
	mov	w9, #46153                      ; =0xb449
	movk	w9, #59074, lsl #16
	dup.4s	v5, w9
	mov	x9, #7864                       ; =0x1eb8
	movk	x9, #60293, lsl #16
	movk	x9, #47185, lsl #32
	movk	x9, #16286, lsl #48
	dup.2d	v6, x9
Lloh2:
	adrp	x9, lCPI0_0@PAGE
Lloh3:
	ldr	q7, [x9, lCPI0_0@PAGEOFF]
	lsl	x9, x19, #2
	and	x9, x9, #0x3ffffffe0
	fmov.2d	v16, #1.00000000
	movi.4s	v17, #89
	fmov.2d	v18, #0.50000000
	movi.4s	v19, #71
	fmov.2d	v20, #1.50000000
	movi.4s	v21, #4
LBB0_8:                                 ; =>This Inner Loop Header: Depth=1
	umull2.2d	v22, v7, v0
	umull.2d	v23, v7, v0
	uzp2.4s	v22, v23, v22
	sub.4s	v23, v7, v22
	usra.4s	v22, v23, #1
	ushr.4s	v22, v22, #6
	mov.16b	v23, v7
	mls.4s	v23, v22, v1
	ushll2.2d	v22, v23, #0
	ushll.2d	v23, v23, #0
	ucvtf.2d	v22, v22
	ucvtf.2d	v23, v23
	fmul.2d	v23, v23, v2
	umull2.2d	v24, v7, v3
	umull.2d	v25, v7, v3
	uzp2.4s	v24, v25, v24
	fmul.2d	v22, v22, v2
	ushr.4s	v24, v24, #6
	mov.16b	v25, v7
	mls.4s	v25, v24, v17
	ushll2.2d	v24, v25, #0
	ucvtf.2d	v24, v24
	fadd.2d	v22, v22, v16
	ushll.2d	v25, v25, #0
	ucvtf.2d	v25, v25
	fmul.2d	v25, v25, v4
	fmul.2d	v24, v24, v4
	fadd.2d	v24, v24, v18
	fadd.2d	v23, v23, v16
	fadd.2d	v25, v25, v18
	fcvtn	v25.2s, v25.2d
	fcvtn2	v25.4s, v24.2d
	umull2.2d	v24, v7, v5
	str	q25, [x22, x8]
	umull.2d	v25, v7, v5
	uzp2.4s	v24, v25, v24
	ushr.4s	v24, v24, #6
	mov.16b	v25, v7
	mls.4s	v25, v24, v19
	fcvtn	v23.2s, v23.2d
	ushll2.2d	v24, v25, #0
	ucvtf.2d	v24, v24
	ushll.2d	v25, v25, #0
	ucvtf.2d	v25, v25
	fmul.2d	v25, v25, v6
	fcvtn2	v23.4s, v22.2d
	fmul.2d	v22, v24, v6
	fadd.2d	v22, v22, v20
	fadd.2d	v24, v25, v20
	fcvtn	v24.2s, v24.2d
	str	q23, [x21, x8]
	fcvtn2	v24.4s, v22.2d
	str	q24, [x23, x8]
	add.4s	v7, v7, v21
	add	x8, x8, #16
	cmp	x9, x8
	b.ne	LBB0_8
; %bb.9:
	bl	__ZNSt3__16chrono12steady_clock3nowEv
	str	x0, [sp, #64]                   ; 8-byte Folded Spill
Lloh4:
	adrp	x1, l_.str@PAGE
Lloh5:
	add	x1, x1, l_.str@PAGEOFF
	mov	x0, x26
	bl	_strcmp
	cbz	w0, LBB0_16
; %bb.10:
Lloh6:
	adrp	x1, l_.str.1@PAGE
Lloh7:
	add	x1, x1, l_.str.1@PAGEOFF
	mov	x0, x26
	bl	_strcmp
	cbz	w0, LBB0_33
; %bb.11:
Lloh8:
	adrp	x1, l_.str.2@PAGE
Lloh9:
	add	x1, x1, l_.str.2@PAGEOFF
	mov	x0, x26
	bl	_strcmp
	cbz	w0, LBB0_48
; %bb.12:
Lloh10:
	adrp	x1, l_.str.3@PAGE
Lloh11:
	add	x1, x1, l_.str.3@PAGEOFF
	mov	x0, x26
	bl	_strcmp
	cbz	w0, LBB0_57
; %bb.13:
Lloh12:
	adrp	x1, l_.str.4@PAGE
Lloh13:
	add	x1, x1, l_.str.4@PAGEOFF
	mov	x0, x26
	bl	_strcmp
	cbz	w0, LBB0_72
; %bb.14:
Lloh14:
	adrp	x1, l_.str.5@PAGE
Lloh15:
	add	x1, x1, l_.str.5@PAGEOFF
	mov	x0, x26
	bl	_strcmp
	cbz	w0, LBB0_87
; %bb.15:
Lloh16:
	adrp	x8, ___stderrp@GOTPAGE
Lloh17:
	ldr	x8, [x8, ___stderrp@GOTPAGEOFF]
Lloh18:
	ldr	x0, [x8]
	str	x26, [sp]
Lloh19:
	adrp	x1, l_.str.6@PAGE
Lloh20:
	add	x1, x1, l_.str.6@PAGEOFF
	bl	_fprintf
	mov	w20, #1                         ; =0x1
	b	LBB0_107
LBB0_16:                                ; %.preheader26.preheader
	stp	x27, x26, [sp, #72]             ; 16-byte Folded Spill
	mov	x27, x19
	mov	w19, #0                         ; =0x0
	movi	d8, #0000000000000000
LBB0_17:                                ; %.preheader26
                                        ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB0_18 Depth 2
	mov	x20, #0                         ; =0x0
	movi	d9, #0000000000000000
	mov	x26, x21
LBB0_18:                                ;   Parent Loop BB0_17 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	mov	x0, x26
	bl	__ZL16array_sum_block8PKf
	fadd	s9, s9, s0
	add	x20, x20, #8
	add	x26, x26, #32
	cmp	x20, x25
	b.lo	LBB0_18
; %bb.19:                               ;   in Loop: Header=BB0_17 Depth=1
	fcvt	d0, s9
	fadd	d8, d8, d0
	add	w19, w19, #1
	cmp	w19, w28
	b.ne	LBB0_17
; %bb.20:
	mov	w8, #0                          ; =0x0
	and	x9, x25, #0x7ffffff0
	add	x10, x21, #32
	and	x11, x25, #0xf
	ubfiz	x12, x27, #2, #32
	and	x12, x12, #0x3ffffffc0
	add	x12, x21, x12
	and	x13, x27, #0xfffffff8
	neg	x13, x13
	movi	d9, #0000000000000000
	ldr	x27, [sp, #72]                  ; 8-byte Folded Reload
	b	LBB0_22
LBB0_21:                                ; %.loopexit
                                        ;   in Loop: Header=BB0_22 Depth=1
	fcvt	d0, s0
	fadd	d9, d9, d0
	add	w8, w8, #1
	cmp	w8, w28
	b.eq	LBB0_103
LBB0_22:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB0_27 Depth 2
                                        ;     Child Loop BB0_30 Depth 2
                                        ;     Child Loop BB0_25 Depth 2
	cmp	w24, #16
	b.ge	LBB0_26
; %bb.23:                               ;   in Loop: Header=BB0_22 Depth=1
	mov	x16, #0                         ; =0x0
	movi	d0, #0000000000000000
LBB0_24:                                ; %.preheader252.preheader
                                        ;   in Loop: Header=BB0_22 Depth=1
	add	x14, x13, x16
	add	x15, x21, x16, lsl #2
LBB0_25:                                ; %.preheader252
                                        ;   Parent Loop BB0_22 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	q1, [x15], #16
	mov	s2, v1[3]
	mov	s3, v1[2]
	mov	s4, v1[1]
	fadd	s0, s0, s1
	fadd	s0, s0, s4
	fadd	s0, s0, s3
	fadd	s0, s0, s2
	adds	x14, x14, #4
	b.ne	LBB0_25
	b	LBB0_21
LBB0_26:                                ; %.preheader24.preheader
                                        ;   in Loop: Header=BB0_22 Depth=1
	movi	d0, #0000000000000000
	mov	x14, x10
	mov	x15, x9
LBB0_27:                                ; %.preheader24
                                        ;   Parent Loop BB0_22 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldp	q1, q2, [x14, #-32]
	mov	s3, v1[3]
	mov	s4, v1[2]
	mov	s5, v1[1]
	mov	s6, v2[3]
	mov	s7, v2[2]
	mov	s16, v2[1]
	ldp	q17, q18, [x14], #64
	mov	s19, v17[3]
	mov	s20, v17[2]
	mov	s21, v17[1]
	mov	s22, v18[3]
	mov	s23, v18[2]
	mov	s24, v18[1]
	fadd	s0, s0, s1
	fadd	s0, s0, s5
	fadd	s0, s0, s4
	fadd	s0, s0, s3
	fadd	s0, s0, s2
	fadd	s0, s0, s16
	fadd	s0, s0, s7
	fadd	s0, s0, s6
	fadd	s0, s0, s17
	fadd	s0, s0, s21
	fadd	s0, s0, s20
	fadd	s0, s0, s19
	fadd	s0, s0, s18
	fadd	s0, s0, s24
	fadd	s0, s0, s23
	fadd	s0, s0, s22
	subs	x15, x15, #16
	b.ne	LBB0_27
; %bb.28:                               ;   in Loop: Header=BB0_22 Depth=1
	cmp	x25, x9
	b.eq	LBB0_21
; %bb.29:                               ;   in Loop: Header=BB0_22 Depth=1
	mov	x16, x9
	mov	x14, x12
	mov	x15, x11
	tbnz	w25, #3, LBB0_24
LBB0_30:                                ; %.preheader
                                        ;   Parent Loop BB0_22 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	s1, [x14], #4
	fadd	s0, s0, s1
	subs	x15, x15, #1
	b.ne	LBB0_30
	b	LBB0_21
LBB0_31:
	mov	x20, x1
	ldr	x8, [x1, #16]
	mov	x21, x0
	mov	x0, x8
	bl	_atoi
	mov	x19, x0
	cmp	w21, #4
	b.lo	LBB0_56
; %bb.32:
	ldr	x0, [x20, #24]
	bl	_atoi
	mov	x24, x0
	b	LBB0_5
LBB0_33:                                ; %.preheader32.preheader
	str	x19, [sp, #56]                  ; 8-byte Folded Spill
	stp	x27, x26, [sp, #72]             ; 16-byte Folded Spill
	mov	w19, #0                         ; =0x0
	movi	d8, #0000000000000000
LBB0_34:                                ; %.preheader32
                                        ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB0_35 Depth 2
	mov	x20, #0                         ; =0x0
	movi	d9, #0000000000000000
	mov	x26, x21
	mov	x27, x22
LBB0_35:                                ;   Parent Loop BB0_34 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	mov	x0, x26
	mov	x1, x27
	bl	__ZL18dot_product_block8PKfS0_
	fadd	s9, s9, s0
	add	x20, x20, #8
	add	x27, x27, #32
	add	x26, x26, #32
	cmp	x20, x25
	b.lo	LBB0_35
; %bb.36:                               ;   in Loop: Header=BB0_34 Depth=1
	fcvt	d0, s9
	fadd	d8, d8, d0
	add	w19, w19, #1
	cmp	w19, w28
	b.ne	LBB0_34
; %bb.37:
	mov	w8, #0                          ; =0x0
	and	x9, x25, #0x7ffffff0
	add	x10, x21, #32
	add	x11, x22, #32
	and	x12, x25, #0xf
	ldr	x15, [sp, #56]                  ; 8-byte Folded Reload
	ubfiz	x13, x15, #2, #32
	and	x14, x13, #0x3ffffffc0
	add	x13, x22, x14
	add	x14, x21, x14
	and	x15, x15, #0xfffffff8
	neg	x15, x15
	movi	d9, #0000000000000000
	ldr	x27, [sp, #72]                  ; 8-byte Folded Reload
	b	LBB0_39
LBB0_38:                                ; %.loopexit27
                                        ;   in Loop: Header=BB0_39 Depth=1
	fcvt	d0, s0
	fadd	d9, d9, d0
	add	w8, w8, #1
	cmp	w8, w28
	b.eq	LBB0_103
LBB0_39:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB0_44 Depth 2
                                        ;     Child Loop BB0_47 Depth 2
                                        ;     Child Loop BB0_42 Depth 2
	cmp	w24, #16
	b.ge	LBB0_43
; %bb.40:                               ;   in Loop: Header=BB0_39 Depth=1
	mov	x1, #0                          ; =0x0
	movi	d0, #0000000000000000
LBB0_41:                                ; %.preheader260.preheader
                                        ;   in Loop: Header=BB0_39 Depth=1
	add	x16, x15, x1
	lsl	x0, x1, #2
	add	x17, x22, x0
	add	x0, x21, x0
LBB0_42:                                ; %.preheader260
                                        ;   Parent Loop BB0_39 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	q1, [x0], #16
	ldr	q2, [x17], #16
	fmul.4s	v1, v1, v2
	mov	s2, v1[3]
	mov	s3, v1[2]
	mov	s4, v1[1]
	fadd	s0, s0, s1
	fadd	s0, s0, s4
	fadd	s0, s0, s3
	fadd	s0, s0, s2
	adds	x16, x16, #4
	b.ne	LBB0_42
	b	LBB0_38
LBB0_43:                                ; %.preheader30.preheader
                                        ;   in Loop: Header=BB0_39 Depth=1
	movi	d0, #0000000000000000
	mov	x16, x11
	mov	x17, x10
	mov	x0, x9
LBB0_44:                                ; %.preheader30
                                        ;   Parent Loop BB0_39 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldp	q1, q2, [x17, #-32]
	ldp	q3, q4, [x17], #64
	ldp	q5, q6, [x16, #-32]
	ldp	q7, q16, [x16], #64
	fmul.4s	v1, v1, v5
	mov	s5, v1[3]
	mov	s17, v1[2]
	mov	s18, v1[1]
	fmul.4s	v2, v2, v6
	mov	s6, v2[3]
	mov	s19, v2[2]
	mov	s20, v2[1]
	fmul.4s	v3, v3, v7
	mov	s7, v3[3]
	mov	s21, v3[2]
	mov	s22, v3[1]
	fmul.4s	v4, v4, v16
	mov	s16, v4[3]
	mov	s23, v4[2]
	mov	s24, v4[1]
	fadd	s0, s0, s1
	fadd	s0, s0, s18
	fadd	s0, s0, s17
	fadd	s0, s0, s5
	fadd	s0, s0, s2
	fadd	s0, s0, s20
	fadd	s0, s0, s19
	fadd	s0, s0, s6
	fadd	s0, s0, s3
	fadd	s0, s0, s22
	fadd	s0, s0, s21
	fadd	s0, s0, s7
	fadd	s0, s0, s4
	fadd	s0, s0, s24
	fadd	s0, s0, s23
	fadd	s0, s0, s16
	subs	x0, x0, #16
	b.ne	LBB0_44
; %bb.45:                               ;   in Loop: Header=BB0_39 Depth=1
	cmp	x25, x9
	b.eq	LBB0_38
; %bb.46:                               ;   in Loop: Header=BB0_39 Depth=1
	mov	x1, x9
	mov	x16, x14
	mov	x17, x13
	mov	x0, x12
	tbnz	w25, #3, LBB0_41
LBB0_47:                                ; %.preheader28
                                        ;   Parent Loop BB0_39 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	s1, [x16], #4
	ldr	s2, [x17], #4
	fmul	s1, s1, s2
	fadd	s0, s0, s1
	subs	x0, x0, #1
	b.ne	LBB0_47
	b	LBB0_38
LBB0_48:                                ; %.preheader35.preheader
	mov	w19, #0                         ; =0x0
	movi	d8, #0000000000000000
LBB0_49:                                ; %.preheader35
                                        ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB0_50 Depth 2
	mov	x20, #0                         ; =0x0
	str	wzr, [sp, #104]
	str	wzr, [sp, #96]
	mov	x24, x21
LBB0_50:                                ;   Parent Loop BB0_49 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	s0, [sp, #104]
	add	x1, sp, #104
	add	x2, sp, #96
	mov	x0, x24
	bl	__ZL18rolling_sum_block8PKffRfS1_
	add	x20, x20, #8
	add	x24, x24, #32
	cmp	x20, x25
	b.lo	LBB0_50
; %bb.51:                               ;   in Loop: Header=BB0_49 Depth=1
	ldr	s0, [sp, #96]
	ldr	s1, [sp, #104]
	fadd	s0, s0, s1
	fcvt	d0, s0
	fadd	d8, d8, d0
	add	w19, w19, #1
	cmp	w19, w28
	b.ne	LBB0_49
; %bb.52:                               ; %.preheader33.preheader
	mov	w8, #0                          ; =0x0
	movi	d9, #0000000000000000
LBB0_53:                                ; %.preheader33
                                        ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB0_54 Depth 2
	movi	d0, #0000000000000000
	mov	x9, x21
	mov	x10, x25
	movi	d1, #0000000000000000
LBB0_54:                                ;   Parent Loop BB0_53 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	s2, [x9], #4
	fadd	s1, s1, s2
	fadd	s0, s0, s1
	subs	x10, x10, #1
	b.ne	LBB0_54
; %bb.55:                               ;   in Loop: Header=BB0_53 Depth=1
	fadd	s0, s1, s0
	fcvt	d0, s0
	fadd	d9, d9, d0
	add	w8, w8, #1
	cmp	w8, w28
	b.ne	LBB0_53
	b	LBB0_104
LBB0_56:
	mov	w24, #8192                      ; =0x2000
	b	LBB0_5
LBB0_57:                                ; %.preheader41.preheader
	str	x19, [sp, #56]                  ; 8-byte Folded Spill
	stp	x27, x26, [sp, #72]             ; 16-byte Folded Spill
	mov	w19, #0                         ; =0x0
	movi	d8, #0000000000000000
LBB0_58:                                ; %.preheader41
                                        ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB0_59 Depth 2
	mov	x20, #0                         ; =0x0
	stp	xzr, xzr, [sp, #96]
	mov	x26, x21
	mov	x27, x23
LBB0_59:                                ;   Parent Loop BB0_58 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	add	x2, sp, #104
	add	x3, sp, #96
	mov	x0, x26
	mov	x1, x27
	bl	__ZL11vwap_block8PKfS0_RdS1_
	add	x20, x20, #8
	add	x27, x27, #32
	add	x26, x26, #32
	cmp	x20, x25
	b.lo	LBB0_59
; %bb.60:                               ;   in Loop: Header=BB0_58 Depth=1
	ldp	d1, d0, [sp, #96]
	fdiv	d0, d0, d1
	fadd	d8, d8, d0
	add	w19, w19, #1
	cmp	w19, w28
	b.ne	LBB0_58
; %bb.61:
	mov	w8, #0                          ; =0x0
	and	x9, x25, #0x7ffffff0
	add	x10, x21, #32
	add	x11, x23, #32
	and	x12, x25, #0xf
	ldr	x15, [sp, #56]                  ; 8-byte Folded Reload
	ubfiz	x13, x15, #2, #32
	and	x14, x13, #0x3ffffffc0
	add	x13, x23, x14
	add	x14, x21, x14
	and	x15, x15, #0xfffffff8
	neg	x15, x15
	movi	d9, #0000000000000000
	ldr	x27, [sp, #72]                  ; 8-byte Folded Reload
	b	LBB0_63
LBB0_62:                                ; %.loopexit36
                                        ;   in Loop: Header=BB0_63 Depth=1
	fdiv	d0, d0, d1
	fadd	d9, d9, d0
	add	w8, w8, #1
	cmp	w8, w28
	b.eq	LBB0_103
LBB0_63:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB0_68 Depth 2
                                        ;     Child Loop BB0_71 Depth 2
                                        ;     Child Loop BB0_66 Depth 2
	cmp	w24, #16
	b.ge	LBB0_67
; %bb.64:                               ;   in Loop: Header=BB0_63 Depth=1
	mov	x1, #0                          ; =0x0
	movi	d1, #0000000000000000
	movi	d0, #0000000000000000
LBB0_65:                                ; %.preheader276.preheader
                                        ;   in Loop: Header=BB0_63 Depth=1
	add	x16, x15, x1
	lsl	x0, x1, #2
	add	x17, x23, x0
	add	x0, x21, x0
LBB0_66:                                ; %.preheader276
                                        ;   Parent Loop BB0_63 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	q2, [x0], #16
	fcvtl	v3.2d, v2.2s
	ldr	q4, [x17], #16
	fcvtl2	v2.2d, v2.4s
	fcvtl2	v5.2d, v4.4s
	mov	d6, v5[1]
	fcvtl	v4.2d, v4.2s
	mov	d7, v4[1]
	fmul.2d	v2, v2, v5
	mov	d16, v2[1]
	fmul.2d	v3, v3, v4
	mov	d17, v3[1]
	fadd	d1, d1, d4
	fadd	d1, d1, d7
	fadd	d1, d1, d5
	fadd	d1, d1, d6
	fadd	d0, d0, d3
	fadd	d0, d0, d17
	fadd	d0, d0, d2
	fadd	d0, d0, d16
	adds	x16, x16, #4
	b.ne	LBB0_66
	b	LBB0_62
LBB0_67:                                ; %.preheader39.preheader
                                        ;   in Loop: Header=BB0_63 Depth=1
	movi	d1, #0000000000000000
	mov	x16, x11
	mov	x17, x10
	mov	x0, x9
	movi	d0, #0000000000000000
LBB0_68:                                ; %.preheader39
                                        ;   Parent Loop BB0_63 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldp	q3, q4, [x17, #-32]
	ldp	q16, q2, [x17], #64
	fcvtl	v17.2d, v3.2s
	fcvtl2	v7.2d, v3.4s
	fcvtl	v6.2d, v4.2s
	fcvtl2	v5.2d, v4.4s
	fcvtl	v4.2d, v16.2s
	fcvtl2	v3.2d, v16.4s
	ldp	q18, q16, [x16, #-32]
	fcvtl2	v19.2d, v18.4s
	fcvtl	v18.2d, v18.2s
	fmul.2d	v17, v17, v18
	fadd	d1, d1, d18
	mov	d18, v18[1]
	fadd	d1, d1, d18
	mov	d18, v19[1]
	fmul.2d	v7, v7, v19
	fadd	d1, d1, d19
	fcvtl	v19.2d, v16.2s
	fadd	d1, d1, d18
	mov	d18, v19[1]
	fmul.2d	v6, v6, v19
	fadd	d1, d1, d19
	fadd	d1, d1, d18
	ldp	q18, q19, [x16], #64
	fcvtl2	v16.2d, v16.4s
	fmul.2d	v5, v5, v16
	fadd	d1, d1, d16
	mov	d16, v16[1]
	fadd	d1, d1, d16
	fcvtl	v16.2d, v18.2s
	fmul.2d	v4, v4, v16
	fadd	d1, d1, d16
	mov	d16, v16[1]
	fadd	d1, d1, d16
	fcvtl	v16.2d, v2.2s
	fcvtl2	v18.2d, v18.4s
	fmul.2d	v3, v3, v18
	fadd	d1, d1, d18
	mov	d18, v18[1]
	fadd	d1, d1, d18
	fcvtl	v18.2d, v19.2s
	fmul.2d	v16, v16, v18
	fadd	d1, d1, d18
	mov	d18, v18[1]
	fadd	d1, d1, d18
	fcvtl2	v2.2d, v2.4s
	fcvtl2	v18.2d, v19.4s
	fmul.2d	v2, v2, v18
	fadd	d1, d1, d18
	mov	d18, v18[1]
	fadd	d1, d1, d18
	fadd	d0, d0, d17
	mov	d17, v17[1]
	fadd	d0, d0, d17
	fadd	d0, d0, d7
	mov	d7, v7[1]
	fadd	d0, d0, d7
	fadd	d0, d0, d6
	mov	d6, v6[1]
	fadd	d0, d0, d6
	fadd	d0, d0, d5
	mov	d5, v5[1]
	fadd	d0, d0, d5
	fadd	d0, d0, d4
	mov	d4, v4[1]
	fadd	d0, d0, d4
	fadd	d0, d0, d3
	mov	d3, v3[1]
	fadd	d0, d0, d3
	fadd	d0, d0, d16
	mov	d3, v16[1]
	fadd	d0, d0, d3
	fadd	d0, d0, d2
	mov	d2, v2[1]
	fadd	d0, d0, d2
	subs	x0, x0, #16
	b.ne	LBB0_68
; %bb.69:                               ;   in Loop: Header=BB0_63 Depth=1
	cmp	x25, x9
	b.eq	LBB0_62
; %bb.70:                               ;   in Loop: Header=BB0_63 Depth=1
	mov	x1, x9
	mov	x16, x14
	mov	x17, x13
	mov	x0, x12
	tbnz	w25, #3, LBB0_65
LBB0_71:                                ; %.preheader37
                                        ;   Parent Loop BB0_63 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	s2, [x16], #4
	ldr	s3, [x17], #4
	fcvt	d2, s2
	fcvt	d3, s3
	fmul	d2, d2, d3
	fadd	d0, d0, d2
	fadd	d1, d1, d3
	subs	x0, x0, #1
	b.ne	LBB0_71
	b	LBB0_62
LBB0_72:                                ; %.preheader47.preheader
	str	x19, [sp, #56]                  ; 8-byte Folded Spill
	str	x26, [sp, #80]                  ; 8-byte Folded Spill
	mov	w19, #0                         ; =0x0
	movi	d8, #0000000000000000
LBB0_73:                                ; %.preheader47
                                        ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB0_74 Depth 2
	mov	x20, #0                         ; =0x0
	stp	xzr, xzr, [sp, #96]
	mov	x26, x21
LBB0_74:                                ;   Parent Loop BB0_73 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	add	x1, sp, #104
	add	x2, sp, #96
	mov	x0, x26
	bl	__ZL15variance_block8PKfRdS1_
	add	x20, x20, #8
	add	x26, x26, #32
	cmp	x20, x25
	b.lo	LBB0_74
; %bb.75:                               ;   in Loop: Header=BB0_73 Depth=1
	ldp	d1, d0, [sp, #96]
	fadd	d0, d0, d1
	fadd	d8, d8, d0
	add	w19, w19, #1
	cmp	w19, w28
	b.ne	LBB0_73
; %bb.76:
	mov	w8, #0                          ; =0x0
	and	x9, x25, #0x7ffffff0
	add	x10, x21, #32
	and	x11, x25, #0xf
	ldr	x13, [sp, #56]                  ; 8-byte Folded Reload
	ubfiz	x12, x13, #2, #32
	and	x12, x12, #0x3ffffffc0
	add	x12, x21, x12
	and	x13, x13, #0xfffffff8
	neg	x13, x13
	movi	d9, #0000000000000000
	b	LBB0_78
LBB0_77:                                ; %.loopexit42
                                        ;   in Loop: Header=BB0_78 Depth=1
	fadd	d0, d0, d1
	fadd	d9, d9, d0
	add	w8, w8, #1
	cmp	w8, w28
	b.eq	LBB0_103
LBB0_78:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB0_83 Depth 2
                                        ;     Child Loop BB0_86 Depth 2
                                        ;     Child Loop BB0_81 Depth 2
	cmp	w24, #16
	b.ge	LBB0_82
; %bb.79:                               ;   in Loop: Header=BB0_78 Depth=1
	mov	x16, #0                         ; =0x0
	movi	d1, #0000000000000000
	movi	d0, #0000000000000000
LBB0_80:                                ; %.preheader290.preheader
                                        ;   in Loop: Header=BB0_78 Depth=1
	add	x14, x13, x16
	add	x15, x21, x16, lsl #2
LBB0_81:                                ; %.preheader290
                                        ;   Parent Loop BB0_78 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	q2, [x15], #16
	fcvtl2	v3.2d, v2.4s
	mov	d4, v3[1]
	fcvtl	v2.2d, v2.2s
	mov	d5, v2[1]
	fmul.2d	v6, v3, v3
	mov	d7, v6[1]
	fmul.2d	v16, v2, v2
	mov	d17, v16[1]
	fadd	d1, d1, d16
	fadd	d1, d1, d17
	fadd	d1, d1, d6
	fadd	d1, d1, d7
	fadd	d0, d0, d2
	fadd	d0, d0, d5
	fadd	d0, d0, d3
	fadd	d0, d0, d4
	adds	x14, x14, #4
	b.ne	LBB0_81
	b	LBB0_77
LBB0_82:                                ; %.preheader45.preheader
                                        ;   in Loop: Header=BB0_78 Depth=1
	movi	d1, #0000000000000000
	mov	x14, x10
	mov	x15, x9
	movi	d0, #0000000000000000
LBB0_83:                                ; %.preheader45
                                        ;   Parent Loop BB0_78 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldp	q3, q4, [x14, #-32]
	ldp	q5, q17, [x14], #64
	fcvtl2	v2.2d, v3.4s
	fcvtl	v6.2d, v3.2s
	fcvtl2	v3.2d, v4.4s
	fcvtl	v7.2d, v4.2s
	fcvtl2	v4.2d, v5.4s
	fcvtl	v16.2d, v5.2s
	fcvtl2	v5.2d, v17.4s
	fcvtl	v17.2d, v17.2s
	fmul.2d	v18, v6, v6
	fadd	d1, d1, d18
	mov	d18, v18[1]
	fadd	d1, d1, d18
	fmul.2d	v18, v2, v2
	fadd	d1, d1, d18
	mov	d18, v18[1]
	fadd	d1, d1, d18
	fmul.2d	v18, v7, v7
	fadd	d1, d1, d18
	mov	d18, v18[1]
	fadd	d1, d1, d18
	fmul.2d	v18, v3, v3
	fadd	d1, d1, d18
	mov	d18, v18[1]
	fadd	d1, d1, d18
	fmul.2d	v18, v16, v16
	fadd	d1, d1, d18
	mov	d18, v18[1]
	fadd	d1, d1, d18
	fmul.2d	v18, v4, v4
	fadd	d1, d1, d18
	mov	d18, v18[1]
	fadd	d1, d1, d18
	fmul.2d	v18, v17, v17
	fadd	d1, d1, d18
	mov	d18, v18[1]
	fadd	d1, d1, d18
	fmul.2d	v18, v5, v5
	fadd	d1, d1, d18
	mov	d18, v18[1]
	fadd	d1, d1, d18
	fadd	d0, d0, d6
	mov	d6, v6[1]
	fadd	d0, d0, d6
	fadd	d0, d0, d2
	mov	d2, v2[1]
	fadd	d0, d0, d2
	fadd	d0, d0, d7
	mov	d2, v7[1]
	fadd	d0, d0, d2
	fadd	d0, d0, d3
	mov	d2, v3[1]
	fadd	d0, d0, d2
	fadd	d0, d0, d16
	mov	d2, v16[1]
	fadd	d0, d0, d2
	fadd	d0, d0, d4
	mov	d2, v4[1]
	fadd	d0, d0, d2
	fadd	d0, d0, d17
	mov	d2, v17[1]
	fadd	d0, d0, d2
	fadd	d0, d0, d5
	mov	d2, v5[1]
	fadd	d0, d0, d2
	subs	x15, x15, #16
	b.ne	LBB0_83
; %bb.84:                               ;   in Loop: Header=BB0_78 Depth=1
	cmp	x25, x9
	b.eq	LBB0_77
; %bb.85:                               ;   in Loop: Header=BB0_78 Depth=1
	mov	x16, x9
	mov	x14, x12
	mov	x15, x11
	tbnz	w25, #3, LBB0_80
LBB0_86:                                ; %.preheader43
                                        ;   Parent Loop BB0_78 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	s2, [x14], #4
	fcvt	d2, s2
	fadd	d0, d0, d2
	fmul	d2, d2, d2
	fadd	d1, d1, d2
	subs	x15, x15, #1
	b.ne	LBB0_86
	b	LBB0_77
LBB0_87:                                ; %.preheader53.preheader
	str	x19, [sp, #56]                  ; 8-byte Folded Spill
	stp	x27, x26, [sp, #72]             ; 16-byte Folded Spill
	mov	w19, #0                         ; =0x0
	movi	d8, #0000000000000000
LBB0_88:                                ; %.preheader53
                                        ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB0_89 Depth 2
	mov	x20, #0                         ; =0x0
	stp	xzr, xzr, [sp, #96]
	mov	x26, x21
	mov	x27, x22
	str	xzr, [sp, #88]
LBB0_89:                                ;   Parent Loop BB0_88 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	add	x2, sp, #104
	add	x3, sp, #96
	add	x4, sp, #88
	mov	x0, x26
	mov	x1, x27
	bl	__ZL17covariance_block8PKfS0_RdS1_S1_
	add	x20, x20, #8
	add	x27, x27, #32
	add	x26, x26, #32
	cmp	x20, x25
	b.lo	LBB0_89
; %bb.90:                               ;   in Loop: Header=BB0_88 Depth=1
	ldp	d1, d0, [sp, #96]
	fadd	d0, d0, d1
	ldr	d1, [sp, #88]
	fadd	d0, d0, d1
	fadd	d8, d8, d0
	add	w19, w19, #1
	cmp	w19, w28
	b.ne	LBB0_88
; %bb.91:
	mov	w8, #0                          ; =0x0
	and	x9, x25, #0x7ffffff0
	add	x10, x21, #32
	add	x11, x22, #32
	and	x12, x25, #0xf
	ldr	x15, [sp, #56]                  ; 8-byte Folded Reload
	ubfiz	x13, x15, #2, #32
	and	x14, x13, #0x3ffffffc0
	add	x13, x22, x14
	add	x14, x21, x14
	and	x15, x15, #0xfffffff8
	neg	x15, x15
	movi	d9, #0000000000000000
	b	LBB0_93
LBB0_92:                                ; %.loopexit48
                                        ;   in Loop: Header=BB0_93 Depth=1
	fadd	d0, d2, d0
	fadd	d0, d0, d1
	fadd	d9, d9, d0
	add	w8, w8, #1
	cmp	w8, w28
	b.eq	LBB0_102
LBB0_93:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB0_98 Depth 2
                                        ;     Child Loop BB0_101 Depth 2
                                        ;     Child Loop BB0_96 Depth 2
	cmp	w24, #16
	b.ge	LBB0_97
; %bb.94:                               ;   in Loop: Header=BB0_93 Depth=1
	mov	x1, #0                          ; =0x0
	movi	d1, #0000000000000000
	movi	d0, #0000000000000000
	movi	d2, #0000000000000000
LBB0_95:                                ; %.preheader304.preheader
                                        ;   in Loop: Header=BB0_93 Depth=1
	add	x16, x15, x1
	lsl	x0, x1, #2
	add	x17, x22, x0
	add	x0, x21, x0
LBB0_96:                                ; %.preheader304
                                        ;   Parent Loop BB0_93 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	q3, [x0], #16
	fcvtl2	v4.2d, v3.4s
	mov	d5, v4[1]
	fcvtl	v3.2d, v3.2s
	mov	d6, v3[1]
	ldr	q7, [x17], #16
	fcvtl2	v16.2d, v7.4s
	mov	d17, v16[1]
	fcvtl	v7.2d, v7.2s
	mov	d18, v7[1]
	fmul.2d	v19, v4, v16
	mov	d20, v19[1]
	fmul.2d	v21, v3, v7
	mov	d22, v21[1]
	fadd	d1, d1, d21
	fadd	d1, d1, d22
	fadd	d1, d1, d19
	fadd	d1, d1, d20
	fadd	d0, d0, d7
	fadd	d0, d0, d18
	fadd	d0, d0, d16
	fadd	d0, d0, d17
	fadd	d2, d2, d3
	fadd	d2, d2, d6
	fadd	d2, d2, d4
	fadd	d2, d2, d5
	adds	x16, x16, #4
	b.ne	LBB0_96
	b	LBB0_92
LBB0_97:                                ; %.preheader51.preheader
                                        ;   in Loop: Header=BB0_93 Depth=1
	movi	d1, #0000000000000000
	mov	x16, x11
	mov	x17, x10
	mov	x0, x9
	movi	d0, #0000000000000000
	movi	d2, #0000000000000000
LBB0_98:                                ; %.preheader51
                                        ;   Parent Loop BB0_93 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldp	q4, q5, [x17, #-32]
	ldp	q6, q18, [x17], #64
	fcvtl2	v3.2d, v4.4s
	fcvtl	v7.2d, v4.2s
	fcvtl2	v4.2d, v5.4s
	fcvtl	v16.2d, v5.2s
	fcvtl2	v5.2d, v6.4s
	fcvtl	v17.2d, v6.2s
	fcvtl2	v6.2d, v18.4s
	fcvtl	v18.2d, v18.2s
	ldp	q20, q21, [x16, #-32]
	ldp	q22, q26, [x16], #64
	fcvtl2	v19.2d, v20.4s
	fcvtl	v23.2d, v20.2s
	fcvtl2	v20.2d, v21.4s
	fcvtl	v24.2d, v21.2s
	fcvtl2	v21.2d, v22.4s
	fcvtl	v25.2d, v22.2s
	fcvtl2	v22.2d, v26.4s
	fcvtl	v26.2d, v26.2s
	fmul.2d	v27, v7, v23
	fadd	d1, d1, d27
	mov	d27, v27[1]
	fadd	d1, d1, d27
	fmul.2d	v27, v3, v19
	fadd	d1, d1, d27
	mov	d27, v27[1]
	fadd	d1, d1, d27
	fmul.2d	v27, v16, v24
	fadd	d1, d1, d27
	mov	d27, v27[1]
	fadd	d1, d1, d27
	fmul.2d	v27, v4, v20
	fadd	d1, d1, d27
	mov	d27, v27[1]
	fadd	d1, d1, d27
	fmul.2d	v27, v17, v25
	fadd	d1, d1, d27
	mov	d27, v27[1]
	fadd	d1, d1, d27
	fmul.2d	v27, v5, v21
	fadd	d1, d1, d27
	mov	d27, v27[1]
	fadd	d1, d1, d27
	fmul.2d	v27, v18, v26
	fadd	d1, d1, d27
	mov	d27, v27[1]
	fadd	d1, d1, d27
	fmul.2d	v27, v6, v22
	fadd	d1, d1, d27
	mov	d27, v27[1]
	fadd	d1, d1, d27
	fadd	d0, d0, d23
	mov	d23, v23[1]
	fadd	d0, d0, d23
	fadd	d0, d0, d19
	mov	d19, v19[1]
	fadd	d0, d0, d19
	fadd	d0, d0, d24
	mov	d19, v24[1]
	fadd	d0, d0, d19
	fadd	d0, d0, d20
	mov	d19, v20[1]
	fadd	d0, d0, d19
	fadd	d0, d0, d25
	mov	d19, v25[1]
	fadd	d0, d0, d19
	fadd	d0, d0, d21
	mov	d19, v21[1]
	fadd	d0, d0, d19
	fadd	d0, d0, d26
	mov	d19, v26[1]
	fadd	d0, d0, d19
	fadd	d0, d0, d22
	mov	d19, v22[1]
	fadd	d0, d0, d19
	fadd	d2, d2, d7
	mov	d7, v7[1]
	fadd	d2, d2, d7
	fadd	d2, d2, d3
	mov	d3, v3[1]
	fadd	d2, d2, d3
	fadd	d2, d2, d16
	mov	d3, v16[1]
	fadd	d2, d2, d3
	fadd	d2, d2, d4
	mov	d3, v4[1]
	fadd	d2, d2, d3
	fadd	d2, d2, d17
	mov	d3, v17[1]
	fadd	d2, d2, d3
	fadd	d2, d2, d5
	mov	d3, v5[1]
	fadd	d2, d2, d3
	fadd	d2, d2, d18
	mov	d3, v18[1]
	fadd	d2, d2, d3
	fadd	d2, d2, d6
	mov	d3, v6[1]
	fadd	d2, d2, d3
	subs	x0, x0, #16
	b.ne	LBB0_98
; %bb.99:                               ;   in Loop: Header=BB0_93 Depth=1
	cmp	x25, x9
	b.eq	LBB0_92
; %bb.100:                              ;   in Loop: Header=BB0_93 Depth=1
	mov	x1, x9
	mov	x16, x14
	mov	x17, x13
	mov	x0, x12
	tbnz	w25, #3, LBB0_95
LBB0_101:                               ; %.preheader49
                                        ;   Parent Loop BB0_93 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	s3, [x16], #4
	fcvt	d3, s3
	ldr	s4, [x17], #4
	fcvt	d4, s4
	fadd	d2, d2, d3
	fadd	d0, d0, d4
	fmul	d3, d3, d4
	fadd	d1, d1, d3
	subs	x0, x0, #1
	b.ne	LBB0_101
	b	LBB0_92
LBB0_102:
	ldr	x27, [sp, #72]                  ; 8-byte Folded Reload
LBB0_103:                               ; %.loopexit25
	ldr	x26, [sp, #80]                  ; 8-byte Folded Reload
LBB0_104:                               ; %.loopexit25
	bl	__ZNSt3__16chrono12steady_clock3nowEv
	fabd	d0, d8, d9
	fabs	d1, d9
	mov	x8, #60813                      ; =0xed8d
	movk	x8, #41141, lsl #16
	movk	x8, #50935, lsl #32
	movk	x8, #16064, lsl #48
	fmov	d2, x8
	fmul	d1, d1, d2
	mov	x8, #5243                       ; =0x147b
	movk	x8, #18350, lsl #16
	movk	x8, #31457, lsl #32
	movk	x8, #16260, lsl #48
	fmov	d2, x8
	fadd	d1, d1, d2
	fcmp	d0, d1
	b.le	LBB0_106
; %bb.105:
Lloh21:
	adrp	x8, ___stderrp@GOTPAGE
Lloh22:
	ldr	x8, [x8, ___stderrp@GOTPAGEOFF]
Lloh23:
	ldr	x0, [x8]
	stp	d0, d1, [sp, #24]
	stp	d8, d9, [sp, #8]
	str	x26, [sp]
Lloh24:
	adrp	x1, l_.str.7@PAGE
Lloh25:
	add	x1, x1, l_.str.7@PAGEOFF
	bl	_fprintf
	mov	w20, #2                         ; =0x2
	b	LBB0_107
LBB0_106:
	ldr	x8, [sp, #64]                   ; 8-byte Folded Reload
	sub	x8, x0, x8
	mov	x9, #63439                      ; =0xf7cf
	movk	x9, #58195, lsl #16
	movk	x9, #39845, lsl #32
	movk	x9, #8388, lsl #48
	smulh	x8, x8, x9
	asr	x9, x8, #7
	add	x8, x9, x8, lsr #63
	stp	d9, d0, [sp, #40]
	str	d8, [sp, #32]
	stp	x26, x28, [sp]
Lloh26:
	adrp	x0, l_.str.8@PAGE
Lloh27:
	add	x0, x0, l_.str.8@PAGEOFF
	stp	x25, x8, [sp, #16]
	bl	_printf
	mov	w20, #0                         ; =0x0
LBB0_107:
	mov	x0, x23
	mov	x1, x27
	bl	__ZdlPvm
	mov	x0, x22
	mov	x1, x27
	bl	__ZdlPvm
	mov	x0, x21
	mov	x1, x27
	bl	__ZdlPvm
	mov	x0, x20
	ldp	x29, x30, [sp, #208]            ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #192]            ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #176]            ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #160]            ; 16-byte Folded Reload
	ldp	x26, x25, [sp, #144]            ; 16-byte Folded Reload
	ldp	x28, x27, [sp, #128]            ; 16-byte Folded Reload
	ldp	d9, d8, [sp, #112]              ; 16-byte Folded Reload
	add	sp, sp, #224
	ret
LBB0_108:
Ltmp5:
	mov	x20, x0
	mov	x0, x22
	mov	x19, x27
	mov	x1, x27
	bl	__ZdlPvm
	mov	x0, x21
	mov	x1, x19
	bl	__ZdlPvm
	mov	x0, x20
	bl	__Unwind_Resume
LBB0_109:
Ltmp2:
	mov	x19, x27
	mov	x20, x0
	mov	x0, x21
	mov	x1, x19
	bl	__ZdlPvm
	mov	x0, x20
	bl	__Unwind_Resume
	.loh AdrpAdd	Lloh0, Lloh1
	.loh AdrpLdr	Lloh2, Lloh3
	.loh AdrpAdd	Lloh4, Lloh5
	.loh AdrpAdd	Lloh6, Lloh7
	.loh AdrpAdd	Lloh8, Lloh9
	.loh AdrpAdd	Lloh10, Lloh11
	.loh AdrpAdd	Lloh12, Lloh13
	.loh AdrpAdd	Lloh14, Lloh15
	.loh AdrpAdd	Lloh19, Lloh20
	.loh AdrpLdrGotLdr	Lloh16, Lloh17, Lloh18
	.loh AdrpAdd	Lloh24, Lloh25
	.loh AdrpLdrGotLdr	Lloh21, Lloh22, Lloh23
	.loh AdrpAdd	Lloh26, Lloh27
Lfunc_end0:
	.cfi_endproc
	.section	__TEXT,__gcc_except_tab
	.p2align	2, 0x0
GCC_except_table0:
Lexception0:
	.byte	255                             ; @LPStart Encoding = omit
	.byte	255                             ; @TType Encoding = omit
	.byte	1                               ; Call site Encoding = uleb128
	.uleb128 Lcst_end0-Lcst_begin0
Lcst_begin0:
	.uleb128 Lfunc_begin0-Lfunc_begin0      ; >> Call Site 1 <<
	.uleb128 Ltmp0-Lfunc_begin0             ;   Call between Lfunc_begin0 and Ltmp0
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp0-Lfunc_begin0             ; >> Call Site 2 <<
	.uleb128 Ltmp1-Ltmp0                    ;   Call between Ltmp0 and Ltmp1
	.uleb128 Ltmp2-Lfunc_begin0             ;     jumps to Ltmp2
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp1-Lfunc_begin0             ; >> Call Site 3 <<
	.uleb128 Ltmp3-Ltmp1                    ;   Call between Ltmp1 and Ltmp3
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp3-Lfunc_begin0             ; >> Call Site 4 <<
	.uleb128 Ltmp4-Ltmp3                    ;   Call between Ltmp3 and Ltmp4
	.uleb128 Ltmp5-Lfunc_begin0             ;     jumps to Ltmp5
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp4-Lfunc_begin0             ; >> Call Site 5 <<
	.uleb128 Lfunc_end0-Ltmp4               ;   Call between Ltmp4 and Lfunc_end0
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
Lcst_end0:
	.p2align	2, 0x0
                                        ; -- End function
	.section	__TEXT,__text,regular,pure_instructions
	.p2align	2                               ; -- Begin function _ZL16array_sum_block8PKf
__ZL16array_sum_block8PKf:              ; @_ZL16array_sum_block8PKf
	.cfi_startproc
; %bb.0:
	movi	d0, #0000000000000000
	ldp	s1, s2, [x0]
	fadd	s0, s1, s0
	fadd	s0, s0, s2
	ldp	s1, s2, [x0, #8]
	fadd	s0, s0, s1
	fadd	s0, s0, s2
	ldp	s1, s2, [x0, #16]
	fadd	s0, s0, s1
	fadd	s0, s0, s2
	ldp	s1, s2, [x0, #24]
	fadd	s0, s0, s1
	fadd	s0, s0, s2
	ret
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function _ZL18dot_product_block8PKfS0_
__ZL18dot_product_block8PKfS0_:         ; @_ZL18dot_product_block8PKfS0_
	.cfi_startproc
; %bb.0:
	ldp	d0, d1, [x0]
	ldp	d2, d3, [x1]
	fmul.2s	v0, v0, v2
	movi	d2, #0000000000000000
	fadd	s2, s0, s2
	mov	s0, v0[1]
	fadd	s0, s2, s0
	fmul.2s	v1, v1, v3
	fadd	s0, s0, s1
	mov	s1, v1[1]
	fadd	s0, s1, s0
	ldp	d1, d2, [x0, #16]
	ldp	d3, d4, [x1, #16]
	fmul.2s	v1, v1, v3
	fadd	s0, s0, s1
	mov	s1, v1[1]
	fadd	s0, s1, s0
	fmul.2s	v1, v2, v4
	fadd	s0, s0, s1
	mov	s1, v1[1]
	fadd	s0, s1, s0
	ret
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function _ZL18rolling_sum_block8PKffRfS1_
__ZL18rolling_sum_block8PKffRfS1_:      ; @_ZL18rolling_sum_block8PKffRfS1_
	.cfi_startproc
; %bb.0:
	ldp	s1, s2, [x0]
	fadd	s0, s0, s1
	fadd	s1, s0, s2
	ldp	s2, s3, [x0, #8]
	fadd	s2, s1, s2
	fadd	s3, s2, s3
	ldp	s4, s5, [x0, #16]
	fadd	s4, s3, s4
	fadd	s5, s4, s5
	ldp	s6, s7, [x0, #24]
	fadd	s6, s5, s6
	fadd	s7, s6, s7
	str	s7, [x1]
	fadd	s0, s0, s1
	fadd	s0, s0, s2
	fadd	s0, s0, s3
	fadd	s0, s0, s4
	fadd	s0, s0, s5
	fadd	s0, s0, s6
	fadd	s0, s0, s7
	ldr	s1, [x2]
	fadd	s0, s1, s0
	str	s0, [x2]
	ret
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function _ZL11vwap_block8PKfS0_RdS1_
__ZL11vwap_block8PKfS0_RdS1_:           ; @_ZL11vwap_block8PKfS0_RdS1_
	.cfi_startproc
; %bb.0:
	ldp	s0, s1, [x0]
	ldp	s2, s3, [x1]
	fmul	s0, s0, s2
	movi	d4, #0000000000000000
	fadd	s0, s0, s4
	fadd	s2, s2, s4
	fmul	s1, s1, s3
	fadd	s0, s0, s1
	fadd	s1, s2, s3
	ldp	s2, s3, [x0, #8]
	ldp	s4, s5, [x1, #8]
	fmul	s2, s2, s4
	fadd	s0, s0, s2
	fadd	s1, s1, s4
	fmul	s2, s3, s5
	fadd	s0, s0, s2
	fadd	s1, s1, s5
	ldp	s2, s3, [x0, #16]
	ldp	s4, s5, [x1, #16]
	fmul	s2, s2, s4
	fadd	s0, s0, s2
	fadd	s1, s1, s4
	fmul	s2, s3, s5
	fadd	s0, s0, s2
	fadd	s1, s1, s5
	ldp	s2, s3, [x0, #24]
	ldp	s4, s5, [x1, #24]
	fmul	s2, s2, s4
	fadd	s0, s0, s2
	fadd	s1, s1, s4
	fmul	s2, s3, s5
	fadd	s0, s0, s2
	fadd	s1, s1, s5
	fcvt	d0, s0
	ldr	d2, [x2]
	fadd	d0, d2, d0
	str	d0, [x2]
	fcvt	d0, s1
	ldr	d1, [x3]
	fadd	d0, d1, d0
	str	d0, [x3]
	ret
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function _ZL15variance_block8PKfRdS1_
__ZL15variance_block8PKfRdS1_:          ; @_ZL15variance_block8PKfRdS1_
	.cfi_startproc
; %bb.0:
	movi	d0, #0000000000000000
	ldp	s1, s2, [x0]
	fadd	s0, s1, s0
	fmul	s1, s1, s1
	fadd	s0, s0, s2
	fmul	s2, s2, s2
	fadd	s1, s1, s2
	ldp	s2, s3, [x0, #8]
	fadd	s0, s0, s2
	fmul	s2, s2, s2
	fadd	s1, s1, s2
	fadd	s0, s0, s3
	fmul	s2, s3, s3
	fadd	s1, s1, s2
	ldp	s2, s3, [x0, #16]
	fadd	s0, s0, s2
	fmul	s2, s2, s2
	fadd	s1, s1, s2
	fadd	s0, s0, s3
	fmul	s2, s3, s3
	fadd	s1, s1, s2
	ldp	s2, s3, [x0, #24]
	fadd	s0, s0, s2
	fmul	s2, s2, s2
	fadd	s1, s1, s2
	fadd	s0, s0, s3
	fmul	s2, s3, s3
	fadd	s1, s1, s2
	fcvt	d0, s0
	ldr	d2, [x1]
	fadd	d0, d2, d0
	str	d0, [x1]
	fcvt	d0, s1
	ldr	d1, [x2]
	fadd	d0, d1, d0
	str	d0, [x2]
	ret
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function _ZL17covariance_block8PKfS0_RdS1_S1_
__ZL17covariance_block8PKfS0_RdS1_S1_:  ; @_ZL17covariance_block8PKfS0_RdS1_S1_
	.cfi_startproc
; %bb.0:
	movi	d0, #0000000000000000
	ldp	s1, s2, [x0]
	fadd	s3, s1, s0
	ldp	s4, s5, [x1]
	fadd	s6, s4, s0
	fmul	s1, s1, s4
	fadd	s0, s1, s0
	fadd	s1, s3, s2
	fadd	s3, s6, s5
	fmul	s2, s2, s5
	fadd	s0, s0, s2
	ldp	s2, s4, [x0, #8]
	fadd	s1, s1, s2
	ldp	s5, s6, [x1, #8]
	fadd	s3, s3, s5
	fmul	s2, s2, s5
	fadd	s0, s0, s2
	fadd	s1, s1, s4
	fadd	s2, s3, s6
	fmul	s3, s4, s6
	fadd	s0, s0, s3
	ldp	s3, s4, [x0, #16]
	fadd	s1, s1, s3
	ldp	s5, s6, [x1, #16]
	fadd	s2, s2, s5
	fmul	s3, s3, s5
	fadd	s0, s0, s3
	fadd	s1, s1, s4
	fadd	s2, s2, s6
	fmul	s3, s4, s6
	fadd	s0, s0, s3
	ldp	s3, s4, [x0, #24]
	fadd	s1, s1, s3
	ldp	s5, s6, [x1, #24]
	fadd	s2, s2, s5
	fmul	s3, s3, s5
	fadd	s0, s0, s3
	fadd	s1, s1, s4
	fadd	s2, s2, s6
	fmul	s3, s4, s6
	fadd	s0, s0, s3
	fcvt	d1, s1
	ldr	d3, [x2]
	fadd	d1, d3, d1
	str	d1, [x2]
	fcvt	d1, s2
	ldr	d2, [x3]
	fadd	d1, d2, d1
	str	d1, [x3]
	fcvt	d0, s0
	ldr	d1, [x4]
	fadd	d0, d1, d0
	str	d0, [x4]
	ret
	.cfi_endproc
                                        ; -- End function
	.section	__TEXT,__cstring,cstring_literals
l_.str:                                 ; @.str
	.asciz	"array_sum"

l_.str.1:                               ; @.str.1
	.asciz	"dot_product"

l_.str.2:                               ; @.str.2
	.asciz	"rolling_sum"

l_.str.3:                               ; @.str.3
	.asciz	"vwap"

l_.str.4:                               ; @.str.4
	.asciz	"variance"

l_.str.5:                               ; @.str.5
	.asciz	"covariance"

l_.str.6:                               ; @.str.6
	.asciz	"Unknown kernel '%s'. Use array_sum|dot_product|rolling_sum|vwap|variance|covariance\n"

l_.str.7:                               ; @.str.7
	.asciz	"validation_failed kernel=%s got=%0.9f ref=%0.9f abs_err=%0.9f tol=%0.9f\n"

l_.str.8:                               ; @.str.8
	.asciz	"kernel=%s iters=%d n=%d time_us=%lld checksum=%.9f ref=%.9f abs_err=%.9f\n"

.subsections_via_symbols
