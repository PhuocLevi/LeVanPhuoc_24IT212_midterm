#include "options.h"
#include "list.h"

int main(int argc, char **argv) {
    Options opts;
    int optind_parsed = 0;

    /* Bước 1: Phân tích tham số dòng lệnh */
    if (options_parse(argc, argv, &opts, &optind_parsed) != 0) {
        return EXIT_FAILURE;
    }

    /* Bước 2-6: Xử lý và duyệt danh sách đường dẫn đầu vào */
    int status = list_main(argc, argv, optind_parsed, &opts);

    return (status == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
