/* === Options to switch code on and off === */

	.equ	RELOCATE_SELF,		1
	.equ	PASS_KERNEL_PARAMS,	0

	.equ	DEBUG_GEOMETRY,		1
	.equ	DEBUG_LOAD_DST,		1
	.equ	DEBUG_LOAD_VALUE,	1
	.equ	DEBUG_NO_LAUNCH_KERNEL,	0


/* === Important Constants === */

#include <syslib/addrs.h>

#define KERNEL_START_PADDR	KERNEL_PADDR

	.equ	SECTOR_SIZE_BYTES,	512
	.equ	KERNEL_SECTOR,		2


/* === Beginning of Bootblock Code === */

	.text
	.code16

	.globl	_start
_start:				# Start at zero.
	jmp	main		# Jump to the real start point.

/* === Data area === */

	/* createimage will write the number of sectors to load in this spot. */
os_size_sectors:	.word	0

	/* Space to hold drive geometry. */
cylinder_max:		.word	0
head_max:		.byte	0
sector_max:		.byte	0
drive_num:		.byte	0


/* === Print routines === */

#include "print16.S"


/* === Bootblock main program === */

main:
	/* Make sure that interrupts are off.
	 * The kernel will reenable them when it's ready. */
	cli

	/*
	 * Reset segment registers.
	 *
	 * - BIOSes typically start the bootblock at 0x0000:7c000.
	 * - But our bootblock is compiled to run at 0x07c0:00000.
	 *
	 * Having offsets relative to zero gives us the possibilty to relocate
	 * the bootblock if we need to.
	 */

	/* Set CS by doing a long jump to the next instruction. */
	ljmp	$(BOOTBLOCK_ORIG_PADDR >> 4),	$(1f)	# Set CS via long jump.
1:
	mov	%cs,	%ax				# Set DS to match.
	mov	%ax,	%ds

	/* Set up a stack for ourselves. */
	mov	$(STACK_PADDR>>4),		%ax
	mov	%ax,				%ss
	mov	$0,				%sp

	/* The BIOS has passed us the drive number in DL. Save it. */
	mov	%dl,		drive_num

	/* Hello world! */
	print_str	$str_hello

	.if RELOCATE_SELF
	/* Move the bootblock to another location, in case loading the kernel
	 * will clobber the memory where we are now. */
	print_str	$str_relo
	call		relocate_self
	print_str	$str_ok_nl
	.endif

	/* Get disk geometry. */
	print_str	$str_geom
	call		disk_get_geometry
	print_str	$str_ok_nl

	/* Load the kernel. */
	print_str	$str_load
	call		disk_load_kernel
	print_str	$str_ok_nl

	/* Turn off teletype cursor before going further. */
	call	print16_cursor_disable

	.if	PASS_KERNEL_PARAMS
	/* Save OS size in a register before we change the data segments. */
	movzxw	os_size_sectors,	%ecx	# Zero-extend word into long.
	.endif

	/* Set essential segment registers for the kernel.
	 * The kernel's initialization will handle the rest. */
	mov	$0,	%ax
	mov	%ax,	%ds
	mov	%ax,	%es

	/* Launch the kernel. */
	.if	DEBUG_NO_LAUNCH_KERNEL
	jmp	halt
	.endif
	.if	PASS_KERNEL_PARAMS
	pushl	%ecx			# OS size as parameter to kernel
	pushl	$0			# Dummy return address
	ljmp	$0,	$KERNEL_START_PADDR
	.else
	ljmp	$0,	$KERNEL_START_PADDR
	.endif


	/*
	 * Print error code and halt
	 *
	 * In:	AL	Error code
	 *
	 * Does not return.
	 */
error:
	push	%ax			# Save error code.
	print_str	$str_err
	pop	%ax			# Restore error code.
	print_byte	%al		# Print error code.
	/* No return. Fall through to halt. */

halt:
	hlt				# Pause execution
	jmp	halt			# Loop forever

	/* Strings for the main program.
	 * Bootblock space is precious, so we have to keep them short. */
str_hello:	.asciz	"Bootblock\r\n"
.if RELOCATE_SELF
str_relo:	.asciz	"Relo"
.endif
str_geom:	.asciz	"Geom"
str_load:	.asciz	"Load"

str_ok_nl:	.ascii	" OK"	# No terminator. String continues into nl.
str_nl:		.asciz	"\r\n"
str_err:	.asciz	" ERR "


/* === Bootblock self-relocation === */

	/*
	 * Copy bootblock to a another location in memory
	 *
	 * Out:
	 *	DS, ES	Set for data access at new bootblock location
	 */
.if RELOCATE_SELF
relocate_self:
	/* Set copy source: DS:SI. */
	mov	$0x0,	%si

	/* Set copy destination: ES:DI. */
	mov	$(BOOTBLOCK_RELO_PADDR>>4),	%ax
	mov	%ax,				%es
	mov	$0x0,				%di

	/* Copy bytes. */
	mov	$SECTOR_SIZE_BYTES,	%cx	# Set number of bytes to copy.
	cld					# Set direction to count down.
	rep	movsb				# Copy bytes.

	/* Set segment registers for the new location. */
	ljmp	$(BOOTBLOCK_RELO_PADDR>>4),	$(1f)	# Set CS via long jump.
1:
	mov	%es,	%ax				# Set DS to match.
	mov	%ax,	%ds

	ret
.endif


/* === Disk read routines === */

	.equ	BIOS_INT_DISK,		0x13
	.equ	BIOS_DISK_READ_AH,	0x02
	.equ	BIOS_DISK_GEOM_AH,	0x08

	/*
	 * BIOS's packed CHS format
	 *
	 * The BIOS disk INT calls work with CHS (Cylinder/Head/Sector)
	 * addressing in a particular format:
	 *
	 *	Component	Bits	Register
	 *	---------	----	---------
	 *	Disk		 8	DL
	 *	Cylinder	10	CH, with overflow bits packed into CL
	 *	Head		 8	DH
	 *	Sector		 6	CL, packed with 2 cylinder bits
	 *
	 * The get-geometry call returns maximums in this format,
	 * and the read sector call takes its address in this format.
	 *
	 * Both give results in CF + AH:
	 *
	 *	CF	result: 0 for success, 1 for error
	 *	AH	status/error code
	 */
	.equ	SECTOR_MASK,	0b00111111
	.equ	CYL_HIGH_MASK,	0b11000000
	.equ	CYL_HIGH_SHF,	6


	/*
	 * Unpack BIOS cylinder+sector from packed CX to CX + AL
	 *
	 * In:
	 *	CX	Packed sector+cylinder bits
	 * Out:
	 *	CX	Cylinder number as normal word
	 *	AL	Sector number as normal byte
	 */
disk_cx_unpack:
	mov	%cl,		%al
	and	$SECTOR_MASK,	%al
	and	$CYL_HIGH_MASK,	%cl
	shr	$CYL_HIGH_SHF,	%cl
	xchg	%cl,		%ch
	ret


	/*
	 * Repack separate CX + AL cylinder+sector to BIOS packed CX
	 *
	 * In:
	 *	CX	Cylinder number as normal word
	 *	AL	Sector number as normal byte
	 * Out:
	 *	CX	Packed sector+cylinder bits
	 */
disk_cx_pack:
	xchg	%cl,		%ch
	shl	$CYL_HIGH_SHF,	%cl
	and	$SECTOR_MASK,	%al
	or	%al,		%cl
	ret


	/*
	 * Debug: print a newline without touching any registers
	 *
	 * Having this as a function saves bytes if it's called at least twice.
	 *
	 *	pusha/popa	2 bytes
	 *	print_str	6 bytes		# 8 bytes inline
	 *	ret		1 byte		# 9 bytes as a fun
	 *	call		3 bytes		# 3 bytes each call
	 *
	 *		inline		as fn
	 *	1 use:	 8 bytes	vs	12 bytes
	 *	2 uses:	16 bytes	vs	15 bytes
	 *	3 uses:	24 bytes	vs	18 bytes
	 */
.if DEBUG_GEOMETRY || DEBUG_LOAD_DST || DEBUG_LOAD_VALUE
debug_nl:
	pusha
	print_str	$str_nl
	popa
	ret
.endif


	/*
	 * Debug: print geometry on a new line
	 */
.if DEBUG_GEOMETRY
debug_geometry_unpacked_nl:
	.equ	PUSHA_AX_OFFSET,	14
	pusha
	call		debug_nl
	print_byte	%dl	# Drive
	print_char	$':
	print_word	%cx	# Cylinder
	print_char	$'/
	print_byte	%dh	# Head
	print_char	$'/
	mov	PUSHA_AX_OFFSET(%esp),	%ax	# Restore AX (sector)
	print_byte	%al	# Sector
	popa
	ret
.endif


	/*
	 * Call BIOS to get drive geometry and store result in data area
	 */
disk_get_geometry:
	mov	drive_num,		%dl
	mov	$BIOS_DISK_GEOM_AH,	%ah
	int	$BIOS_INT_DISK
	jc	error

	call	disk_cx_unpack
	mov	%cx,	cylinder_max
	mov	%dh,	head_max
	mov	%al,	sector_max

	.if DEBUG_GEOMETRY
	call		debug_geometry_unpacked_nl
	.endif

	ret


	/*
	 * Increment CHS address to next sector, according to geometry
	 *
	 * Takes a CHS address in unpacked format (see disk_cx_unpack) and
	 * increments it, rolling over from one field to the next according to
	 * the maximums saved in sector_max and head_max by disk_get_geometry.
	 */
disk_chs_inc_unpacked:
	inc	%al			# Increment sector.
	cmp	sector_max,	%al	# If no rollover,
	jbe	1f			#	break.

	/* Carry sectors into heads. */
	mov	$1,		%al	# Reset sector (sectors are 1-indexed).
	inc	%dh			# Increment head.
	cmp	head_max,	%dh	# If no rollover,
	jbe	1f			#	break.

	/* Carry heads into cylinders. */
	mov	$0,		%dh	# Reset head.
	inc	%cx			# Increment cylinder.
1:
	ret


	/*
	 * Load kernel from disk
	 */
disk_load_kernel:
	/* Begin with unpacked CHS address for sector 1.
	 * Remember: sectors are 1-indexed. */
	mov	drive_num,	%dl		# Drive
	mov	$0,		%cx		# C
	mov	$0,		%dh		# H
	mov	$1,		%al		# S

	mov	$KERNEL_SECTOR,	%si		# Count down from target,

1:	/* Loop: Fast-forward CHS address to target sector. */
	.if DEBUG_GEOMETRY
	call	debug_geometry_unpacked_nl
	.endif

	cmp	$1,		%si		#	until 1,
	je	2f				#	then break.
	call	disk_chs_inc_unpacked		# Incremement CHS address.
	dec	%si				# Decrement target.
	jmp	1b				# Continue.
2:
	/* Set destination buffer: ES:BX */
	mov	$(KERNEL_PADDR >> 4),	%bx	# Kernel sector
	mov	%bx,			%es	#	into ES.
	mov	$0,			%bx	# Offset 0.

	/* Loop: Read sectors. */
	mov	os_size_sectors,	%si	# Count down sectors to read,
3:
	cmp	$0,			%si	#	until 0,
	je	4f				#	then break.

	/* Debug: Print CHS address to read and/or ES:BX destination. */
	.if DEBUG_GEOMETRY
	call		debug_geometry_unpacked_nl
	.elseif DEBUG_LOAD_DST || DEBUG_LOAD_VALUE
	call		debug_nl
	.endif
	.if DEBUG_LOAD_DST
	pusha
	print_char	$'>
	print_word	%es
	print_char	$':
	print_word	%bx
	popa
	.endif

	/* Make BIOS read call. */
	call	disk_cx_pack			# Pack CHS for BIOS.
	mov	$1,			%al	# Read 1 sector at a time.
	mov	$BIOS_DISK_READ_AH,	%ah	# Call BIOS.
	int	$BIOS_INT_DISK
	jc	error				# Bail if there was an error.

	/* Print progress indicator. */
	print_char	$'.

	/* Debug: Print a small piece the data. */
	.if DEBUG_LOAD_VALUE
	pusha
	print_char	$'\t
	print_word	%es:(%bx)
	popa
	.endif

	/*
	 * Move destination pointer to next sector.
	 *
	 * We move by increasing the _segment_ pointer and leaving the
	 * offset at zero. The offset alone can only address 64 KiB.
	 * Therefore a kernel size greater than 64 KiB would overflow the
	 * offset, causing later reads to overwrite earlier ones.
	 */
	mov	%es,				%ax
	add	$(SECTOR_SIZE_BYTES >> 4),	%ax
	mov	%ax,				%es

	call	disk_cx_unpack		# Unpack CHS address for increment.
	call	disk_chs_inc_unpacked	# Increment.
	dec	%si			# Decrement number of sectors to read.

	jmp	3b			# Continue.
4:
	ret


/* === End of used space === */

	/*
	 * Mark end of used space.
	 *
	 * We can check the location of this symbol to check how much space is
	 * available in the bootblock for more code.
	 */
	.globl	end_used_space
end_used_space:

	/*
	 * Boot sector signature
	 */
. = (SECTOR_SIZE_BYTES - 2)	# At the end of the block,
	.byte	0x55, 0xaa	#	place the signature bytes.

