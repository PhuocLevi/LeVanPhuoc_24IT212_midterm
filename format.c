#include "format.h"

void free_file_info(FileInfo *entry) {
    if (!entry) return;
    if (entry->name) {
        free(entry->name);
        entry->name = NULL;
    }
    if (entry->path) {
        free(entry->path);
        entry->path = NULL;
    }
    if (entry->link_target) {
        free(entry->link_target);
        entry->link_target = NULL;
    }
}

void free_file_infos(FileInfo *entries, size_t count) {
    if (!entries) return;
    for (size_t i = 0; i < count; ++i) {
        free_file_info(&entries[i]);
    }
    free(entries);
}

void format_mode(mode_t mode, char *buf) {
    /* 1. Ký tự phân loại loại tệp */
    if (S_ISDIR(mode)) {
        buf[0] = 'd';
    } else if (S_ISLNK(mode)) {
        buf[0] = 'l';
    } else if (S_ISCHR(mode)) {
        buf[0] = 'c';
    } else if (S_ISBLK(mode)) {
        buf[0] = 'b';
    } else if (S_ISSOCK(mode)) {
        buf[0] = 's';
    } else if (S_ISFIFO(mode)) {
        buf[0] = 'p';
#ifdef S_ISWHT
    } else if (S_ISWHT(mode)) {
        buf[0] = 'w';
#endif
    } else {
        buf[0] = '-';
    }

    /* 2. Quyền của chủ sở hữu (Owner) */
    buf[1] = (mode & S_IRUSR) ? 'r' : '-';
    buf[2] = (mode & S_IWUSR) ? 'w' : '-';
    if (mode & S_ISUID) {
        buf[3] = (mode & S_IXUSR) ? 's' : 'S';
    } else {
        buf[3] = (mode & S_IXUSR) ? 'x' : '-';
    }

    /* 3. Quyền của nhóm (Group) */
    buf[4] = (mode & S_IRGRP) ? 'r' : '-';
    buf[5] = (mode & S_IWGRP) ? 'w' : '-';
    if (mode & S_ISGID) {
        buf[6] = (mode & S_IXGRP) ? 's' : 'S';
    } else {
        buf[6] = (mode & S_IXGRP) ? 'x' : '-';
    }

    /* 4. Quyền của người khác (Others) */
    buf[7] = (mode & S_IROTH) ? 'r' : '-';
    buf[8] = (mode & S_IWOTH) ? 'w' : '-';
    if (mode & S_ISVTX) {
        buf[9] = (mode & S_IXOTH) ? 't' : 'T';
    } else {
        buf[9] = (mode & S_IXOTH) ? 'x' : '-';
    }

    buf[10] = '\0';
}

char get_type_indicator(mode_t mode) {
    if (S_ISDIR(mode)) {
        return '/';
    }
    if (S_ISLNK(mode)) {
        return '@';
    }
    if (S_ISSOCK(mode)) {
        return '=';
    }
    if (S_ISFIFO(mode)) {
        return '|';
    }
#ifdef S_ISWHT
    if (S_ISWHT(mode)) {
        return '%';
    }
#endif
    if (S_ISREG(mode) && (mode & (S_IXUSR | S_IXGRP | S_IXOTH))) {
        return '*';
    }
    return '\0';
}

static int int_width(long long val) {
    char buf[64];
    return snprintf(buf, sizeof(buf), "%lld", val);
}

static void get_owner_str(uid_t uid, bool opt_n, char *buf, size_t len) {
    if (!opt_n) {
        struct passwd *pw = getpwuid(uid);
        if (pw && pw->pw_name) {
            snprintf(buf, len, "%s", pw->pw_name);
            return;
        }
    }
    snprintf(buf, len, "%u", (unsigned int)uid);
}

static void get_group_str(gid_t gid, bool opt_n, char *buf, size_t len) {
    if (!opt_n) {
        struct group *gr = getgrgid(gid);
        if (gr && gr->gr_name) {
            snprintf(buf, len, "%s", gr->gr_name);
            return;
        }
    }
    snprintf(buf, len, "%u", (unsigned int)gid);
}

static time_t get_time_field(const struct stat *st, time_type_t ttype) {
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

static col_widths_t compute_column_widths(const FileInfo *entries, size_t count, const Options *opts) {
    col_widths_t w;
    memset(&w, 0, sizeof(w));

    char tmp[256];

    for (size_t i = 0; i < count; ++i) {
        const FileInfo *e = &entries[i];
        if (e->stat_err != 0) continue;

        if (opts->opt_i) {
            int iw = int_width((long long)e->st.st_ino);
            if (iw > w.max_inode_w) w.max_inode_w = iw;
        }

        if (opts->opt_s) {
            int bw = 0;
            if (opts->block_fmt == BLOCK_HUMAN) {
                human_readable((uint64_t)e->st.st_blocks * 512ULL, tmp, sizeof(tmp));
                bw = (int)strlen(tmp);
            } else {
                long long blks = calc_blocks(&e->st, opts);
                bw = int_width(blks);
            }
            if (bw > w.max_block_w) w.max_block_w = bw;
        }

        int lw = int_width((long long)e->st.st_nlink);
        if (lw > w.max_nlink_w) w.max_nlink_w = lw;

        get_owner_str(e->st.st_uid, opts->opt_n, tmp, sizeof(tmp));
        int ow = (int)strlen(tmp);
        if (ow > w.max_owner_w) w.max_owner_w = ow;

        get_group_str(e->st.st_gid, opts->opt_n, tmp, sizeof(tmp));
        int gw = (int)strlen(tmp);
        if (gw > w.max_group_w) w.max_group_w = gw;

        if (S_ISCHR(e->st.st_mode) || S_ISBLK(e->st.st_mode)) {
            w.has_dev = true;
            int maj_w = int_width(major(e->st.st_rdev));
            int min_w = int_width(minor(e->st.st_rdev));
            if (maj_w > w.max_major_w) w.max_major_w = maj_w;
            if (min_w > w.max_minor_w) w.max_minor_w = min_w;
        } else {
            int sw = 0;
            if (opts->block_fmt == BLOCK_HUMAN) {
                human_readable((uint64_t)e->st.st_size, tmp, sizeof(tmp));
                sw = (int)strlen(tmp);
            } else {
                sw = int_width((long long)e->st.st_size);
            }
            if (sw > w.max_size_w) w.max_size_w = sw;
        }
    }

    if (w.has_dev) {
        int dev_w = w.max_major_w + w.max_minor_w + 2; /* "maj, min" */
        if (dev_w > w.max_size_w) {
            w.max_size_w = dev_w;
        }
    }

    return w;
}

void print_single_entry(const FileInfo *entry, const Options *opts, const col_widths_t *widths) {
    if (entry->stat_err != 0) {
        report_error(entry->name ? entry->name : entry->path, entry->stat_err);
        return;
    }

    col_widths_t local_widths;
    if (!widths && (opts->opt_l || opts->opt_s || opts->opt_i)) {
        local_widths = compute_column_widths(entry, 1, opts);
        widths = &local_widths;
    }

    /* 1. In số Inode (-i) */
    if (opts->opt_i) {
        if (opts->opt_l && widths) {
            printf("%*llu ", widths->max_inode_w, (unsigned long long)entry->st.st_ino);
        } else {
            printf("%llu ", (unsigned long long)entry->st.st_ino);
        }
    }

    /* 2. In số Block (-s) */
    if (opts->opt_s) {
        if (opts->block_fmt == BLOCK_HUMAN) {
            char blk_buf[32];
            human_readable((uint64_t)entry->st.st_blocks * 512ULL, blk_buf, sizeof(blk_buf));
            if (opts->opt_l && widths) {
                printf("%*s ", widths->max_block_w, blk_buf);
            } else {
                printf("%s ", blk_buf);
            }
        } else {
            long long blks = calc_blocks(&entry->st, opts);
            if (opts->opt_l && widths) {
                printf("%*lld ", widths->max_block_w, blks);
            } else {
                printf("%lld ", blks);
            }
        }
    }

    if (opts->opt_l) {
        /* File mode 10 ký tự */
        char mode_str[12];
        format_mode(entry->st.st_mode, mode_str);
        printf("%s ", mode_str);

        /* Số liên kết cứng (hard links) */
        printf("%*lu ", widths->max_nlink_w, (unsigned long)entry->st.st_nlink);

        /* Tên hoặc ID chủ sở hữu */
        char owner_buf[64];
        get_owner_str(entry->st.st_uid, opts->opt_n, owner_buf, sizeof(owner_buf));
        printf("%-*s  ", widths->max_owner_w, owner_buf);

        /* Tên hoặc ID nhóm */
        char group_buf[64];
        get_group_str(entry->st.st_gid, opts->opt_n, group_buf, sizeof(group_buf));
        printf("%-*s  ", widths->max_group_w, group_buf);

        /* Kích thước hoặc thiết bị đặc biệt Major/Minor */
        if (S_ISCHR(entry->st.st_mode) || S_ISBLK(entry->st.st_mode)) {
            printf("%*u, %*u ", widths->max_major_w, major(entry->st.st_rdev),
                                widths->max_minor_w, minor(entry->st.st_rdev));
        } else {
            if (opts->block_fmt == BLOCK_HUMAN) {
                char size_buf[32];
                human_readable((uint64_t)entry->st.st_size, size_buf, sizeof(size_buf));
                printf("%*s ", widths->max_size_w, size_buf);
            } else {
                printf("%*lld ", widths->max_size_w, (long long)entry->st.st_size);
            }
        }

        /* Thời gian: tháng viết tắt, ngày trong tháng, giờ và phút */
        time_t ftime = get_time_field(&entry->st, opts->time_type);
        struct tm tm_buf;
        localtime_r(&ftime, &tm_buf);
        char time_str[32];
        strftime(time_str, sizeof(time_str), "%b %e %H:%M", &tm_buf);
        printf("%s ", time_str);
    }

    /* Tên file */
    print_escaped_name(entry->name ? entry->name : entry->path, opts);

    /* Ký hiệu phân loại cho -F */
    if (opts->opt_F) {
        char ind = get_type_indicator(entry->st.st_mode);
        if (ind != '\0') {
            putchar(ind);
        }
    }

    /* Đích đến của liên kết tượng trưng (-l) */
    if (opts->opt_l && S_ISLNK(entry->st.st_mode) && entry->link_target) {
        printf(" -> ");
        print_escaped_name(entry->link_target, opts);
        if (opts->opt_F) {
            struct stat target_st;
            if (stat(entry->path, &target_st) == 0) {
                char target_ind = get_type_indicator(target_st.st_mode);
                if (target_ind != '\0') {
                    putchar(target_ind);
                }
            }
        }
    }

    putchar('\n');
}

void print_entries(FileInfo *entries, size_t count, const Options *opts, bool is_dir_contents) {
    if (!entries || count == 0) {
        return;
    }

    col_widths_t widths;
    if (opts->opt_l || opts->opt_s || opts->opt_i) {
        widths = compute_column_widths(entries, count, opts);
    }

    /* In dòng total khi dùng -l cho thư mục, hoặc -s khi xuất ra terminal */
    bool should_print_total = false;
    if (is_dir_contents && opts->opt_l) {
        should_print_total = true;
    } else if (opts->opt_s && opts->is_stdout_tty) {
        should_print_total = true;
    }

    if (should_print_total) {
        long long total_blocks = 0;
        uint64_t total_bytes = 0;

        for (size_t i = 0; i < count; ++i) {
            if (entries[i].stat_err == 0) {
                total_blocks += calc_blocks(&entries[i].st, opts);
                total_bytes += (uint64_t)entries[i].st.st_blocks * 512ULL;
            }
        }

        if (opts->block_fmt == BLOCK_HUMAN) {
            char hum_buf[32];
            human_readable(total_bytes, hum_buf, sizeof(hum_buf));
            printf("total %s\n", hum_buf);
        } else {
            printf("total %lld\n", total_blocks);
        }
    }

    for (size_t i = 0; i < count; ++i) {
        print_single_entry(&entries[i], opts, &widths);
    }
}
