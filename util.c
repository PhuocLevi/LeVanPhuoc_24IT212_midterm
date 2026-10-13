#include "util.h"
#include "options.h"

void *xmalloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr && size > 0) {
        fprintf(stderr, "ls: out of memory (malloc %zu bytes failed)\n", size);
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void *xrealloc(void *ptr, size_t size) {
    void *new_ptr = realloc(ptr, size);
    if (!new_ptr && size > 0) {
        fprintf(stderr, "ls: out of memory (realloc %zu bytes failed)\n", size);
        exit(EXIT_FAILURE);
    }
    return new_ptr;
}

char *xstrdup(const char *s) {
    if (!s) return NULL;
    char *res = strdup(s);
    if (!res) {
        fprintf(stderr, "ls: out of memory (strdup failed)\n");
        exit(EXIT_FAILURE);
    }
    return res;
}

char *join_path(const char *dir, const char *file) {
    if (!dir || dir[0] == '\0') {
        return xstrdup(file);
    }
    size_t dlen = strlen(dir);
    size_t flen = strlen(file);

    char *res = xmalloc(dlen + flen + 2);
    strcpy(res, dir);
    if (res[dlen - 1] != '/') {
        res[dlen] = '/';
        strcpy(res + dlen + 1, file);
    } else {
        strcpy(res + dlen, file);
    }
    return res;
}

void human_readable(uint64_t bytes, char *buf, size_t buf_len) {
    static const char prefixes[] = "BKMGTPE";
    double val = (double)bytes;
    int scale = 0;

    while (val >= 1024.0 && scale < (int)(sizeof(prefixes) - 2)) {
        val /= 1024.0;
        scale++;
    }

    if (scale == 0) {
        snprintf(buf, buf_len, "%uB", (unsigned int)bytes);
    } else {
        if (val < 10.0) {
            snprintf(buf, buf_len, "%.1f%c", val, prefixes[scale]);
        } else {
            snprintf(buf, buf_len, "%.0f%c", val, prefixes[scale]);
        }
    }
}

long parse_blocksize(const char *env_val) {
    if (!env_val || *env_val == '\0') {
        return 512;
    }

    char *endptr = NULL;
    long val = strtol(env_val, &endptr, 10);
    if (val <= 0) {
        return 512;
    }

    if (endptr && *endptr != '\0') {
        char suffix = *endptr;
        if (suffix == 'k' || suffix == 'K') {
            val *= 1024;
        } else if (suffix == 'm' || suffix == 'M') {
            val *= 1024 * 1024;
        } else if (suffix == 'g' || suffix == 'G') {
            val *= 1024 * 1024 * 1024;
        }
    }

    if (val < 512) {
        val = 512;
    }
    return val;
}

long long calc_blocks(const struct stat *st, const Options *opts) {
    /* st_blocks luôn ở đơn vị 512 bytes trên POSIX/BSD */
    long long bytes = (long long)st->st_blocks * 512LL;
    long bs = 512;

    if (opts->block_fmt == BLOCK_KILO) {
        bs = 1024;
    } else if (opts->block_fmt == BLOCK_DEFAULT) {
        bs = opts->blocksize > 0 ? opts->blocksize : 512;
    } else {
        bs = 512;
    }

    return (bytes + bs - 1) / bs;
}

void print_escaped_name(const char *name, const Options *opts) {
    if (!name) return;

    if (opts->opt_q) {
        for (const unsigned char *p = (const unsigned char *)name; *p != '\0'; ++p) {
            if (isprint(*p)) {
                putchar(*p);
            } else {
                putchar('?');
            }
        }
    } else {
        fputs(name, stdout);
    }
}

void report_error(const char *path, int errnum) {
    fprintf(stderr, "ls: %s: %s\n", path ? path : "", strerror(errnum));
}
