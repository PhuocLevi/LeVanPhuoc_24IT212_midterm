#include "ls.h"
#include "util.h"
#include "traversal.h"

static void init_default_options(ls_options_t *opts) {
    memset(opts, 0, sizeof(*opts));

    opts->is_stdout_tty = (isatty(STDOUT_FILENO) != 0);

    /*
     * Default for non-printable characters:
     * -q is default when output is to a terminal;
     * -w is default when output is not to a terminal.
     */
    if (opts->is_stdout_tty) {
        opts->opt_q = true;
        opts->opt_w = false;
    } else {
        opts->opt_q = false;
        opts->opt_w = true;
    }

    opts->sort_type = SORT_NAME;
    opts->time_type = TIME_MODIFIED;
    opts->block_fmt = BLOCK_DEFAULT;

    /* Parse BLOCKSIZE environment variable */
    const char *bs_env = getenv("BLOCKSIZE");
    opts->blocksize = parse_blocksize(bs_env);

    /*
     * "-A List all entries except for '.' and '..'. Always set for the super-user."
     */
    if (getuid() == 0) {
        opts->opt_A = true;
    }
}

static void print_usage(const char *progname) {
    fprintf(stderr, "usage: %s [-AacdFfhiklnqRrSstuw] [file ...]\n", progname ? progname : "ls");
}

int main(int argc, char **argv) {
    ls_options_t opts;
    init_default_options(&opts);

    int opt;
    while ((opt = getopt(argc, argv, "AacdFfhiklnqRrSstuw")) != -1) {
        switch (opt) {
            case 'A':
                opts.opt_A = true;
                opts.opt_a = false;
                break;
            case 'a':
                opts.opt_a = true;
                break;
            case 'c':
                /* -c and -u override each other */
                opts.time_type = TIME_STATUS_CHANGE;
                break;
            case 'u':
                /* -c and -u override each other */
                opts.time_type = TIME_ACCESS;
                break;
            case 'd':
                /* -R and -d override each other */
                opts.opt_d = true;
                opts.opt_R = false;
                break;
            case 'R':
                /* -R and -d override each other */
                opts.opt_R = true;
                opts.opt_d = false;
                break;
            case 'F':
                opts.opt_F = true;
                break;
            case 'f':
                /* -f: Output is not sorted. In BSD, also enables -a */
                opts.sort_type = SORT_NONE;
                opts.opt_a = true;
                break;
            case 'h':
                /* -h overrides -k */
                opts.block_fmt = BLOCK_HUMAN;
                break;
            case 'k':
                /* -k overrides -h */
                opts.block_fmt = BLOCK_KILO;
                break;
            case 'i':
                opts.opt_i = true;
                break;
            case 'l':
                /* -l and -n override each other */
                opts.opt_l = true;
                opts.opt_n = false;
                break;
            case 'n':
                /* -l and -n override each other */
                opts.opt_l = true;
                opts.opt_n = true;
                break;
            case 'q':
                /* -w and -q override each other */
                opts.opt_q = true;
                opts.opt_w = false;
                break;
            case 'w':
                /* -w and -q override each other */
                opts.opt_w = true;
                opts.opt_q = false;
                break;
            case 'r':
                opts.opt_r = true;
                break;
            case 'S':
                /* -S and -t override each other */
                opts.sort_type = SORT_SIZE;
                break;
            case 't':
                /* -S and -t override each other */
                opts.sort_type = SORT_TIME;
                break;
            case 's':
                opts.opt_s = true;
                break;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    int status = process_operands(argc, argv, optind, &opts);
    return (status == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
