#ifndef SORT_H
#define SORT_H

#include "ls.h"

/**
 * Sort an array of file_entry_t according to the configured options:
 * - -f: no sorting
 * - -S: sort by size descending
 * - -t: sort by time (mtime, ctime, or atime) descending
 * - default: lexicographical order by filename
 * - -r: reverses the final order
 */
void sort_entries(file_entry_t *entries, size_t count, const ls_options_t *opts);

/**
 * Helper to extract the selected timestamp from stat structure.
 */
time_t get_entry_time(const struct stat *st, time_type_t ttype);
long get_entry_timensec(const struct stat *st, time_type_t ttype);

#endif /* SORT_H */
