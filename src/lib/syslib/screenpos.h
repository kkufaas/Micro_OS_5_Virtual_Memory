#ifndef SCREENPOS_H
#define SCREENPOS_H

#include <ansi_term/termbuf.h>

/* === Left column === */

#define CLOCK_TERM_INIT TERM_INIT_VGA_WIN(0, 1, 0, 30) // Clock
#define LOCKS_TERM_INIT TERM_INIT_VGA_WIN(1, 2, 0, 30) // Lock test

/* Message passing processes */
#define PROC3_TERM_INIT TERM_INIT_VGA_WIN(4, 2, 0, 15)  // IPC test A
#define PROC4_TERM_INIT TERM_INIT_VGA_WIN(4, 2, 15, 15) // IPC test B

#define SUM_TERM_INIT TERM_INIT_VGA_WIN(7, 1, 0, 40) // Sum process

#define PROCTAB_TERM_INIT TERM_INIT_VGA_WIN(9, 13, 0, 40) // Process tab

/* === Right column === */

#define SHELL_SIZEX     50
#define SHELL_TERM_INIT TERM_INIT_VGA_WIN(0, 7, 30, SHELL_SIZEX) // Shell

#define PLANE_TERM_INIT TERM_INIT_VGA_WIN(7, 4, 40, 40) // Plane

#define MBOXTAB_TERM_INIT TERM_INIT_VGA_WIN(12, 9, 40, 40) // Mbox table
#define PHL_TERM_INIT TERM_INIT_VGA_WIN(16, 3, 40, 40) // Philosophers
#define BAR_TERM_INIT TERM_INIT_VGA_WIN(19, 3, 40, 40) // Barrier test

/* === Footer: printk log === */

#define PRINTK_TERM_INIT TERM_INIT_VGA_WIN(22, 3, 0, 80)

#endif /* SCREENPOS_H */
