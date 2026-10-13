#ifndef UTIL_H
#define UTIL_H

#include "ls.h"

/* Safe memory allocation helpers */
void *safe_malloc(size_t size);
void *safe_realloc(void *ptr, size_t size);
char *safe_strdup(const char *s);

/* Path concatenation helper */
char *path_join(const char *dir, const char *file);

/* File entry cleanup */
void free_file_entry(file_entry_t *entry);
void free_file_entries(file_entry_t *entries, size_t count);

/* Block size parsing and calculation */
long parse_blocksize(const char *env_val);
long long calc_blocks(const struct stat *st, const ls_options_t *opts);

/* Human-readable number formatting */
void humanize_size(uint64_t bytes, char *buf, size_t buf_len);

/* Print filename with proper character escaping (-q / -w) */
void print_name(const char *name, const ls_options_t *opts);

/* Report error to stderr with program name prefix */
void report_error(const char *path, int errnum);

#endif /* UTIL_H */
