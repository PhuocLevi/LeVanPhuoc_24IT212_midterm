#include "list.h"
#include <dirent.h>

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__APPLE__)
#define STAT_MTIME_NSEC(st) ((st)->st_mtimespec.tv_nsec)
#define STAT_ATIME_NSEC(st) ((st)->st_atimespec.tv_nsec)
#define STAT_CTIME_NSEC(st) ((st)->st_ctimespec.tv_nsec)
#elif defined(st_mtimensec)
#define STAT_MTIME_NSEC(st) ((st)->st_mtimensec)
#define STAT_ATIME_NSEC(st) ((st)->st_atimensec)
#define STAT_CTIME_NSEC(st) ((st)->st_ctimensec)
#else
#define STAT_MTIME_NSEC(st) ((st)->st_mtim.tv_nsec)
#define STAT_ATIME_NSEC(st) ((st)->st_atim.tv_nsec)
#define STAT_CTIME_NSEC(st) ((st)->st_ctim.tv_nsec)
#endif

static bool g_has_printed_anything = false;
static const Options *g_sort_opts = NULL;

static time_t get_sort_time(const struct stat *st, time_type_t ttype) {
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

static long get_sort_timensec(const struct stat *st, time_type_t ttype) {
    switch (ttype) {
        case TIME_STATUS_CHANGE:
            return STAT_CTIME_NSEC(st);
        case TIME_ACCESS:
            return STAT_ATIME_NSEC(st);
        case TIME_MODIFIED:
        default:
            return STAT_MTIME_NSEC(st);
    }
}

static int cmp_fileinfo(const void *p1, const void *p2) {
    const FileInfo *a = (const FileInfo *)p1;
    const FileInfo *b = (const FileInfo *)p2;
    const Options *opts = g_sort_opts;

    int cmp = 0;

    if (opts->sort_type == SORT_SIZE) {
        /* -S: dung lượng lớn nhất xếp trước */
        if (a->st.st_size < b->st.st_size) {
            cmp = 1;
        } else if (a->st.st_size > b->st.st_size) {
            cmp = -1;
        } else {
            cmp = strcmp(a->name, b->name);
        }
    } else if (opts->sort_type == SORT_TIME) {
        /* -t: thời gian mới nhất xếp trước */
        time_t ta = get_sort_time(&a->st, opts->time_type);
        time_t tb = get_sort_time(&b->st, opts->time_type);

        if (ta < tb) {
            cmp = 1;
        } else if (ta > tb) {
            cmp = -1;
        } else {
            long nsa = get_sort_timensec(&a->st, opts->time_type);
            long nsb = get_sort_timensec(&b->st, opts->time_type);
            if (nsa < nsb) {
                cmp = 1;
            } else if (nsa > nsb) {
                cmp = -1;
            } else {
                cmp = strcmp(a->name, b->name);
            }
        }
    } else {
        /* Mặc định: theo bảng chữ cái alphabet */
        cmp = strcmp(a->name, b->name);
    }

    /* -r: đảo ngược kết quả sắp xếp */
    if (opts->opt_r) {
        cmp = -cmp;
    }

    return cmp;
}

void sort_file_infos(FileInfo *entries, size_t count, const Options *opts) {
    if (!entries || count <= 1 || opts->sort_type == SORT_NONE) {
        return;
    }

    g_sort_opts = opts;
    qsort(entries, count, sizeof(FileInfo), cmp_fileinfo);
    g_sort_opts = NULL;
}

void init_file_info(FileInfo *entry, const char *name, const char *full_path, bool follow_symlink) {
    memset(entry, 0, sizeof(*entry));
    entry->name = xstrdup(name);
    entry->path = xstrdup(full_path);

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
            entry->link_target = xstrdup(target);
        }

        /* Khi là toán hạng dòng lệnh không có -d, kiểm tra đích đến có phải thư mục */
        if (follow_symlink) {
            struct stat st_target;
            if (stat(full_path, &st_target) == 0 && S_ISDIR(st_target.st_mode)) {
                entry->is_dir = true;
            }
        }
    }
}

FileInfo *read_dir(const char *dir_path, const Options *opts, size_t *out_count, int *out_err) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        if (out_err) *out_err = errno;
        if (out_count) *out_count = 0;
        return NULL;
    }

    size_t capacity = 64;
    size_t count = 0;
    FileInfo *entries = xmalloc(capacity * sizeof(FileInfo));

    struct dirent *dp;
    errno = 0;
    while ((dp = readdir(dir)) != NULL) {
        const char *name = dp->d_name;

        /* Áp dụng bộ lọc -a và -A */
        if (!opts->opt_a) {
            if (opts->opt_A) {
                if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
                    continue;
                }
            } else {
                if (name[0] == '.') {
                    continue;
                }
            }
        }

        if (count >= capacity) {
            capacity *= 2;
            entries = xrealloc(entries, capacity * sizeof(FileInfo));
        }

        char *full_path = join_path(dir_path, name);
        init_file_info(&entries[count], name, full_path, false);
        free(full_path);
        count++;
    }

    if (errno != 0 && out_err) {
        *out_err = errno;
    } else if (out_err) {
        *out_err = 0;
    }

    closedir(dir);
    *out_count = count;
    return entries;
}

int list_dir(const char *path, const Options *opts, bool print_header, bool is_first) {
    int err = 0;
    size_t count = 0;
    FileInfo *entries = read_dir(path, opts, &count, &err);

    if (err != 0 && !entries) {
        report_error(path, err);
        return 1;
    }

    /* In tiêu đề thư mục */
    if (print_header) {
        if (g_has_printed_anything && !is_first) {
            putchar('\n');
        }
        printf("%s:\n", path);
        g_has_printed_anything = true;
    }

    /* Sắp xếp danh sách */
    sort_file_infos(entries, count, opts);

    /* In danh sách */
    print_entries(entries, count, opts, true);
    if (count > 0 || print_header) {
        g_has_printed_anything = true;
    }

    int exit_status = 0;

    /* Duyệt đệ quy (-R) */
    if (opts->opt_R && !opts->opt_d) {
        for (size_t i = 0; i < count; ++i) {
            FileInfo *e = &entries[i];
            /* Bỏ qua . và .. để tránh đệ quy vô hạn */
            if (strcmp(e->name, ".") == 0 || strcmp(e->name, "..") == 0) {
                continue;
            }

            if (S_ISDIR(e->st.st_mode)) {
                int r = list_dir(e->path, opts, true, false);
                if (r != 0) {
                    exit_status = 1;
                }
            }
        }
    }

    free_file_infos(entries, count);
    return exit_status;
}

int list_main(int argc, char **argv, int optind, const Options *opts) {
    g_has_printed_anything = false;
    int num_operands = argc - optind;

    /* Không có toán hạng: liệt kê thư mục hiện tại */
    if (num_operands <= 0) {
        return list_dir(".", opts, false, true);
    }

    /* 1 toán hạng */
    if (num_operands == 1) {
        const char *arg = argv[optind];
        FileInfo entry;
        bool follow_symlink = !opts->opt_d;
        init_file_info(&entry, arg, arg, follow_symlink);

        if (entry.stat_err != 0) {
            report_error(arg, entry.stat_err);
            free_file_info(&entry);
            return 1;
        }

        if (entry.is_dir && !opts->opt_d) {
            free_file_info(&entry);
            return list_dir(arg, opts, opts->opt_R, true);
        } else {
            print_entries(&entry, 1, opts, false);
            free_file_info(&entry);
            return 0;
        }
    }

    /* Nhiều toán hạng: in file trước, thư mục sau, mỗi nhóm sắp xếp riêng */
    FileInfo *non_dirs = xmalloc(num_operands * sizeof(FileInfo));
    size_t non_dir_count = 0;

    FileInfo *dirs = xmalloc(num_operands * sizeof(FileInfo));
    size_t dir_count = 0;

    int exit_status = 0;

    for (int i = optind; i < argc; ++i) {
        const char *arg = argv[i];
        FileInfo entry;
        bool follow_symlink = !opts->opt_d;
        init_file_info(&entry, arg, arg, follow_symlink);

        if (entry.stat_err != 0) {
            report_error(arg, entry.stat_err);
            exit_status = 1;
            free_file_info(&entry);
            continue;
        }

        if (entry.is_dir && !opts->opt_d) {
            dirs[dir_count++] = entry;
        } else {
            non_dirs[non_dir_count++] = entry;
        }
    }

    /* Sắp xếp và in file thường trước */
    sort_file_infos(non_dirs, non_dir_count, opts);
    if (non_dir_count > 0) {
        print_entries(non_dirs, non_dir_count, opts, false);
        g_has_printed_anything = true;
    }
    free_file_infos(non_dirs, non_dir_count);

    /* Sắp xếp và in thư mục sau */
    sort_file_infos(dirs, dir_count, opts);
    bool multiple_dirs = (dir_count > 1) || (non_dir_count > 0);
    for (size_t i = 0; i < dir_count; ++i) {
        bool print_header = multiple_dirs || opts->opt_R;
        int r = list_dir(dirs[i].path, opts, print_header, (i == 0 && non_dir_count == 0));
        if (r != 0) {
            exit_status = 1;
        }
    }
    free_file_infos(dirs, dir_count);

    return exit_status;
}
