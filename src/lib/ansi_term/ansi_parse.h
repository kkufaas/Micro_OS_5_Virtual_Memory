/*
 * Limited parser for select ANSI escape codes
 */
#ifndef ANSI_PARSE_H
#define ANSI_PARSE_H

enum ansi_result {
    APS_ESCAPE = 1,    // Character is part of an escape sequence in progress
    APS_FINAL,         // End of escape sequence; terminal can execute it
    APS_INVALID,       // Escape sequence is invalid
    APS_UNIMPLEMENTED, // This type of sequence is not implemented
};

enum _ansi_inner_state {
    AIS_READY,        // Not in an escape sequence
    AIS_ESC,          // ESC byte encountered
    AIS_PARAM,        // Processing Control Sequence parameter bytes
    AIS_INTERMEDIATE, // Processing Control Sequence intermediate bytes
};

#define CSI_MAX_PARAM 3

struct ansi_parse {
    enum _ansi_inner_state _state;
    int                    csi_param[CSI_MAX_PARAM];
    int                    param_num;
    char                   cmd; // Final char that determines command
};

#define ANSI_PARSE_INIT \
    { \
    }

int ansi_parse_putc(struct ansi_parse *parser, int ch);

#endif /* ANSI_PARSE_H */
