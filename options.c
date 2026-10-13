#include "options.h"
#include "util.h"

void options_init(Options *opts) {
    memset(opts, 0, sizeof(*opts));

    opts->is_stdout_tty = (isatty(STDOUT_FILENO) != 0);

    /* Mặc định: -q khi xuất ra terminal, -w khi xuất qua pipe */
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

    /* Đọc biến môi trường BLOCKSIZE */
    const char *bs_env = getenv("BLOCKSIZE");
    opts->blocksize = parse_blocksize(bs_env);

    /* -A luôn bật mặc định cho super-user (uid == 0) */
    if (getuid() == 0) {
        opts->opt_A = true;
    }
}

int options_parse(int argc, char **argv, Options *opts, int *optind_out) {
    options_init(opts);

    int opt;
    while ((opt = getopt(argc, argv, "AacdFfhiklnqRrSstuw")) != -1) {
        switch (opt) {
            case 'A':
                opts->opt_A = true;
                opts->opt_a = false;
                break;
            case 'a':
                opts->opt_a = true;
                break;
            case 'c':
                /* -c và -u ghi đè lẫn nhau */
                opts->time_type = TIME_STATUS_CHANGE;
                break;
            case 'u':
                /* -c và -u ghi đè lẫn nhau */
                opts->time_type = TIME_ACCESS;
                break;
            case 'd':
                /* -R và -d ghi đè lẫn nhau */
                opts->opt_d = true;
                opts->opt_R = false;
                break;
            case 'R':
                /* -R và -d ghi đè lẫn nhau */
                opts->opt_R = true;
                opts->opt_d = false;
                break;
            case 'F':
                opts->opt_F = true;
                break;
            case 'f':
                /* -f: không sắp xếp kết quả, tự động bật -a */
                opts->sort_type = SORT_NONE;
                opts->opt_a = true;
                break;
            case 'h':
                /* -h ghi đè -k */
                opts->block_fmt = BLOCK_HUMAN;
                break;
            case 'k':
                /* -k ghi đè -h */
                opts->block_fmt = BLOCK_KILO;
                break;
            case 'i':
                opts->opt_i = true;
                break;
            case 'l':
                /* -l và -n ghi đè lẫn nhau */
                opts->opt_l = true;
                opts->opt_n = false;
                break;
            case 'n':
                /* -l và -n ghi đè lẫn nhau */
                opts->opt_l = true;
                opts->opt_n = true;
                break;
            case 'q':
                /* -w và -q ghi đè lẫn nhau */
                opts->opt_q = true;
                opts->opt_w = false;
                break;
            case 'w':
                /* -w và -q ghi đè lẫn nhau */
                opts->opt_w = true;
                opts->opt_q = false;
                break;
            case 'r':
                opts->opt_r = true;
                break;
            case 'S':
                /* -S và -t ghi đè lẫn nhau */
                opts->sort_type = SORT_SIZE;
                break;
            case 't':
                /* -S và -t ghi đè lẫn nhau */
                opts->sort_type = SORT_TIME;
                break;
            case 's':
                opts->opt_s = true;
                break;
            default:
                fprintf(stderr, "usage: %s [-AacdFfhiklnqRrSstuw] [file ...]\n", argv[0]);
                return -1;
        }
    }

    if (optind_out) {
        *optind_out = optind;
    }
    return 0;
}
