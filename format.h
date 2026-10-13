#ifndef FORMAT_H
#define FORMAT_H

#include "util.h"
#include "options.h"
#include <pwd.h>
#include <grp.h>

#ifndef major
#define major(x) ((int)(((uint32_t)(x) & 0x000fff00) >> 8))
#endif
#ifndef minor
#define minor(x) ((int)((((uint32_t)(x) & 0xfff00000) >> 12) | (((uint32_t)(x) & 0x000000ff))))
#endif

/**
 * Cấu trúc chứa thông tin của một entry tệp tin / thư mục
 */
typedef struct FileInfo {
    char *name;             /* Tên tệp (vd: "main.c") */
    char *path;             /* Đường dẫn đầy đủ (vd: "src/main.c") */
    char *link_target;      /* Đích đến nếu là symlink */
    struct stat st;         /* Kết quả lstat() */
    int stat_err;           /* Mã lỗi nếu lstat thất bại (0 nếu thành công) */
    bool is_dir;            /* Là thư mục (hoặc symlink trỏ đến thư mục không có -d) */
} FileInfo;

/**
 * Cấu trúc tính độ rộng cột để căn lề đẹp mắt cho định dạng dài (-l)
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

/* Định dạng chuỗi quyền hạn 10 ký tự (drwxr-xr-x) */
void format_mode(mode_t mode, char *buf);

/* Lấy ký hiệu phân loại cho tùy chọn -F */
char get_type_indicator(mode_t mode);

/* In danh sách các entry */
void print_entries(FileInfo *entries, size_t count, const Options *opts, bool is_dir_contents);

/* In một entry đơn lẻ */
void print_single_entry(const FileInfo *entry, const Options *opts, const col_widths_t *widths);

/* Giải phóng bộ nhớ của FileInfo */
void free_file_info(FileInfo *entry);
void free_file_infos(FileInfo *entries, size_t count);

#endif /* FORMAT_H */
