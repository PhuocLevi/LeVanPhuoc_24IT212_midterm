#ifndef TRAVERSAL_H
#define TRAVERSAL_H

#include "ls.h"

/**
 * Process command-line file operands:
 * - Separates non-directory and directory operands.
 * - Sorts them separately.
 * - Displays non-directory operands first.
 * - Displays directory operands subsequently.
 * - Handles recursion (-R) if specified.
 * Returns 0 on success, or > 0 if errors occurred.
 */
int process_operands(int argc, char **argv, int optind, const ls_options_t *opts);

/**
 * List contents of a single directory.
 * If print_header is true, prints "<dir_path>:\n".
 * Recursively traverses subdirectories if opts->opt_R is true.
 * Returns 0 on success, or 1 on error.
 */
int list_directory(const char *path, const ls_options_t *opts, bool print_header, bool is_first);

/**
 * Read entries of a directory and filter based on -a and -A options.
 * Populates out_count with number of valid entries.
 * Returns allocated array of file_entry_t.
 */
file_entry_t *read_directory(const char *dir_path, const ls_options_t *opts, size_t *out_count, int *out_err);

/**
 * Inspect a file path using lstat() (or stat() if follow_symlink is true)
 * and populate a file_entry_t structure.
 */
void init_file_entry(file_entry_t *entry, const char *name, const char *full_path, bool follow_symlink);

#endif /* TRAVERSAL_H */
