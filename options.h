#ifndef OPTIONS_H
#define OPTIONS_H

#include <stdbool.h>

typedef enum {
    TIME_MODIFIED = 0,      /* Mặc định: mtime */
    TIME_STATUS_CHANGE,     /* -c: ctime */
    TIME_ACCESS             /* -u: atime */
} time_type_t;

typedef enum {
    SORT_NAME = 0,          /* Mặc định: alphabet */
    SORT_SIZE,              /* -S: dung lượng giảm dần */
    SORT_TIME,              /* -t: thời gian mới nhất */
    SORT_NONE               /* -f: không sắp xếp */
} sort_type_t;

typedef enum {
    BLOCK_DEFAULT = 0,      /* 512 bytes hoặc BLOCKSIZE env */
    BLOCK_KILO,             /* -k: 1 KiB (1024 bytes) */
    BLOCK_HUMAN             /* -h: định dạng dễ đọc (1.5K, 2M) */
} block_fmt_t;

/**
 * Cấu trúc lưu trữ tất cả 19 tùy chọn dòng lệnh và tham số môi trường.
 */
typedef struct Options {
    bool opt_A;             /* Hiện mục ẩn nhưng bỏ qua . và .. */
    bool opt_a;             /* Hiện tất cả mục, gồm . và .. */
    bool opt_c;             /* Dùng thời gian thay đổi trạng thái tệp (ctime) */
    bool opt_d;             /* Liệt kê chính thư mục, không mở nội dung */
    bool opt_F;             /* Thêm ký hiệu phân loại /, *, @, =, | */
    bool opt_f;             /* Không sắp xếp kết quả (bật -a) */
    bool opt_h;             /* Hiện kích thước dễ đọc (1.5K, 2M) */
    bool opt_i;             /* Hiện số inode */
    bool opt_k;             /* Hiện số block theo đơn vị 1 KiB */
    bool opt_l;             /* Hiện thông tin chi tiết của tệp (long format) */
    bool opt_n;             /* Định dạng dài nhưng UID và GID ở dạng số */
    bool opt_q;             /* Thay ký tự không in được bằng dấu ? */
    bool opt_R;             /* Liệt kê đệ quy các thư mục con */
    bool opt_r;             /* Đảo ngược thứ tự sắp xếp */
    bool opt_S;             /* Sắp xếp theo kích thước giảm dần */
    bool opt_s;             /* Hiện số block thực tế đã sử dụng */
    bool opt_t;             /* Sắp xếp theo thời gian mới nhất trước */
    bool opt_u;             /* Dùng thời gian truy cập gần nhất (atime) */
    bool opt_w;             /* Giữ nguyên ký tự không in được (raw) */

    sort_type_t sort_type;  /* Phương pháp sắp xếp */
    time_type_t time_type;  /* Loại thời gian sử dụng */
    block_fmt_t block_fmt;  /* Định dạng block và size */

    long blocksize;         /* Kích thước block hiệu dụng */
    bool is_stdout_tty;     /* Đầu ra chuẩn là terminal hay pipe/file */
} Options;

/* Khởi tạo các giá trị mặc định */
void options_init(Options *opts);

/* Phân tích tham số dòng lệnh bằng getopt() */
int options_parse(int argc, char **argv, Options *opts, int *optind_out);

#endif /* OPTIONS_H */
