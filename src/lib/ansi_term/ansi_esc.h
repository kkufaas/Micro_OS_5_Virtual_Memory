/*
 * Constants for ANSI escape codes to control terminal output
 *
 * For a summary of ANSI escape codes and how they work, see:
 *
 * - Wikipedia: <https://en.wikipedia.org/wiki/ANSI_escape_code>
 * - OSDev Wiki: <https://wiki.osdev.org/Terminals>
 *
 * The full standard is ECMA-48:
 * <https://ecma-international.org/publications-and-standards/standards/ecma-48/>
 */
#ifndef ANSI_ESCAPE_H
#define ANSI_ESCAPE_H

#define ANSI_ESC '\033' // Escape: starts all escape sequences

/* Fe Escape Sequences */

#define ANSI_CSI '[' // Control Sequence Introducer

/* CSI Sequences */

#define ANSI_CUP 'H' // ESC CSI n ; m H   = Cursor Position to (row n, col m)
#define ANSI_ED  'J' // ESC CSI n J       = Erase in Display (mode n)
#define ANSI_EL  'K' // ESC CSI n K       = Erase in Line (mode n)

/* Erase mode parameters */

#define ANSI_EFWD  0 // Erase parameter: from cursor forward
#define ANSI_EBACK 1 // Erase parameter: from cursor backward
#define ANSI_EFULL 2 // Erase parameter: whole line/display

/* Printf strings for common operations */

#define ANSIF_CUP "\033[%d;%dH" // Cursor position (row, col) (1-indexed)
#define ANSIF_ED  "\033[%dJ"    // Erase in Display (mode)
#define ANSIF_EL  "\033[%dK"    // Erase in Display (mode)

#endif /* ANSI_ESCAPE_H */
