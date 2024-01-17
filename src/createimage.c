#include <stdarg.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <elf.h>
#include <syslib/addrs.h>
#include <syslib/compiler_compat.h>

#define IMAGE_FILE "./image"
static const char *usage_args[] = {
        "[--extended]",
        "<bootblock>",  "<kernel>",
        "[process...]",
};

#define SECTOR_SIZE  512
#define OS_SIZE_LOC  2
#define BOOT_MEM_LOC 0x7c00

#define UNUSED(var) ((void) var)

/* structure to store command line options */
static struct {
    const char *progname;
    bool        vm;
    bool        extended;
    const char *bootblock;
    const char *kernel;
    size_t       process_count;
    const char **processes;
} options;

/* Image state */
static struct image_t {
    FILE *img; /* the file pointer to the image file */
    FILE *fp;  /* the file pointer to the input file */

    size_t nbytes; /* bytes written so far */
} image;

/* prototypes of local functions */
static void create_image();
static void add_file(struct image_t *im, const char *filename);

ATTR_PRINTFLIKE(1, 2) static void verbose_printf(const char *fmt, ...);
ATTR_PRINTFLIKE(1, 2) static void error(const char *fmt, ...);
ATTR_PRINTFLIKE(1, 2) static void usage_error(const char *fmt, ...);

static void read_ehdr(Elf32_Ehdr *ehdr, FILE *fp);
static void read_phdr(Elf32_Phdr *phdr, FILE *fp, int ph, Elf32_Ehdr ehdr);

static void
write_segment(struct image_t *im, Elf32_Ehdr ehdr, Elf32_Phdr phdr);
static void write_os_size(struct image_t *im);

int main(int argc, const char **argv)
{
    /* process command line options */
    options.progname = argv[0];
    options.vm       = 0;
    options.extended = 0;

    int i = 1;

    /* first, check for flags (args that begin with "--") */
    for (; i < argc && strncmp(argv[i], "--", 2) == 0; i++) {

        if (strcmp(argv[i], "--extended") == 0) {
            options.extended = true;

        } else {
            usage_error("unknown option '%s'\n", argv[i]);
        }
    }

    /* the first arg after the flags is the bootblock */
    if (i >= argc) usage_error("missing bootblock\n");
    options.bootblock = argv[i];
    i++;

    /* then the kernel */
    if (i >= argc) usage_error("missing kernel\n");
    options.kernel = argv[i];
    i++;

    /* remaining arguments are processes */
    options.process_count = argc - i;
    options.processes     = &argv[i];

    create_image();
    return 0;
}

static void create_image()
{
    image.img             = fopen(IMAGE_FILE, "w");
    assert(image.img != NULL);
    image.nbytes = 0;

    add_file(&image, options.bootblock);
    add_file(&image, options.kernel);

    for (size_t i = 0; i < options.process_count; i++) {
        add_file(&image, options.processes[i]);
    }

    write_os_size(&image);

    assert((image.nbytes % SECTOR_SIZE) == 0);
    fclose(image.img);
}

static void add_file(struct image_t *im, const char *filename)
{
    int        ph;
    Elf32_Ehdr ehdr;
    Elf32_Phdr phdr;

    /* open input file */
    im->fp = fopen(filename, "r");
    if (!im->fp) {
        error("%s: could not open file\n", filename);
    }

    /* read ELF header */
    read_ehdr(&ehdr, im->fp);
    printf("0x%04x: %s\n", ehdr.e_entry, filename);

    /* for each program header */
    for (ph = 0; ph < ehdr.e_phnum; ph++) {
        /* read program header */
        read_phdr(&phdr, im->fp, ph, ehdr);

        /* write segment to the image */
        write_segment(im, ehdr, phdr);
    }

    fclose(im->fp);
}

static void read_ehdr(Elf32_Ehdr *ehdr, FILE *fp)
{
    int read_ok = fread(ehdr, sizeof(*ehdr), 1, fp);
    assert(read_ok == 1);
    assert(ehdr->e_ident[EI_MAG1] == 'E');
    assert(ehdr->e_ident[EI_MAG2] == 'L');
    assert(ehdr->e_ident[EI_MAG3] == 'F');
}

static void read_phdr(Elf32_Phdr *phdr, FILE *fp, int ph, Elf32_Ehdr ehdr)
{
    int ret;

    fseek(fp, ehdr.e_phoff + ph * ehdr.e_phentsize, SEEK_SET);
    ret = fread(phdr, sizeof(*phdr), 1, fp);
    assert(ret == 1);

    verbose_printf("\tsegment %d\n", ph);
    verbose_printf("\t\toffset 0x%04x", phdr->p_offset);
    verbose_printf("\t\tvaddr 0x%04x\n", phdr->p_vaddr);
    verbose_printf("\t\tfilesz 0x%04x", phdr->p_filesz);
    verbose_printf("\t\tmemsz 0x%04x\n", phdr->p_memsz);
}

static void write_segment(struct image_t *im, Elf32_Ehdr ehdr, Elf32_Phdr phdr)
{
    UNUSED(ehdr);

    size_t phyaddr;

    if (phdr.p_memsz == 0) return; /* nothing to write */

        /* find out the physical address */
    if (im->nbytes == 0) {
        phyaddr = 0;
    } else {
        phyaddr = phdr.p_vaddr - KERNEL_PADDR + SECTOR_SIZE;
    }

    if (phyaddr < im->nbytes) {
        error("memory conflict: write would backtrack in image\n"
              "\tdesired segment offset: %08zx\n"
              "\t  current image offset: %08zx\n",
              phyaddr, im->nbytes);
    }

    /* write padding before the segment */
    if (im->nbytes < phyaddr) {
        while (im->nbytes < phyaddr) {
            fputc(0, im->img);
            im->nbytes++;
        }
        verbose_printf("\t\tpadding up to 0x%04lx\n", phyaddr);
    }

    /* write the segment itself */
    verbose_printf("\t\twriting 0x%04x bytes\n", phdr.p_memsz);

    fseek(im->fp, phdr.p_offset, SEEK_SET);
    while (phdr.p_filesz-- > 0) {
        fputc(fgetc(im->fp), im->img);
        im->nbytes++;
        phdr.p_memsz--;
    }
    while (phdr.p_memsz-- > 0) {
        fputc(0, im->img);
        im->nbytes++;
    }

    /* write padding after the segment */
    if (im->nbytes % SECTOR_SIZE != 0) {
        while (im->nbytes % SECTOR_SIZE != 0) {
            fputc(0, im->img);
            im->nbytes++;
        }

        verbose_printf("\t\tpadding up to 0x%04lx\n", im->nbytes);
    }
}

static void write_os_size(struct image_t *im)
{
    short os_size;

    /* each image must be padded to be sector-aligned */
    assert((im->nbytes % SECTOR_SIZE) == 0);

    /* -1 to account for the boot block */
    os_size = im->nbytes / SECTOR_SIZE - 1;

    verbose_printf(
            "writing os_size to bootblock: writing %#06x (%d) to pos %#06x\n",
            os_size, os_size, OS_SIZE_LOC
    );

    fseek(im->img, OS_SIZE_LOC, SEEK_SET);
    fwrite(&os_size, sizeof(os_size), 1, im->img);

    /*
     * move the file pointer to the end of file because we will
     * write something else after it
     */
    fseek(im->img, 0, SEEK_END);
}

ATTR_UNUSED
ATTR_PRINTFLIKE(1, 2) static void verbose_printf(const char *fmt, ...)
{
    if (options.extended) {
        va_list args;
        va_start(args, fmt);
        vfprintf(stdout, fmt, args);
        va_end(args);
    }
}

ATTR_PRINTFLIKE(1, 2) static void usage_error(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "%s: ", options.progname);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "usage: %s", options.progname);
    size_t n = sizeof(usage_args) / sizeof(char *);
    for (size_t i = 0; i < n; i++) {
        fprintf(stderr, " %s", usage_args[i]);
    }
    fprintf(stderr, "\n");

    exit(EXIT_FAILURE);
}

/* print an error message and exit */
ATTR_UNUSED
ATTR_PRINTFLIKE(1, 2) static void error(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "%s: ", options.progname);
    vfprintf(stderr, fmt, args);
    va_end(args);

    if (errno != 0) {
        perror(NULL);
    }
    exit(EXIT_FAILURE);
}
