#include "display.h"
#include "util.h"
#include "sort.h"
#include <sys/sysmacros.h>

void format_mode_str(mode_t mode, char *buf) {
    /* 1. File type character */
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

    /* 2. Owner permissions */
    buf[1] = (mode & S_IRUSR) ? 'r' : '-';
    buf[2] = (mode & S_IWUSR) ? 'w' : '-';
    if (mode & S_ISUID) {
        buf[3] = (mode & S_IXUSR) ? 's' : 'S';
    } else {
        buf[3] = (mode & S_IXUSR) ? 'x' : '-';
    }

    /* 3. Group permissions */
    buf[4] = (mode & S_IRGRP) ? 'r' : '-';
    buf[5] = (mode & S_IWGRP) ? 'w' : '-';
    if (mode & S_ISGID) {
        buf[6] = (mode & S_IXGRP) ? 's' : 'S';
    } else {
        buf[6] = (mode & S_IXGRP) ? 'x' : '-';
    }

    /* 4. Other permissions */
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

static col_widths_t compute_column_widths(const file_entry_t *entries, size_t count, const ls_options_t *opts) {
    col_widths_t w;
    memset(&w, 0, sizeof(w));

    char tmp[256];

    for (size_t i = 0; i < count; ++i) {
        const file_entry_t *e = &entries[i];
        if (e->stat_err != 0) continue;

        if (opts->opt_i) {
            int iw = int_width((long long)e->st.st_ino);
            if (iw > w.max_inode_w) w.max_inode_w = iw;
        }

        if (opts->opt_s) {
            int bw = 0;
            if (opts->block_fmt == BLOCK_HUMAN) {
                humanize_size((uint64_t)e->st.st_blocks * 512ULL, tmp, sizeof(tmp));
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
                humanize_size((uint64_t)e->st.st_size, tmp, sizeof(tmp));
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

void print_single_entry(const file_entry_t *entry, const ls_options_t *opts, const col_widths_t *widths) {
    if (entry->stat_err != 0) {
        report_error(entry->name ? entry->name : entry->path, entry->stat_err);
        return;
    }

    col_widths_t local_widths;
    if (!widths && (opts->opt_l || opts->opt_s || opts->opt_i)) {
        local_widths = compute_column_widths(entry, 1, opts);
        widths = &local_widths;
    }

    /* Print Inode number (-i) */
    if (opts->opt_i) {
        if (opts->opt_l && widths) {
            printf("%*llu ", widths->max_inode_w, (unsigned long long)entry->st.st_ino);
        } else {
            printf("%llu ", (unsigned long long)entry->st.st_ino);
        }
    }

    /* Print Block count (-s) */
    if (opts->opt_s) {
        if (opts->block_fmt == BLOCK_HUMAN) {
            char blk_buf[32];
            humanize_size((uint64_t)entry->st.st_blocks * 512ULL, blk_buf, sizeof(blk_buf));
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
        /* File mode */
        char mode_str[12];
        format_mode_str(entry->st.st_mode, mode_str);
        printf("%s ", mode_str);

        /* Number of links */
        printf("%*lu ", widths->max_nlink_w, (unsigned long)entry->st.st_nlink);

        /* Owner name */
        char owner_buf[64];
        get_owner_str(entry->st.st_uid, opts->opt_n, owner_buf, sizeof(owner_buf));
        printf("%-*s  ", widths->max_owner_w, owner_buf);

        /* Group name */
        char group_buf[64];
        get_group_str(entry->st.st_gid, opts->opt_n, group_buf, sizeof(group_buf));
        printf("%-*s  ", widths->max_group_w, group_buf);

        /* Size or Device Major/Minor */
        if (S_ISCHR(entry->st.st_mode) || S_ISBLK(entry->st.st_mode)) {
            printf("%*u, %*u ", widths->max_major_w, major(entry->st.st_rdev),
                                widths->max_minor_w, minor(entry->st.st_rdev));
        } else {
            if (opts->block_fmt == BLOCK_HUMAN) {
                char size_buf[32];
                humanize_size((uint64_t)entry->st.st_size, size_buf, sizeof(size_buf));
                printf("%*s ", widths->max_size_w, size_buf);
            } else {
                printf("%*lld ", widths->max_size_w, (long long)entry->st.st_size);
            }
        }

        /* Timestamp: abbreviated month, day-of-month, hour and minute */
        time_t ftime = get_entry_time(&entry->st, opts->time_type);
        struct tm tm_buf;
        localtime_r(&ftime, &tm_buf);
        char time_str[32];
        strftime(time_str, sizeof(time_str), "%b %e %H:%M", &tm_buf);
        printf("%s ", time_str);
    }

    /* Pathname */
    print_name(entry->name ? entry->name : entry->path, opts);

    /* Indicator character for -F */
    if (opts->opt_F) {
        char ind = get_type_indicator(entry->st.st_mode);
        if (ind != '\0') {
            putchar(ind);
        }
    }

    /* Symlink destination for -l */
    if (opts->opt_l && S_ISLNK(entry->st.st_mode) && entry->link_target) {
        printf(" -> ");
        print_name(entry->link_target, opts);
        if (opts->opt_F) {
            /* Check target indicator if accessible */
            struct stat target_st;
            char *resolved = path_join(entry->path, entry->link_target);
            /* Or stat relative to dir of entry */
            if (stat(entry->path, &target_st) == 0) {
                char target_ind = get_type_indicator(target_st.st_mode);
                if (target_ind != '\0') {
                    putchar(target_ind);
                }
            }
            free(resolved);
        }
    }

    putchar('\n');
}

void print_entries(file_entry_t *entries, size_t count, const ls_options_t *opts, bool is_dir_contents) {
    if (!entries || count == 0) {
        return;
    }

    col_widths_t widths;
    if (opts->opt_l || opts->opt_s || opts->opt_i) {
        widths = compute_column_widths(entries, count, opts);
    }

    /*
     * Directory total line:
     * - In -l, total number of blocks used by files in the directory is output immediately before files.
     * - In -s, if output is to a terminal, total sum is output before listing.
     */
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
            humanize_size(total_bytes, hum_buf, sizeof(hum_buf));
            printf("total %s\n", hum_buf);
        } else {
            printf("total %lld\n", total_blocks);
        }
    }

    for (size_t i = 0; i < count; ++i) {
        print_single_entry(&entries[i], opts, &widths);
    }
}
