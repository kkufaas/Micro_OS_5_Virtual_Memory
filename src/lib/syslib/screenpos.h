#ifndef SCREENPOS_H
#define SCREENPOS_H

#include <ansi_term/termbuf.h>

/* === Left column === */

#define CLOCK_TERM_INIT TERM_INIT_VGA_WIN(0, 1, 0, 30) // Clock
#define LOCKS_TERM_INIT TERM_INIT_VGA_WIN(1, 2, 0, 30) // Lock test

#define SUM_TERM_INIT TERM_INIT_VGA_WIN(7, 1, 0, 40) // Sum process

#define PROCTAB_TERM_INIT TERM_INIT_VGA_WIN(9, 13, 0, 40) // Process tab

/* === Right column === */

#define MONTE_TERM_INIT TERM_INIT_VGA_WIN(0, 5, 40, 30) // Monte Carlo Pi

#define PLANE_TERM_INIT TERM_INIT_VGA_WIN(7, 4, 40, 40) // Plane

/* === Footer: printk log === */

#define PRINTK_TERM_INIT TERM_INIT_VGA_WIN(22, 3, 0, 80)

#endif /* SCREENPOS_H */
