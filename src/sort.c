#include "sort.h"

static const ls_options_t *g_current_sort_opts = NULL;

time_t get_entry_time(const struct stat *st, time_type_t ttype) {
    if (!st) return 0;
    switch (ttype) {
        case TIME_STATUS_CHANGE:
            return st->st_ctime;
        case TIME_ACCESS:
            return st->st_atime;
        case TIME_MODIFIED:
        default:
            return st->st_mtime;
    }
}

long get_entry_timensec(const struct stat *st, time_type_t ttype) {
    if (!st) return 0;
#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200809L
    switch (ttype) {
        case TIME_STATUS_CHANGE:
            return st->st_ctim.tv_nsec;
        case TIME_ACCESS:
            return st->st_atim.tv_nsec;
        case TIME_MODIFIED:
        default:
            return st->st_mtim.tv_nsec;
    }
#else
    return 0;
#endif
}

static int compare_entries_internal(const void *p1, const void *p2) {
    const file_entry_t *a = (const file_entry_t *)p1;
    const file_entry_t *b = (const file_entry_t *)p2;
    const ls_options_t *opts = g_current_sort_opts;

    int cmp = 0;

    if (opts->sort_type == SORT_SIZE) {
        /* -S: largest file first */
        if (a->st.st_size < b->st.st_size) {
            cmp = 1;
        } else if (a->st.st_size > b->st.st_size) {
            cmp = -1;
        } else {
            /* Tie-breaker: lexicographical order */
            cmp = strcmp(a->name, b->name);
        }
    } else if (opts->sort_type == SORT_TIME) {
        /* -t: most recently modified first */
        time_t ta = get_entry_time(&a->st, opts->time_type);
        time_t tb = get_entry_time(&b->st, opts->time_type);

        if (ta < tb) {
            cmp = 1;
        } else if (ta > tb) {
            cmp = -1;
        } else {
            long nsa = get_entry_timensec(&a->st, opts->time_type);
            long nsb = get_entry_timensec(&b->st, opts->time_type);
            if (nsa < nsb) {
                cmp = 1;
            } else if (nsa > nsb) {
                cmp = -1;
            } else {
                /* Tie-breaker: lexicographical order */
                cmp = strcmp(a->name, b->name);
            }
        }
    } else {
        /* Default: lexicographical order */
        cmp = strcmp(a->name, b->name);
    }

    /* -r: reverse order */
    if (opts->opt_r) {
        cmp = -cmp;
    }

    return cmp;
}

void sort_entries(file_entry_t *entries, size_t count, const ls_options_t *opts) {
    if (!entries || count <= 1) {
        return;
    }

    /* -f: output is not sorted */
    if (opts->sort_type == SORT_NONE) {
        return;
    }

    g_current_sort_opts = opts;
    qsort(entries, count, sizeof(file_entry_t), compare_entries_internal);
    g_current_sort_opts = NULL;
}
