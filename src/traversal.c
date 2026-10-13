#include "traversal.h"
#include "util.h"
#include "sort.h"
#include "display.h"

static bool g_has_printed_anything = false;

void init_file_entry(file_entry_t *entry, const char *name, const char *full_path, bool follow_symlink) {
    memset(entry, 0, sizeof(*entry));
    entry->name = safe_strdup(name);
    entry->path = safe_strdup(full_path);

    struct stat lst;
    if (lstat(full_path, &lst) != 0) {
        entry->stat_err = errno;
        return;
    }

    entry->st = lst;
    entry->stat_err = 0;

    if (S_ISDIR(lst.st_mode)) {
        entry->is_dir = true;
    } else if (S_ISLNK(lst.st_mode)) {
        char target[PATH_MAX];
        ssize_t len = readlink(full_path, target, sizeof(target) - 1);
        if (len != -1) {
            target[len] = '\0';
            entry->link_target = safe_strdup(target);
        }

        /* If follow_symlink is true (command line operand without -d), check if it targets a directory */
        if (follow_symlink) {
            struct stat st_target;
            if (stat(full_path, &st_target) == 0 && S_ISDIR(st_target.st_mode)) {
                entry->is_dir = true;
            }
        }
    }
}

file_entry_t *read_directory(const char *dir_path, const ls_options_t *opts, size_t *out_count, int *out_err) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        if (out_err) *out_err = errno;
        if (out_count) *out_count = 0;
        return NULL;
    }

    size_t capacity = 64;
    size_t count = 0;
    file_entry_t *entries = safe_malloc(capacity * sizeof(file_entry_t));

    struct dirent *dp;
    errno = 0;
    while ((dp = readdir(dir)) != NULL) {
        const char *name = dp->d_name;

        /* Filter entries */
        if (!opts->opt_a) {
            if (opts->opt_A) {
                /* -A: skip '.' and '..' */
                if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
                    continue;
                }
            } else {
                /* Default: skip all starting with '.' */
                if (name[0] == '.') {
                    continue;
                }
            }
        }

        if (count >= capacity) {
            capacity *= 2;
            entries = safe_realloc(entries, capacity * sizeof(file_entry_t));
        }

        char *full_path = path_join(dir_path, name);
        init_file_entry(&entries[count], name, full_path, false);
        free(full_path);
        count++;
    }

    if (errno != 0) {
        if (out_err) *out_err = errno;
    } else {
        if (out_err) *out_err = 0;
    }

    closedir(dir);
    *out_count = count;
    return entries;
}

int list_directory(const char *path, const ls_options_t *opts, bool print_header, bool is_first) {
    int err = 0;
    size_t count = 0;
    file_entry_t *entries = read_directory(path, opts, &count, &err);

    if (err != 0 && !entries) {
        report_error(path, err);
        return 1;
    }

    /* Print header if required */
    if (print_header) {
        if (g_has_printed_anything && !is_first) {
            putchar('\n');
        }
        printf("%s:\n", path);
        g_has_printed_anything = true;
    }

    /* Sort entries */
    sort_entries(entries, count, opts);

    /* Display entries */
    print_entries(entries, count, opts, true);
    if (count > 0 || print_header) {
        g_has_printed_anything = true;
    }

    int exit_status = 0;

    /* Recursive listing (-R) */
    if (opts->opt_R && !opts->opt_d) {
        for (size_t i = 0; i < count; ++i) {
            file_entry_t *e = &entries[i];
            /* Skip '.' and '..' to avoid infinite loops */
            if (strcmp(e->name, ".") == 0 || strcmp(e->name, "..") == 0) {
                continue;
            }

            /* Only recurse into directories (do not follow symlinks to dirs) */
            if (S_ISDIR(e->st.st_mode)) {
                int r = list_directory(e->path, opts, true, false);
                if (r != 0) {
                    exit_status = 1;
                }
            }
        }
    }

    free_file_entries(entries, count);
    return exit_status;
}

int process_operands(int argc, char **argv, int optind, const ls_options_t *opts) {
    g_has_printed_anything = false;
    int num_operands = argc - optind;

    /* If no operands are given, list current directory */
    if (num_operands <= 0) {
        return list_directory(".", opts, false, true);
    }

    /* If 1 operand is given */
    if (num_operands == 1) {
        const char *arg = argv[optind];
        file_entry_t entry;

        /* If -d is set, do not follow symlink */
        bool follow_symlink = !opts->opt_d;
        init_file_entry(&entry, arg, arg, follow_symlink);

        if (entry.stat_err != 0) {
            report_error(arg, entry.stat_err);
            free_file_entry(&entry);
            return 1;
        }

        if (entry.is_dir && !opts->opt_d) {
            free_file_entry(&entry);
            return list_directory(arg, opts, opts->opt_R, true);
        } else {
            print_entries(&entry, 1, opts, false);
            free_file_entry(&entry);
            return 0;
        }
    }

    /*
     * More than one operand given:
     * "non-directory operands are displayed first; directory and non-directory
     * operands are sorted separately and in lexicographical order."
     */
    file_entry_t *non_dirs = safe_malloc(num_operands * sizeof(file_entry_t));
    size_t non_dir_count = 0;

    file_entry_t *dirs = safe_malloc(num_operands * sizeof(file_entry_t));
    size_t dir_count = 0;

    int exit_status = 0;

    for (int i = optind; i < argc; ++i) {
        const char *arg = argv[i];
        file_entry_t entry;
        bool follow_symlink = !opts->opt_d;
        init_file_entry(&entry, arg, arg, follow_symlink);

        if (entry.stat_err != 0) {
            report_error(arg, entry.stat_err);
            exit_status = 1;
            free_file_entry(&entry);
            continue;
        }

        if (entry.is_dir && !opts->opt_d) {
            dirs[dir_count++] = entry;
        } else {
            non_dirs[non_dir_count++] = entry;
        }
    }

    /* Sort non-directory operands */
    sort_entries(non_dirs, non_dir_count, opts);

    /* Display non-directory operands */
    if (non_dir_count > 0) {
        print_entries(non_dirs, non_dir_count, opts, false);
        g_has_printed_anything = true;
    }
    free_file_entries(non_dirs, non_dir_count);

    /* Sort directory operands */
    sort_entries(dirs, dir_count, opts);

    /* Display directory operands */
    bool multiple_dirs = (dir_count > 1) || (non_dir_count > 0);
    for (size_t i = 0; i < dir_count; ++i) {
        bool print_header = multiple_dirs || opts->opt_R;
        int r = list_directory(dirs[i].path, opts, print_header, (i == 0 && non_dir_count == 0));
        if (r != 0) {
            exit_status = 1;
        }
    }
    free_file_entries(dirs, dir_count);

    return exit_status;
}
