	/*
	 * Print routines for 16-bit Real Mode operation
	 *
	 * These are based on the video services that PC BIOSes provide with
	 * INT 0x10. For more information on INT 0x10 services, see:
	 *
	 * - Stanislav's interrupt list
	 * <https://stanislavs.org/helppc/int_10.html>
	 *
	 * - Ralf Brown's Interrupt List
	 * <https://www.cs.cmu.edu/~ralf/files.html>
	 */

	.equ	BIOS_INT_VIDEO,		0x10
	.equ	BIOS_INT_TELETYPE_AH,	0x0e
	.equ	BIOS_INT_TELETYPE_BH,	0x00
	.equ	BIOS_INT_SET_CURSOR_AH,	0x01
	.equ	BIOS_INT_SET_CURSOR_DISABLE_CX,	0x2000

.text
.code16

	/*
	 * Print a single ASCII character
	 *
	 * In:	AL	Character to print
	 */
_print16_char:
	push	%bx

	mov	$BIOS_INT_TELETYPE_AH,	%ah
	mov	$BIOS_INT_TELETYPE_BH,	%bh
	int	$BIOS_INT_VIDEO

	pop	%bx
	ret

.macro	print_char	char
	.ifnc	\char,	%al
	mov	\char,	%al
	.endif
	call	_print16_char
.endm

	/*
	 * Print a zero-terminated ASCII string
	 *
	 * In:	DS:SI	Pointer to string
	 */
_print16_str:
1:	/* Loop: Print characters. */
	lodsb			# Load byte from SI to AL, increment SI.
	cmp	$0,	%al	# If byte is zero,
	je	2f		#	break.
	call	_print16_char	# Print character.
	jmp	1b		# Continue.
2:
	ret

.macro	print_str	str
	mov	\str,	%si
	call	_print16_str
.endm


	/*
	 * Routines to print hex digits
	 *
	 * If code space is precious, like in a bootblock, these routines can
	 * be skipped by defining the symbol PRINT16_SKIP_PRINT_HEX.
	 */
.ifndef PRINT16_SKIP_PRINT_HEX

	/*
	 * Print a nibble as a hex digit
	 *
	 * In:	AL(low)	Nibble to print
	 */
_print16_nibble:
	and	$0x0f,	%al
	cmp	$0x0a,	%al
	jge	1f

	add	$('0 - 0x0),	%al
	jmp	2f
1:	add	$('a - 0xa),	%al
2:
	call	_print16_char
	ret

	/*
	 * Print a byte as two hex digits
	 *
	 * In:	AL	Byte to print
	 */
_print16_byte:
	push	%ax
	shr	$4,	%al
	call	_print16_nibble
	pop	%ax
	call	_print16_nibble
	ret

.macro	print_byte	byte
	.ifnc	\byte,	%al
	mov	\byte,	%al
	.endif
	call	_print16_byte
.endm

	/*
	 * Print a 16-bit word as four hex digits
	 *
	 * In:	AX	Word to print
	 */
_print16_word:
	push	%ax
	mov	%ah,	%al
	call	_print16_byte
	pop	%ax
	call	_print16_byte
	ret

.macro	print_word	word
	.ifnc	\word,	%ax
	mov	\word,	%ax
	.endif
	call	_print16_word
.endm

.endif	# !PRINT16_SKIP_PRINT_HEX

	/*
	 * Turn off the teletype cursor
	 *
	 * The cursor is helpful to show the bootblock's loading progress.
	 * However, a flashing cursor at a random place in the screen will be
	 * distracting when we start the actual kernel.
	 */
print16_cursor_disable:
	mov	$BIOS_INT_SET_CURSOR_AH,		%ah
	mov	$BIOS_INT_SET_CURSOR_DISABLE_CX,	%cx
	int	$BIOS_INT_VIDEO
	ret

