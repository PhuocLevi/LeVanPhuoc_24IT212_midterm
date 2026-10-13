#ifndef DISPLAY_H
#define DISPLAY_H

#include "ls.h"

/**
 * Structure holding maximum column widths for formatted long listing (-l).
 */
typedef struct {
    int max_inode_w;
    int max_block_w;
    int max_nlink_w;
    int max_owner_w;
    int max_group_w;
    int max_size_w;
    int max_major_w;
    int max_minor_w;
    bool has_dev;
} col_widths_t;

/**
 * Format the file mode into a 10-character string (e.g., "drwxr-xr-x").
 */
void format_mode_str(mode_t mode, char *buf);

/**
 * Get indicator character for -F flag ('/', '*', '@', '=', '|', '%', or '\0').
 */
char get_type_indicator(mode_t mode);

/**
 * Print a list of file entries (either operands or directory contents).
 * Handles -i, -s, -l, -n, -F, -q, -w, and calculates column alignments.
 */
void print_entries(file_entry_t *entries, size_t count, const ls_options_t *opts, bool is_dir_contents);

/**
 * Print a single file entry according to options and column widths.
 */
void print_single_entry(const file_entry_t *entry, const ls_options_t *opts, const col_widths_t *widths);

#endif /* DISPLAY_H */
