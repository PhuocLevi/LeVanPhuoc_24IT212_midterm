#ifndef LS_H
#define LS_H

#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _XOPEN_SOURCE 700

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>

/**
 * File timestamp type used for sorting (-t) or printing (-l).
 * - TIME_MODIFIED: Last modification time (default, st_mtime)
 * - TIME_STATUS_CHANGE: Last status change time (-c, st_ctime)
 * - TIME_ACCESS: Last access time (-u, st_atime)
 */
typedef enum {
    TIME_MODIFIED = 0,
    TIME_STATUS_CHANGE,
    TIME_ACCESS
} time_type_t;

/**
 * Sorting method.
 * - SORT_NAME: Lexicographical order by filename (default)
 * - SORT_SIZE: Sort by size, largest first (-S)
 * - SORT_TIME: Sort by time, newest first (-t)
 * - SORT_NONE: Unsorted directory order (-f)
 */
typedef enum {
    SORT_NAME = 0,
    SORT_SIZE,
    SORT_TIME,
    SORT_NONE
} sort_type_t;

/**
 * Block size display format for -s and -l options.
 * - BLOCK_DEFAULT: Units of 512 bytes or BLOCKSIZE environment variable
 * - BLOCK_KILO: Units of 1024 bytes (-k)
 * - BLOCK_HUMAN: Human-readable bytes / KiB / MiB (-h)
 */
typedef enum {
    BLOCK_DEFAULT = 0,
    BLOCK_KILO,
    BLOCK_HUMAN
} block_format_t;

/**
 * Structure holding all parsed CLI flags and configuration.
 */
typedef struct {
    bool opt_A;          /* -A: list all entries except '.' and '..' */
    bool opt_a;          /* -a: include entries beginning with '.' */
    bool opt_d;          /* -d: directories listed as plain files */
    bool opt_F;          /* -F: append type indicators (/ * @ % = |) */
    bool opt_i;          /* -i: print inode number */
    bool opt_l;          /* -l: list in long format */
    bool opt_n;          /* -n: numeric UID and GID in long format */
    bool opt_q;          /* -q: force '?' for non-printable characters */
    bool opt_w;          /* -w: force raw printing of non-printable chars */
    bool opt_R;          /* -R: recursively list subdirectories */
    bool opt_r;          /* -r: reverse sort order */
    bool opt_s;          /* -s: display number of blocks used */

    sort_type_t sort_type;       /* sorting strategy (-f, -S, -t, or name) */
    time_type_t time_type;       /* timestamp selection (default, -c, or -u) */
    block_format_t block_fmt;    /* block display format (default, -k, or -h) */

    long blocksize;      /* block size in bytes (512 or from BLOCKSIZE) */
    bool is_stdout_tty;  /* true if stdout is connected to a terminal */
} ls_options_t;

/**
 * Structure representing an inspected file or directory entry.
 */
typedef struct {
    char *name;          /* Base filename (e.g. "main.c") */
    char *path;          /* Path used for stat/display (e.g. "src/main.c") */
    char *link_target;   /* Target path if symbolic link, or NULL */
    struct stat st;      /* Result of lstat() */
    int stat_err;        /* Error number if stat failed, 0 on success */
    bool is_dir;         /* Whether this entry is treated as a directory */
} file_entry_t;

#endif /* LS_H */
