#ifndef LIST_H
#define LIST_H

#include "format.h"

/* Điểm vào xử lý danh sách toán hạng */
int list_main(int argc, char **argv, int optind, const Options *opts);

/* Duyệt và hiển thị nội dung một thư mục */
int list_dir(const char *path, const Options *opts, bool print_header, bool is_first);

/* Đọc các entry trong thư mục có áp dụng bộ lọc -a / -A */
FileInfo *read_dir(const char *dir_path, const Options *opts, size_t *out_count, int *out_err);

/* Khởi tạo cấu trúc FileInfo */
void init_file_info(FileInfo *entry, const char *name, const char *full_path, bool follow_symlink);

/* Sắp xếp danh sách FileInfo theo -S, -t, alphabet hoặc -r */
void sort_file_infos(FileInfo *entries, size_t count, const Options *opts);

#endif /* LIST_H */
