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
        "[--vm]",
        "<bootblock>",  "<kernel>",
        "[process...]",
};

#define SECTOR_SIZE  512
#define OS_SIZE_LOC  2
#define BOOT_MEM_LOC 0x7c00

#define UNUSED(var) ((void) var)

/* to align down to a page boundary, just mask off the last 12 bits */
#define ALIGN_PAGE_DOWN(addr) ((addr) &0xfffff000)

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

/* process directory entry */
struct directory_t {
    int location;
    int size;
};

/* Image state */
static struct image_t {
    FILE *img; /* the file pointer to the image file */
    FILE *fp;  /* the file pointer to the input file */

    size_t nbytes; /* bytes written so far */
    int offset; /* offset of virtual address from physical address */

    size_t pd_loc; /* the location for next process directory entry */
    size_t pd_lim; /* the upper limit for process directory, one sector */
    struct directory_t dir;
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

static void reserve_process_dir(struct image_t *im);
static void add_process_to_dir(struct image_t *im);

static void process_start(struct image_t *im, int vaddr);
static void process_end(struct image_t *im);

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

        } else if (strcmp(argv[i], "--vm") == 0) {
            options.vm = true;

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

    if (options.vm) {
        write_os_size(&image);
        reserve_process_dir(&image);
    }

    for (size_t i = 0; i < options.process_count; i++) {
        add_file(&image, options.processes[i]);
        if (options.vm) {
            add_process_to_dir(&image);
        }
    }

    if (!options.vm) {
        write_os_size(&image);
    }

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

        if (ph == 0) process_start(im, phdr.p_vaddr);

        /* Avoid including segments spanning unreasonably large ranges */
        if ((phdr.p_type != PT_LOAD) || !(phdr.p_flags & PF_X)) {
            verbose_printf("\t\tskipping non-loadable segment\n");
            continue;
        }

        /* write segment to the image */
        write_segment(im, ehdr, phdr);
    }
    process_end(im);

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

static void process_start(struct image_t *im, int vaddr)
{
    /*
     * this function is called once for each process, im->nbytes
     * should be aligned to sector boundary before calling it,
     * so let's check it
     */
    assert((im->nbytes % SECTOR_SIZE) == 0);
    im->dir.location = im->nbytes / SECTOR_SIZE;

    if (im->nbytes == 0) {
        /* this is the first image, it's got to be the boot block */
        /* The virtual address of boot block must be 0 */
        assert(vaddr == 0);
        im->offset = 0;
    } else if (options.vm == 0) {
        /*
         * if there is no vm, put the image according to their
         * virtual address
         */
        im->offset = -KERNEL_PADDR + SECTOR_SIZE;
    } else {
        /* using vm, then just start from the next sector */
        im->offset = im->nbytes - ALIGN_PAGE_DOWN(vaddr);
    }
}

static void process_end(struct image_t *im)
{
    /* write padding after the process */
    if (im->nbytes % SECTOR_SIZE != 0) {
        while (im->nbytes % SECTOR_SIZE != 0) {
            fputc(0, im->img);
            (im->nbytes)++;
        }
        verbose_printf("\t\tpadding up to 0x%04lx\n", im->nbytes);
    }
    /* the size, in sector, of the current process image */
    im->dir.size = im->nbytes / SECTOR_SIZE - im->dir.location;

    verbose_printf(
            "\tProcess starts at sector %d, and spans for %d sectors\n",
            im->dir.location, im->dir.size
    );
}

static void write_segment(struct image_t *im, Elf32_Ehdr ehdr, Elf32_Phdr phdr)
{
    UNUSED(ehdr);

    size_t phyaddr;

    if (phdr.p_memsz == 0) return; /* nothing to write */

        /* find out the physical address */
    phyaddr = phdr.p_vaddr + im->offset;

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

    /*
     * Note: Modified by Han Chen
     * padding here is removed to process_end, this will allow
     * two segments to reside in the same sector
     */
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

static void reserve_process_dir(struct image_t *im)
{
    /* each image must be padded to be sector-aligned */
    assert((im->nbytes % SECTOR_SIZE) == 0);
    assert(options.vm);

    im->pd_loc = im->nbytes;
    im->pd_lim = im->nbytes + SECTOR_SIZE;

    verbose_printf(
            "reserving space for process directory: %#lx to %#lx\n",
            im->pd_loc, im->pd_lim
    );

    /* leave a sector for process directory */
    for (size_t i = 0; i < SECTOR_SIZE; i++) {
        fputc(0, im->img);
        im->nbytes++;
    }
}

static void add_process_to_dir(struct image_t *im)
{
    assert(options.vm);

    if (im->pd_loc + sizeof(struct directory_t) >= im->pd_lim) {
        error("Too many processes! Can't hold them in the directory!\n");
    }

    verbose_printf(
            "\tadding process to directory: "
            "slot %#06lx, img offset %#06x, size %#06x \n",
            im->pd_loc, im->dir.location, im->dir.size
    );

    fseek(im->img, im->pd_loc, SEEK_SET);
    fwrite(&im->dir, sizeof(struct directory_t), 1, im->img);
    /* move the pd_loc to the next entry */
    im->pd_loc = ftell(im->img);

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
