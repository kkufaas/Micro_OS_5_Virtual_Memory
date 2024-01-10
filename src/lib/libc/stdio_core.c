#include "stdio.h"

#include <limits.h>
#include <stdint.h>
#include "ctype.h"
#include "errno.h"
#include "string.h"

#include "math.h"

#define UNUSED(var) ((void) var)

/* === Basic file information === */

int ferror(FILE *stream) { return stream->error != 0; }

static size_t written_ct(FILE *stream)
{
    return stream->written_flushed_ct
           + (stream->write_ptr - stream->write_base);
}

int setvbuf(FILE *stream, char *buffer, int mode, size_t size)
{
    if (buffer) {
        stream->write_base = (unsigned char *) buffer;
        stream->write_ptr  = (unsigned char *) buffer;
        stream->write_end  = (unsigned char *) buffer + size;
    }
    stream->buffer_mode = mode;
    return 0;
}

/* === File operations that use the vtable === */

int fflush(FILE *stream) { return stream->vtable->fflush(stream); }

/* === File and vtable setup for string-only operation (sprintf, etc.) === */

static int fflush_noop(FILE *stream)
{
    UNUSED(stream);
    return 0;
}

static const struct _file_vtable FILE_VTABLE_STRING_IO = {
        .fflush = fflush_noop,
};

static void file_init_string_io(FILE *stream, char *buf, size_t bufsz)
{
    *stream = (struct _file_impl){
            .vtable = &FILE_VTABLE_STRING_IO,
    };
    setvbuf(stream, buf, _IOFBF, bufsz);
}

/* === Simple output to files === */

int fputc(int ch, FILE *stream)
{
    /* If the write pointer is in bounds of the buffer, write the character. */
    if (stream->write_ptr < stream->write_end)
        *stream->write_ptr = (unsigned char) ch;

    /* Advance the write pointer either way.
     * This allows snprintf to calculate the number of characters that would be
     * written, if there was enough space. */
    stream->write_ptr++;

    /* Flush the stream if needed. */
    if (stream->buffer_mode == _IONBF // Unbuffered: flush on every char
        || (stream->buffer_mode == _IOLBF && ch == '\n') // Line: flush on EOL
        || (stream->write_ptr == stream->write_end))     // Buffer full
        fflush(stream);

    return ferror(stream) ? EOF : ch;
}

/* Write a string to file, only up to the given number of characters */
static int fputsn(const char *str, size_t n, FILE *file)
{
    if (!str) return 0;

    for (; n && *str; n--, str++) {
        putc(*str, file);
    }

    return ferror(file) ? EOF : 0;
}

int fputs(const char *str, FILE *file) { return fputsn(str, SIZE_MAX, file); }

/* === Parsing printf format strings === */

#define WP_UNSPECIFIED (-1)
#define WP_STAR        (-2)

enum lenmod {
    LEN_NONE,
    LEN_CHAR,
    LEN_SHORT,
    LEN_LONG,
    LEN_LONG_LONG,
    LEN_MAX,
    LEN_SIZE,
    LEN_PTRDIFF,
    LEN_FIXED,
    LEN_FAST,
    LEN_LONG_DOUBLE,
    LEN_DECIMAL32,
    LEN_DECIMAL64,
    LEN_DECIMAL128,
};

struct fmtspec {
    const char *start;
    size_t      len;
    bool        left;
    bool        plus;
    bool        space;
    bool        alt;
    bool        zero;
    int         width;
    int         precision;
    enum lenmod lenmod;
    size_t      lenbits;
    char        specifier;
};

static const char *parse_flags(const char *fmt, struct fmtspec *spec)
{
    bool endflags = false;
    while (!endflags) {
        switch (*fmt) {
        case '-': spec->left = true; break;
        case '+': spec->plus = true; break;
        case ' ': spec->space = true; break;
        case '#': spec->alt = true; break;
        case '0': spec->zero = true; break;
        default: endflags = true;
        }
        if (!endflags) fmt++;
    }
    return fmt;
}

static const char *parse_width_precision(const char *fmt, int *target)
{
    if (*fmt == '*') {
        *target = WP_STAR;
        return fmt + 1;
    }

    const char *start  = fmt;
    int         result = 0;
    for (; *fmt && isdigit(*fmt); fmt++) {
        int digit = *fmt - '0';
        result    = result * 10 + digit;
    }

    if (fmt == start) *target = WP_UNSPECIFIED;
    else *target = result;

    return fmt;
}

struct lenmod_mapping {
    char        ch0;
    enum lenmod lm0;
    char        ch1;
    enum lenmod lm1;
};

const struct lenmod_mapping LENMOD_MAP[] = {
        {'h', LEN_SHORT, 'h', LEN_CHAR},
        {'l', LEN_LONG, 'l', LEN_LONG_LONG},
        {'j', LEN_MAX, 0, LEN_NONE},
        {'z', LEN_SIZE, 0, LEN_NONE},
        {'t', LEN_PTRDIFF, 0, LEN_NONE},
        {'w', LEN_FIXED, 'f', LEN_FAST},
        {'L', LEN_LONG_DOUBLE, 0, LEN_NONE},
        {'H', LEN_DECIMAL32, 0, LEN_NONE},
        {'D', LEN_DECIMAL64, 'D', LEN_DECIMAL128},
        {0, 0, 0, 0},
};

static const char *parse_lenmod(const char *fmt, struct fmtspec *spec)
{
    spec->lenmod = LEN_NONE;
    for (const struct lenmod_mapping *map = LENMOD_MAP; map->ch0 != 0; map++) {
        if (map->ch1 && fmt[0] == map->ch0 && fmt[1] == map->ch1) {
            spec->lenmod = map->lm1;
            fmt += 2;
        } else if (fmt[0] == map->ch0) {
            spec->lenmod = map->lm0;
            fmt += 1;
        }
    }

    spec->lenbits = 0;
    if (spec->lenmod == LEN_FIXED || spec->lenmod == LEN_FAST) {
        for (; *fmt && isdigit(*fmt); fmt++) {
            spec->lenbits *= 10;
            spec->lenbits += *fmt - '0';
        }
    }

    return fmt;
}

static const char *parse_spec(const char *fmt, struct fmtspec *spec)
{
    *spec = (struct fmtspec){0};

    spec->start = fmt;
    spec->len   = 0;

    if (*fmt != '%') {
        /* Non-pattern part of format string */
        spec->specifier = '\0';
        for (; *fmt && *fmt != '%'; fmt++, spec->len++) {
        }
        return fmt;
    }

    /* Consume opening '%' */
    fmt++;

    fmt = parse_flags(fmt, spec);

    spec->width = WP_UNSPECIFIED;
    fmt         = parse_width_precision(fmt, &spec->width);

    spec->precision = WP_UNSPECIFIED;
    if (*fmt == '.') {
        fmt++;
        fmt = parse_width_precision(fmt, &spec->precision);
        if (spec->precision == WP_UNSPECIFIED) {
            /* "If only the period is specified, the precision is taken as
             * zero." */
            spec->precision = 0;
        }
    }

    fmt = parse_lenmod(fmt, spec);

    spec->specifier = *fmt;
    fmt++;
    spec->len = fmt - spec->start;

    return fmt;
}

/* === Printing formatted values === */

static void fpad(FILE *file, int ch, int count)
{
    for (int i = count; i > 0; i--) {
        putc(ch, file);
    }
}

#define min(x, y) ((x) < (y) ? (x) : (y))

static void fmt_str_val(
        FILE           *file,
        struct fmtspec *spec,
        const char     *prefix,
        size_t          zeropad,
        const char     *val,
        size_t          limit
)
{
    size_t len = strlen(prefix) + zeropad + min(strlen(val), (int) limit);
    size_t pad = (size_t) spec->width > len ? spec->width - len : 0;

    if (pad && !spec->left) fpad(file, ' ', pad);

    if (prefix) fputs(prefix, file);
    if (zeropad) fpad(file, '0', zeropad);

    fputsn(val, limit, file);

    if (pad && spec->left) fpad(file, ' ', pad);
}

static void fmt_char(FILE *file, struct fmtspec *spec, int ch)
{
    char chstr[] = "_";
    chstr[0]     = (unsigned char) ch;
    fmt_str_val(file, spec, NULL, 0, chstr, 1);
}

static void fmt_str(FILE *file, struct fmtspec *spec, const char *str)
{
    size_t limit =
            spec->precision == WP_UNSPECIFIED ? INT_MAX : spec->precision;

    fmt_str_val(file, spec, NULL, 0, str, limit);
}

#define DIGITBUFSZ 129 /* Enough for a 128-bit number in binary */

const char lowerhex[] = "0123456789abcdef";
const char upperhex[] = "0123456789ABCDEF";

static char *inttodigits(
        char *buf_right, uintmax_t num, unsigned int base, const char *digitset
)
{
    char *cur = buf_right; // We'll work from right to left.

    while (num) {
        uintmax_t    div = num / base;
        unsigned int mod = num % base;

        *--cur = digitset[mod];
        num    = div;
    }

    return cur;
}

static const char *signstr(bool isneg, struct fmtspec *spec)
{
    if (isneg) return "-";
    else if (spec->plus) return "+";
    else if (spec->space) return " ";
    else return "";
}

static void fmt_signed(FILE *file, struct fmtspec *spec, intmax_t val)
{
    bool      isneg = val < 0;
    uintmax_t uval  = val < 0 ? -val : val;

    char  digitbuf[DIGITBUFSZ];
    char *digits = digitbuf + DIGITBUFSZ; // We'll work from right to left.
    *--digits    = '\0';                  // Starting with the null terminator.
    digits       = inttodigits(digits, uval, 10, lowerhex);
    int digitct  = &digitbuf[DIGITBUFSZ - 1] - digits;

    int zeropad = 0;

    if (spec->precision == WP_UNSPECIFIED) spec->precision = 1;
    if (spec->precision > digitct) zeropad += spec->precision - digitct;

    const char *prefix = signstr(val < 0, spec);
    int         len    = strlen(prefix) + zeropad + digitct;
    if (spec->width > len && spec->zero) zeropad += spec->width - len;

    fmt_str_val(file, spec, prefix, zeropad, digits, INT_MAX);
}

static void fmt_unsigned(FILE *file, struct fmtspec *spec, uintmax_t uval)
{
    unsigned int base;
    switch (spec->specifier) {
    case 'b':
    case 'B': base = 2; break;
    case 'o': base = 8; break;
    case 'x':
    case 'X': base = 16; break;
    case 'u': base = 10; break;
    default: base = 10; /* unreachable */
    }

    const char *digitset = spec->specifier == 'X' ? upperhex : lowerhex;

    char  digitbuf[DIGITBUFSZ];
    char *digits = digitbuf + DIGITBUFSZ; // We'll work from right to left.
    *--digits    = '\0';                  // Starting with the null terminator.
    digits       = inttodigits(digits, uval, base, digitset);
    int digitct  = &digitbuf[DIGITBUFSZ - 1] - digits;

    int zeropad = 0;

    if (spec->precision == WP_UNSPECIFIED) spec->precision = 1;
    if (spec->precision > digitct) zeropad += spec->precision - digitct;

    const char *prefix = "";
    if (spec->alt) {
        switch (spec->specifier) {
        case 'b': prefix = "0b"; break;
        case 'B': prefix = "0B"; break;
        case 'x': prefix = "0x"; break;
        case 'X': prefix = "0X"; break;
        case 'o':
            if (!zeropad && *digits != '0') prefix = "0";
            break;
        }
    }

    int len = strlen(prefix) + zeropad + digitct;
    if (spec->width > len && spec->zero) zeropad += spec->width - len;

    fmt_str_val(file, spec, prefix, zeropad, digits, INT_MAX);
}

#define FLOATBUFSZ 256

static void fmt_float(FILE *file, struct fmtspec *spec, long double dval)
{
    if (spec->precision == WP_UNSPECIFIED) spec->precision = 6;

    char  buf[FLOATBUFSZ];
    char *str = buf + FLOATBUFSZ; // We'll work from right to left.
    *--str    = '\0';             // Starting with the null terminator.
    char *end = str;              // Mark the end for length comparison.

    char prefixbuf[4] = {};

    /* Special cases */
    switch (fpclassify(dval)) {
    case FP_NORMAL:
    case FP_SUBNORMAL:
    case FP_ZERO: break;

    case FP_INFINITE:
        if (isupper(spec->specifier)) fmt_str(file, spec, "INF");
        else fmt_str(file, spec, "inf");
        return;
    case FP_NAN:
        if (isupper(spec->specifier)) fmt_str(file, spec, "NAN");
        else fmt_str(file, spec, "nan");
        return;
    };

    /* Separate sign */
    bool        isneg = signbitl(dval);
    long double uval  = fabsl(dval);
    strcat(prefixbuf, signstr(isneg, spec));

    /* Determine base (decimal or hex) and need for exponent notation. */
    unsigned int base       = 10;
    char         exp_marker = 0;
    const char  *digitset   = lowerhex;

    switch (spec->specifier) {
    case 'e':
    case 'E': exp_marker = spec->specifier; break;
    case 'a':
        strcat(prefixbuf, "0x");
        exp_marker = 'p';
        base       = 16;
        break;
    case 'A':
        strcat(prefixbuf, "0X");
        exp_marker = 'P';
        base       = 16;
        break;
    }

    /* Calculate exponent (if necessary) */
    if (exp_marker) {
        uintmax_t nval = uval; // Integer version of the value
        long      exp  = 0;    // Exponent
        long      mul  = 1;    // Multiplier: 1^exp

        while (nval >= base) {
            exp++;
            mul *= base;
            nval = uval / mul;
        }

        while (nval < 1 && !iszero(uval)) {
            exp--;
            mul *= base;
            nval = uval * mul;
        }

        /* Write exponent to buffer. */
        char *exp_end = str;
        str           = inttodigits(str, labs(exp), base, lowerhex);
        /* The spec says that exponent part should show at least two digits. */
        while (exp_end - str < 2) *--str = '0';

        *--str = exp >= 0 ? '+' : '-';
        *--str = exp_marker;

        /* Normalize the value. */
        uval = exp > 0 ? uval / mul : uval * mul;
    }

    /* Separate whole part from fractional part. */
    unsigned int mul = 1;
    for (int i = 0; i < spec->precision; i++) mul *= base;
    uintmax_t   wholepart = (uintmax_t) uval;
    long double dfracpart = (uval - wholepart) * mul;
    uintmax_t   fracpart  = (uintmax_t) dfracpart;
    if (dfracpart - fracpart >= 0.5) fracpart += 1; // Round up if needed

    /* Write fractional part into buffer. */
    char *frac_end = str;
    str            = inttodigits(str, fracpart, base, lowerhex);
    while (frac_end - str < spec->precision) *--str = '0';

    /* Write decimal point if necessary. */
    if (fracpart || spec->precision || spec->alt) *--str = '.';

    /* Write whole part into buffer. */
    str = inttodigits(str, wholepart, base, lowerhex);

    /* C Standard: "If a decimal-point character appears, at least one digit
     * appears before it." */
    if (*str == '.') *--str = '0';

    /* Zero-pad if necessary. */
    if (spec->zero) {
        int prelen = strlen(prefixbuf);
        while (end - str - prelen < spec->width) *--str = '0';
    }

    fmt_str_val(file, spec, prefixbuf, 0, str, INT_MAX);
}

static void fmt_pointer(FILE *file, struct fmtspec *spec, void *p)
{
    /* Print a pointer as a fixed-width hex value. */
    spec->lenmod    = LEN_PTRDIFF;        // This will pull a uintptr_t.
    spec->specifier = 'x';                // Print as hex.
    spec->alt       = true;               // Use the 0x prefix.
    spec->precision = sizeof(void *) * 2; // Print full, 2 hex digits per byte.
    fmt_unsigned(file, spec, (uintptr_t) p);
}

/* === Main vfprintf function === */

int vfprintf(FILE *file, const char *format, va_list args)
{
    size_t start_ct = written_ct(file);

    while (*format) {

        /* Parse the format spec. */

        struct fmtspec spec;
        format = parse_spec(format, &spec);

        /*
         * Note that all pulling of arguments from the va_list must be done in
         * the same function. Therefore it is not really possible to break this
         * up into smaller functions.
         *
         * C Standard: "The object ap may be passed as an argument to another
         * function; if that function invokes the va_arg macro with parameter
         * ap, the representation of ap in the calling function is
         * indeterminate and shall be passed to the va_end macro prior to any
         * further reference to ap."
         *
         * There is a footnote that says that you may pass a pointer to the
         * va_list to another function. However that may not work in practice.
         * We tried it with this code, but it caused errors when running the
         * unit tests on an x86-64 host machine. It seems the only safe thing
         * to do is to keep all va_arg calls in the same function.
         *
         * | arch   | va_list type      | pass va_list      | pass va_list* |
         * |--------|-------------------|-------------------|---------------|
         * | i386   | void*             | wild behavior     | ok            |
         * | x86-64 | struct {...} [1]  | happens to work   | wild behavior |
         *
         * See the System V ABI architecture supplements for specifics of how
         * variable arguments are implemented on different architectures.
         */

        /* Pull width and precision from args if necessary, then adjust. */

        if (spec.width == WP_UNSPECIFIED) spec.width = 0;
        if (spec.width == WP_STAR) spec.width = va_arg(args, int);
        if (spec.width < 0) {
            /* C Standard: "A negative field width argument is taken as a '-'
             * flag followed by a positive field width." */
            spec.left  = true;
            spec.width = -spec.width;
        }

        if (spec.precision == WP_STAR) spec.precision = va_arg(args, int);
        if (spec.precision < 0) {
            /* C Standard: "A negative precision argument is taken as if the
             * precision were omitted." */
            spec.precision = WP_UNSPECIFIED;
        }

        /* Pull the specified argument and call the appropriate format fn. */

        switch (spec.specifier) {
        case '\0': fputsn(spec.start, spec.len, file); break;

        case '%':
            if (spec.len != 2) return -EINVAL;
            putc('%', file);
            break;

        case 'c': {
            if (spec.lenmod != LEN_NONE) return -EUNIMPLEMENTED;
            int ch = va_arg(args, int); // char is promoted to int
            fmt_char(file, &spec, ch);
            break;
        }

        case 's': {
            if (spec.lenmod != LEN_NONE) return -EUNIMPLEMENTED;
            const char *str = va_arg(args, const char *);
            fmt_str(file, &spec, str);
            break;
        }

        case 'd':
        case 'i': {
            intmax_t val = 0;
            switch (spec.lenmod) {
            case LEN_CHAR:  // Promoted to int
            case LEN_SHORT: // Promoted to int
            case LEN_NONE: val = va_arg(args, signed int); break;
            case LEN_LONG: val = va_arg(args, signed long); break;
            case LEN_LONG_LONG: val = va_arg(args, signed long long); break;
            case LEN_MAX: val = va_arg(args, intmax_t); break;
            case LEN_SIZE: val = va_arg(args, intptr_t); break;
            case LEN_PTRDIFF: val = va_arg(args, ptrdiff_t); break;

            case LEN_FIXED:
            case LEN_FAST:
                if (spec.lenbits <= sizeof(signed int) * 8)
                    val = va_arg(args, signed int);
                else if (spec.lenbits <= sizeof(signed long) * 8)
                    val = va_arg(args, signed long);
                else if (spec.lenbits <= sizeof(signed long long) * 8)
                    val = va_arg(args, signed long);
                else val = va_arg(args, intmax_t);
                break;

            case LEN_LONG_DOUBLE:
            case LEN_DECIMAL32:
            case LEN_DECIMAL64:
            case LEN_DECIMAL128: return -EINVAL; // Invalid for this pattern
            }

            fmt_signed(file, &spec, val);
            break;
        }

        case 'b':
        case 'B':
        case 'o':
        case 'x':
        case 'X':
        case 'u': {
            uintmax_t uval = 0;
            switch (spec.lenmod) {
            case LEN_CHAR:  // Promoted to int
            case LEN_SHORT: // Promoted to int
            case LEN_NONE: uval = va_arg(args, unsigned int); break;
            case LEN_LONG: uval = va_arg(args, unsigned long); break;
            case LEN_LONG_LONG: uval = va_arg(args, unsigned long long); break;
            case LEN_MAX: uval = va_arg(args, uintmax_t); break;
            case LEN_SIZE: uval = va_arg(args, uintptr_t); break;
            case LEN_PTRDIFF: uval = va_arg(args, uintptr_t); break;

            case LEN_FIXED:
            case LEN_FAST:
                if (spec.lenbits <= sizeof(unsigned int) * 8)
                    uval = va_arg(args, unsigned int);
                else if (spec.lenbits <= sizeof(unsigned long) * 8)
                    uval = va_arg(args, unsigned long);
                else if (spec.lenbits <= sizeof(unsigned long long) * 8)
                    uval = va_arg(args, unsigned long long);
                else uval = va_arg(args, uintmax_t);
                break;

            case LEN_LONG_DOUBLE:
            case LEN_DECIMAL32:
            case LEN_DECIMAL64:
            case LEN_DECIMAL128: return -EINVAL; // Invalid for this pattern
            }

            fmt_unsigned(file, &spec, uval);
            break;
        }

        case 'p': {
            void *p = va_arg(args, void *);
            fmt_pointer(file, &spec, p);
            break;
        }

        case 'f':
        case 'F':
        case 'e':
        case 'E':
        case 'a':
        case 'A': {
            long double dval = 0;
            switch (spec.lenmod) {
            case LEN_CHAR:
            case LEN_SHORT: return -EINVAL; // Invalid for this pattern

            case LEN_NONE:
            case LEN_LONG: dval = va_arg(args, double); break;

            case LEN_LONG_LONG:
            case LEN_MAX:
            case LEN_SIZE:
            case LEN_PTRDIFF:
            case LEN_FIXED:
            case LEN_FAST: return -EINVAL; // Invalid for this pattern

            case LEN_LONG_DOUBLE: dval = va_arg(args, long double); break;

            case LEN_DECIMAL32:
            case LEN_DECIMAL64:
            case LEN_DECIMAL128: return -EUNIMPLEMENTED;
            }
            fmt_float(file, &spec, dval);
            break;
        }

        default: return -EUNIMPLEMENTED;
        }
    }

    if (ferror(file)) return -EIO;
    return written_ct(file) - start_ct;
}

__attribute__((format(printf, 2, 3))) int
fprintf(FILE *file, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int count = vfprintf(file, format, args);
    va_end(args);
    return count;
}

/* === String printf family === */

int vsnprintf(char *buf, size_t bufsz, const char *format, va_list args)
{
    FILE str_file;
    file_init_string_io(&str_file, buf, bufsz);

    int count = vfprintf(&str_file, format, args);

    /* Place null terminator */
    if (count < 0) return count;                        // Error: no terminator
    else if ((size_t) count < bufsz) buf[count] = '\0'; // End of string
    else if (bufsz) buf[bufsz - 1] = '\0';              // End of buffer

    return count;
}

int snprintf(char *buffer, size_t bufsz, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int count = vsnprintf(buffer, bufsz, format, args);
    va_end(args);
    return count;
}

int vsprintf(char *buffer, const char *format, va_list args)
{
    return vsnprintf(buffer, INT_MAX, format, args);
}

int sprintf(char *buffer, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int count = vsnprintf(buffer, INT_MAX, format, args);
    va_end(args);
    return count;
}

