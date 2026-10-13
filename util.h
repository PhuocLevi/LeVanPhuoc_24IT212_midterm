#ifndef UTIL_H
#define UTIL_H

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>

#if defined(__linux__)
#include <sys/sysmacros.h>
#endif

/* Forward declaration */
struct Options;

/* An toàn cấp phát bộ nhớ */
void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t size);
char *xstrdup(const char *s);

/* Nối đường dẫn thư mục và file */
char *join_path(const char *dir, const char *file);

/* Định dạng kích thước dễ đọc (-h) */
void human_readable(uint64_t bytes, char *buf, size_t buf_len);

/* Xử lý biến môi trường BLOCKSIZE và tính số block */
long parse_blocksize(const char *env_val);
long long calc_blocks(const struct stat *st, const struct Options *opts);

/* In tên file với hỗ trợ -q / -w */
void print_escaped_name(const char *name, const struct Options *opts);

/* Báo lỗi ra stderr */
void report_error(const char *path, int errnum);

#endif /* UTIL_H */
