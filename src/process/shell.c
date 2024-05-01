#include <stdlib.h>
#include <string.h>

#include <ansi_term/ansi_esc.h>
#include <ansi_term/termbuf.h>
#include <ansi_term/tprintf.h>
#include <syslib/common.h>
#include <syslib/screenpos.h>
#include <util/util.h>

#include "syslib.h"

#define COMMAND_MBOX 1 /* mbox to send commands to process1 (plane) */
#define LINE_MAX     (SHELL_SIZEX * 3)
#define CMD_LS_PROCS "ls"

static struct term term = SHELL_TERM_INIT;

/* Print character in the shell window */
static int shprintf(char *in, ...)
{
    va_list args;
    va_start(args, in);
    int ret = vtprintf(&term, in, args);
    va_end(args);
    return ret;
}

/*
 * Read characters from command line into 'line'. This function calls
 * getchar() to recive a character. It returns when either 'max'
 * characters are read or "enter" is read
 */
static void read_line(int max, char *line);

/*
 * Parse command in 'line'. Arguments are stored in argv. Returns
 * number of arguments
 */
static int parse_line(char *line, char *argv[SHELL_SIZEX]);

/* cursor coordinate */
int cursor = 0;

int main(void)
{
    int                 q, p, n;
    int                 rc, load_code;
    unsigned char       buf[SECTOR_SIZE]; /* directory buffer */
    struct directory_t *dir = (struct directory_t *) buf;
    int                 argc;              /* argument count */
    char               *argv[SHELL_SIZEX]; /* argument vector */
    char                line[LINE_MAX];    /* unparsed command */

    term.show_cursor = true;
    shprintf("D-240 Shell Version 0.00001\n\n");

    /* open mbox for communication with plane */
    q = mbox_open(COMMAND_MBOX);

    while (1) {
        shprintf("$ ");
        /* read command into line[] */
        read_line(LINE_MAX, line);

        /*
         * Parses command in line[] into *argv[], and returns
         * number of arguments.
         */
        argc = parse_line(line, argv);
        /* if no arguments goto beginning of loop */
        if (argc == 0) {
            continue;
        }

        /* clear screen */
        if (same_string("clear", argv[0])) {
            if (argc == 1) {
                /* Clear shell window */
                shprintf(ANSIF_ED, ANSI_EFULL);
                cursor = 0;
            } else {
                shprintf("Usage: %s\n", argv[0]);
            }
        }
        /*
         * Send "fire" command through the mailbox to any
         * listening plane
         */
        else if (same_string("fire", argv[0])) {
            if (argc == 1) {
                if (q >= 0) {
                    msg_t m;
                    m.size = 0;
                    mbox_send(q, &m);
                } else {
                    shprintf("Mailboxes not available\n");
                }
            } else shprintf("Usage: %s\n", argv[0]);
        }
        /* Close shell */
        else if (same_string("exit", argv[0])) {
            if (argc == 1) {
                shprintf("Goodbye");
                exit();
            } else shprintf("Usage: %s\n", argv[0]);
        } else if (same_string(CMD_LS_PROCS, argv[0])) {
            if (argc == 1) {
                rc = readdir(buf);
                if (rc < 0) {
                    shprintf("Read error.\n");
                    continue;
                }
                dir = (struct directory_t *) buf;

                /* number of proceses in directory */
                p = 0;
                /* parse directory */
                while (dir->location != 0) {
                    shprintf(
                            "process %d - location: %d, size: %d\n", p++,
                            dir->location, dir->size
                    );
                    dir++;
                }

                if (p == 0) {
                    shprintf("Process not found.\n");
                } else {
                    shprintf("%d process(es).\n", p);
                }
            } else {
                shprintf("Usage: %s\n", argv[0]);
            }
        } else if (same_string("load", argv[0])) {
            if (argc == 2) {
                rc = readdir(buf);
                if (rc < 0) {
                    shprintf("Read error.\n");
                    continue;
                }

                dir = (struct directory_t *) buf;
                n   = atoi(argv[1]);
                for (p = 0; p < n && dir->location != 0; p++, dir++)
                    ;

                if (dir->location != 0) {
                    load_code = loadproc(dir->location, dir->size);
                    if (load_code == 0) shprintf("Done.\n");
                    else if (load_code == 1) shprintf("Too much competition for pages. Try again later.\n");
                    else shprintf("error");
                } else {
                    shprintf("File not found.\n");
                }
            } else {
                shprintf("usage: %s  'process number'\n", argv[0]);
            }
        } else if (same_string("ps", argv[0])) {
            shprintf("%s : Command not implemented.\n", argv[0]);
        } else if (same_string("kill", argv[0])) {
            shprintf("%s : Command not implemented.\n", argv[0]);
        } else if (same_string("suspend", argv[0])) {
            shprintf("%s : Command not implemented.\n", argv[0]);
        } else if (same_string("resume", argv[0])) {
            shprintf("%s : Command not implemented.\n", argv[0]);
        } else {
            shprintf("%s : Command not found.\n", argv[0]);
        }
    }

    return -1;
}

/*
 * Parse command in 'line'.
 * Pointers to arguments (in line) are stored in argv.
 * Returns number of arguments.
 */
static int parse_line(char *line, char *argv[SHELL_SIZEX])
{
    int   argc;
    char *s = line;

    argc = 0;
    while (1) {
        /* Jump over blanks */
        while (*s == ' ') {
            /* All blanks are replaced with '\0' */
            *s = '\0';
            s++;
        }
        /* End of line reached, no more arguments */
        if (*s == '\0') return argc;
        else {
            /* Store pointer to argument in argv */
            argv[argc++] = s;
            /* And goto next blank character (or end of line) */
            while ((*s != ' ') && (*s != '\0')) s++;
        }
    }
}

/*
 * Read characters from command line into 'line'. This function calls
 * getchar() to recive a character. It returns when either 'max' characters
 * are read or "enter" is read
 */
static void read_line(int max, char *line)
{
    int i = 0;
    int c;

    do {
        getchar(&c);

        switch (c) {
        case '\r':
            line[i] = '\0';
            shprintf("\n");
            return;
        case '\b':
            if (i > 0) {
                line[--i] = '\0';
                shprintf("\b");
            }
            break;
        default: line[i++] = c; shprintf("%c", c);
        }
    } while (i < max - 1);
    line[i] = '\0';
    shprintf("\n");
}

