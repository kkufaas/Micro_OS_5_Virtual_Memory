#include "ansi_parse.h"

#include "ansi_esc.h"

#define UNUSED(param) ((void) param)

static void start_esc(struct ansi_parse *parser)
{
    parser->_state = AIS_ESC;
    for (int i = 0; i < CSI_MAX_PARAM; i++) parser->csi_param[i] = 0;
    parser->param_num = 0;
    parser->cmd       = 0;
}

static void start_csi(struct ansi_parse *parser)
{
    parser->_state = AIS_PARAM;
}

static void parameter_byte(struct ansi_parse *parser, char ch)
{
    if ('0' <= ch && ch <= '9' && parser->param_num < CSI_MAX_PARAM) {
        int digit = ch - '0';
        parser->csi_param[parser->param_num] *= 10;
        parser->csi_param[parser->param_num] += digit;
        return;
    }

    switch (ch) {
    case ';': // next param
        parser->param_num++;
        return;

    default: return;
    }
}

static void intermediate_byte(struct ansi_parse *parser, char ch)
{
    UNUSED(parser);
    UNUSED(ch);
}

static void final_byte(struct ansi_parse *parser, char ch)
{
    parser->cmd    = ch;
    parser->_state = AIS_READY;
}

/*
 * Passes the given character through the parser
 *
 * Returns:
 *
 * If the character is not part of an escape sequence, it is returned as-is.
 * If the character is part of an escape sequence, a negative value is
 * returned indicating parse state.
 */
int ansi_parse_putc(struct ansi_parse *parser, int ch)
{
    switch (parser->_state) {
    case AIS_READY:
        switch (ch) {
        case ANSI_ESC: start_esc(parser); return -APS_ESCAPE;
        default: return ch;
        }

    case AIS_ESC:
        switch (ch) {
        case ANSI_CSI: start_csi(parser); return -APS_ESCAPE;
        default: return -APS_UNIMPLEMENTED;
        }

    case AIS_PARAM:
        if (0x30 <= ch && ch <= 0x3f) {
            parameter_byte(parser, ch);
            return -APS_ESCAPE;
        }

        /* End of parameter bytes */
        parser->_state = AIS_INTERMEDIATE;

        /* FALLTHROUGH */

    case AIS_INTERMEDIATE:
        if (0x20 <= ch && ch <= 0x2f) {
            intermediate_byte(parser, ch);
            return -APS_ESCAPE;
        }

        if (0x40 <= ch && ch <= 0x7e) {
            final_byte(parser, ch);
            return -APS_FINAL;
        }

        return -APS_INVALID;
    }

    return ch;
}

