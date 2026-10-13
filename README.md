# TRƯỜNG ĐẠI HỌC CÔNG NGHỆ THÔNG TIN VÀ TRUYỀN THÔNG VIỆT HÀN
## KHOA KHOA HỌC MÁY TÍNH
***

# BÁO CÁO ĐỒ ÁN GIỮA KÌ 1
### ĐỀ TÀI: XÂY DỰNG PHIÊN BẢN ĐƠN GIẢN CỦA LỆNH LS(1)

* **Sinh viên thực hiện:** Lê Văn Phước — **MSSV:** 24IT212
* **Lớp:** 24IT2
* **Giảng viên hướng dẫn:** TS. Nguyễn Nhật Ân
* **Đà Nẵng, tháng 10 năm 2026**

***

## 1. Giới thiệu

`ls` là một trong những lệnh cơ bản và được sử dụng thường xuyên nhất trên hệ điều hành UNIX/Linux, dùng để liệt kê nội dung thư mục. Mặc dù giao diện đơn giản đối với người dùng, nhưng để cài đặt được nó cần vận dụng nhiều kiến thức cốt lõi và quan trọng của lập trình hệ thống như:
- Quản lý tệp tin và thư mục ở mức hệ thống (`stat`, `lstat`, `opendir`, `readdir`, `closedir`, `readlink`).
- Phân tích thông tin người dùng và nhóm quyền (`getpwuid`, `getgrgid`).
- Quản lý bộ nhớ động an toàn (không rò rỉ, không lỗi con trỏ NULL).
- Xử lý lỗi hệ thống và mã thoát chuẩn UNIX (`errno`, `strerror`, `stderr`).
- Biên dịch mã nguồn mô-đun hóa với `Makefile` trên môi trường UNIX chuẩn (NetBSD 10.1).

## 2. Mục tiêu và phạm vi

* Xây dựng từ đầu phiên bản đơn giản của lệnh `ls(1)` bằng ngôn ngữ C, tương thích hoàn toàn với hệ điều hành **NetBSD 10.1** và **Linux**.
* Cài đặt đầy đủ **19 tùy chọn (options)** theo manual page NetBSD 10.1:
  `-A`, `-a`, `-c`, `-d`, `-F`, `-f`, `-h`, `-i`, `-k`, `-l`, `-n`, `-q`, `-R`, `-r`, `-S`, `-s`, `-t`, `-u`, `-w`.
* Tổ chức mã nguồn theo mô hình mô-đun hóa (tách biệt thành 5 file `.c` và `.h` tương ứng).
* Viết code sạch, chuẩn C11/GNU11, có chú thích rõ ràng.
* Chương trình hoạt động ổn định, mạnh mẽ, xử lý tốt các trường hợp biên, tuyệt đối không bị segmentation fault.
* Cung cấp `Makefile`, `README.md` (Báo cáo) và `.gitignore`.

## 3. Cấu trúc dự án

Dự án được tổ chức gọn gàng và phân tách nhiệm vụ rõ ràng:

```
LeVanPhuoc_24IT212_midterm/
├── Makefile                # Cấu hình biên dịch (all, clean, test)
├── README.md               # Báo cáo đồ án giữa kì
├── .gitignore              # Loại trừ tệp thực thi myls, các file .o và file rác
├── main.c                  # Điểm vào chương trình, gọi options_parse() và list_main()
├── options.c               # Cài đặt hàm phân tích tham số dòng lệnh
├── options.h               # Định nghĩa struct Options và khai báo options_parse()
├── list.c                  # Duyệt thư mục, sắp xếp, đệ quy, in kết quả
├── list.h                  # Khai báo cấu trúc danh sách và các hàm duyệt
├── format.c                # Định dạng mode, owner, group, size, time cho long format
├── format.h                # Khai báo cấu trúc FileInfo và các hàm định dạng hiển thị
├── util.c                  # Hàm tiện ích: xmalloc, xstrdup, join_path, human_readable
├── util.h                  # Khai báo các hàm tiện ích
└── tests/
    └── test_ls.sh          # Bộ kiểm thử tự động toàn diện (38 test cases)
```

### Bảng phân công chức năng các file:

| File | Chức năng |
| :--- | :--- |
| `main.c` | Điểm vào chương trình, gọi `options_parse()` để nạp tham số và `list_main()` để xử lý. |
| `options.c` / `options.h` | Phân tích tham số dòng lệnh bằng `getopt()`, xử lý luật ghi đè cờ (overrides), lưu vào `struct Options`. |
| `list.c` / `list.h` | Duyệt thư mục (`opendir`, `readdir`), phân loại operands, sắp xếp (`qsort`), đệ quy (`-R`), in kết quả. |
| `format.c` / `format.h` | Định dạng chuỗi quyền hạn (10 ký tự), tra cứu owner, group, hiển thị size/device major minor, time cho long format (`-l`). |
| `util.c` / `util.h` | Các hàm tiện ích dùng chung: `xmalloc`, `xstrdup`, `join_path`, `human_readable`, `parse_blocksize`, `report_error`. |

## 4. Các tính năng đã triển khai

Ngoài chế độ mặc định hiển thị mỗi mục trên một dòng riêng biệt (không hiển thị tệp ẩn), chương trình hỗ trợ đầy đủ 19 tùy chọn theo NetBSD Manual và có thể kết hợp linh hoạt trong cùng một câu lệnh:

| Tùy chọn | Chức năng chi tiết |
| :---: | :--- |
| `-A` | Liệt kê tất cả các mục ẩn ngoại trừ `.` và `..`. Tự động bật mặc định cho super-user (`uid == 0`). |
| `-a` | Liệt kê tất cả các mục trong thư mục, bao gồm cả `.` và `..`. |
| `-c` | Dùng thời gian thay đổi trạng thái tệp (`st_ctime`) để sắp xếp (`-t`) hoặc hiển thị (`-l`). |
| `-d` | Liệt kê chính thư mục như tệp thông thường, không mở nội dung bên trong; không gián tiếp qua symlink trong tham số. |
| `-F` | Thêm ký hiệu phân loại sau tên tệp: `/` (thư mục), `*` (tệp thực thi), `@` (symlink), `=` (socket), `\|` (FIFO). |
| `-f` | Không sắp xếp kết quả (giữ nguyên thứ tự đọc của `readdir()`), tự động bật tùy chọn `-a`. |
| `-h` | Hiển thị kích thước tệp ở định dạng dễ đọc cho người dùng (ví dụ: `512B`, `1.5K`, `2M`, `64K`). Ghi đè `-k`. |
| `-i` | Hiển thị số hiệu inode (`st_ino`) của từng tệp. |
| `-k` | Hiển thị số block theo đơn vị kilobyte (1 KiB = 1024 bytes) cho tùy chọn `-s`. Ghi đè `-h`. |
| `-l` | Hiển thị thông tin chi tiết của tệp (long format: quyền, link, owner, group, size/device, date, name `-> target`). |
| `-n` | Định dạng danh sách dài tương tự `-l`, nhưng hiển thị UID và GID ở dạng số thay vì tra cứu tên chữ. |
| `-q` | Thay thế các ký tự không in được trong tên tệp thành dấu `?` (mặc định khi xuất ra terminal). |
| `-R` | Duyệt đệ quy tất cả các thư mục con bắt gặp. |
| `-r` | Đảo ngược thứ tự sắp xếp (ngược bảng chữ cái, hoặc nhỏ nhất/cũ nhất trước). |
| `-S` | Sắp xếp các mục theo kích thước giảm dần (tệp lớn nhất đứng đầu). |
| `-s` | Hiển thị số block hệ thống tập tin thực tế được cấp phát cho tệp. |
| `-t` | Sắp xếp theo thời gian mới nhất trước (hỗ trợ độ phân giải đến nanosecond). |
| `-u` | Dùng thời gian truy cập gần nhất (`st_atime`) để sắp xếp (`-t`) hoặc hiển thị (`-l`). |
| `-w` | Giữ nguyên các ký tự không in được ở dạng thô raw (mặc định khi đầu ra là đường ống pipe/file). |

## 5. Các hành vi bổ sung & Luồng xử lý chương trình

### Các hành vi bổ sung:
* **Nhiều toán hạng (Multiple operands):** Khi người dùng truyền nhiều đường dẫn vào câu lệnh, chương trình phân loại và in toàn bộ file thường trước (đã sắp xếp), sau đó in nội dung từng thư mục kèm tiêu đề `dirname:` (ngăn cách bằng dòng trống).
* **In dòng total:** Tự động tính toán và in dòng `total <số block>` trước danh sách khi dùng tùy chọn `-l` hoặc khi dùng `-s` xuất ra terminal.
* **Liên kết tượng trưng (Symlink):** Trong định dạng dài (`-l`), symlink hiển thị dạng `tên -> đích_đến`. Khi có cờ `-F`, symlink hiển thị ký hiệu `@`.
* **Thiết bị đặc biệt (Device Files):** Đối với Character special file (`c`) và Block special file (`b`), cột kích thước hiển thị chính xác số hiệu `major, minor` của thiết bị.
* **Xử lý ký tự không in được:** Tự động chuyển ký tự điều khiển thành `?` khi đầu ra là terminal (`-q`), giữ nguyên byte thô khi xuất qua pipe (`-w`). Hai cờ này ghi đè lẫn nhau theo vị trí cuối cùng được chỉ định.
* **Đệ quy an toàn (-R):** Tuyệt đối bỏ qua hai thư mục ảo `.` và `..` để ngăn ngừa hiện tượng lặp vô hạn (infinite loop).
* **Xử lý lỗi chuẩn UNIX:** Nếu gặp tệp không tồn tại hoặc không có quyền đọc thư mục, chương trình in thông báo lỗi chuẩn ra `stderr` (`ls: <đường_dẫn>: <lỗi>`), tiếp tục xử lý các toán hạng còn lại, và trả về mã thoát lớn hơn 0 (`EXIT_FAILURE = 1`).

### Luồng xử lý chương trình:

| Bước | Xử lý | Mô tả chi tiết |
| :---: | :--- | :--- |
| **1** | Phân tích tùy chọn | `main()` gọi `options_parse()` sử dụng `getopt()` để đọc tất cả các cờ, xử lý các quy tắc ghi đè và nạp vào cấu trúc `Options`. |
| **2** | Xử lý đường dẫn đầu vào | `list_main()` phân loại các toán hạng thành hai danh sách riêng biệt: mảng các file thường và mảng các thư mục. |
| **3** | Đọc thư mục | `read_dir()` dùng `opendir()` và `readdir()`, lọc file ẩn theo `-a`/`-A`, đọc thông tin chi tiết từng entry bằng `lstat()`. |
| **4** | Sắp xếp | `sort_file_infos()` sử dụng hàm thư viện chuẩn `qsort()` cùng hàm so sánh `cmp_fileinfo()` theo tiêu chí `-S`, `-t`, bảng chữ cái alphabet và hỗ trợ đảo ngược với `-r`. |
| **5** | Hiển thị thông tin file | `print_entries()` in từng entry; `format.c` đảm nhiệm tính toán bề rộng cột, căn lề và định dạng long format (`-l`, `-n`). |
| **6** | Gọi đệ quy khi có `-R` | `list_dir()` tự gọi lại chính nó cho từng thư mục con bắt gặp (bỏ qua `.` và `..`), in tiêu đề thư mục tương ứng. |

## 6. Cách biên dịch và chạy chương trình

### 6.1. Yêu cầu môi trường
* **Hệ điều hành:** NetBSD 10.1 (hoặc Linux có hỗ trợ `st_blocks`).
* **Trình biên dịch:** GCC 10.5.0 trở lên.
* **Công cụ build:** GNU Make (`make`).

Kiểm tra các công cụ trên NetBSD 10.1:
```bash
gcc --version
make --version
git --version
```

*(Nếu trên NetBSD chưa cài đặt Git, có thể cài đặt nhanh qua pkgin/pkg_add):*
```bash
export PKG_PATH="https://cdn.NetBSD.org/pub/pkgsrc/packages/NetBSD/amd64/10.1/All"
pkg_add git
```

### 6.2. Tải mã nguồn từ GitHub
Đăng nhập vào hệ thống NetBSD, chuyển về thư mục cá nhân và clone dự án:
```bash
cd ~
git clone https://github.com/PhuocLevi/LeVanPhuoc_24IT212_midterm.git
cd ~/LeVanPhuoc_24IT212_midterm
```

Kiểm tra các tệp mã nguồn:
```bash
ls -la
```

### 6.3. Biên dịch
Chương trình được biên dịch tự động thông qua `Makefile`:
```bash
make
```

Makefile áp dụng các cờ biên dịch tối ưu và nghiêm ngặt:
`-Wall -Wextra -Werror -pedantic -std=gnu11 -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -O2 -g`.
Quá trình biên dịch tạo ra file thực thi `myls` (đồng thời tạo liên kết tượng trưng `ls -> myls` để người dùng có thể chạy bằng cả hai tên lệnh).

```
gcc -Wall -Wextra -std=gnu11 -D_DEFAULT_SOURCE -c main.c -o main.o
gcc -Wall -Wextra -std=gnu11 -D_DEFAULT_SOURCE -c options.c -o options.o
gcc -Wall -Wextra -std=gnu11 -D_DEFAULT_SOURCE -c list.c -o list.o
gcc -Wall -Wextra -std=gnu11 -D_DEFAULT_SOURCE -c format.c -o format.o
gcc -Wall -Wextra -std=gnu11 -D_DEFAULT_SOURCE -c util.c -o util.o
gcc -Wall -Wextra -std=gnu11 -D_DEFAULT_SOURCE -o myls main.o options.o list.o format.o util.o
```

### 6.4. Chạy chương trình
Một số ví dụ thực thi chương trình:

| Lệnh thực thi | Ý nghĩa chức năng |
| :--- | :--- |
| `./myls` | Liệt kê nội dung thư mục hiện tại (mỗi mục trên 1 dòng). |
| `./myls -l` | Liệt kê định dạng chi tiết (long format). |
| `./myls -la` | Hiển thị tất cả các mục bao gồm file ẩn với thông tin chi tiết. |
| `./myls -lh /etc` | Liệt kê thư mục `/etc` theo định dạng dài với dung lượng dễ đọc (`-h`). |
| `./myls -lS` | Liệt kê định dạng dài, sắp xếp theo kích thước file giảm dần. |
| `./myls -lt` | Liệt kê định dạng dài, sắp xếp theo thời gian mới nhất trước. |
| `./myls -R /etc` | Duyệt và hiển thị đệ quy toàn bộ thư mục `/etc`. |
| `./myls -ld /etc` | Coi `/etc` là file thường, chỉ in thông tin chính thư mục mà không mở nội dung. |
| `./myls -F` | Thêm ký hiệu nhận diện thư mục (`/`), file thực thi (`*`), liên kết (`@`). |
| `./myls -sk` | Hiển thị số block thực tế cấp phát theo đơn vị kilobyte (1 KiB). |

### 6.5. Dọn dẹp sản phẩm biên dịch
Để xóa các tệp nhị phân và tệp đối tượng `.o` sau khi biên dịch:
```bash
make clean
```
Lệnh sẽ xóa sạch: `main.o`, `options.o`, `list.o`, `format.o`, `util.o`, `myls`, `ls`. Các tệp mã nguồn gốc `.c`, `.h` và tài liệu được bảo toàn nguyên vẹn.

## 7. Kiểm thử

Chương trình `myls` đã được kiểm thử trực tiếp trên hệ điều hành **NetBSD 10.1** và đối chiếu song song với lệnh `ls` gốc của hệ thống (`/bin/ls`). Toàn bộ 12 nhóm kiểm thử (gồm 38 ca kiểm thử tự động trong `tests/test_ls.sh`) đều đạt kết quả **PASS 100%**:

```bash
make test
```

### Kết quả chi tiết các nội dung kiểm thử:
1. **Hiển thị cơ bản & Mặc định:** Đúng định dạng 1 mục/dòng, ẩn chính xác các file bắt đầu bằng dấu chấm (`.`).
2. **Tùy chọn file ẩn (-a, -A):** `-a` hiển thị đầy đủ `.` và `..`; `-A` hiển thị các file ẩn nhưng loại trừ chính xác `.` và `..`.
3. **Định dạng dài (-l, -n):** Chuỗi mode 10 ký tự chính xác, hiển thị đầy đủ liên kết, owner, group, kích thước, thời gian; `-n` hiển thị dạng số numeric UID/GID; hiển thị chính xác `-> đích` của symlink; dòng `total` tính đúng tổng số block.
4. **Phân loại tệp (-F):** Thêm đúng `/` cho thư mục, `*` cho file thực thi (`chmod +x`), `@` cho symbolic link.
5. **Sắp xếp dữ liệu (-S, -t, -r, -f):** `-S` sắp file lớn nhất lên đầu; `-t` sắp file mới sửa đổi nhất lên đầu; `-r` đảo ngược thứ tự; `-f` xuất theo thứ tự thô của filesystem.
6. **Kích thước và khối (-s, -k, -h):** Tính chính xác số block 512-byte; `-k` tính đúng block theo 1024 bytes; `-h` hiển thị đúng đơn vị nhân bản `B`, `K`, `M`.
7. **Số hiệu Inode (-i):** In đúng số hiệu inode của hệ thống tập tin trước tên tệp.
8. **Thư mục thường và đệ quy (-d, -R):** `-d` không duyệt vào thư mục; `-R` duyệt chính xác mọi thư mục con và bỏ qua `.`/`..`.
9. **Ký tự không in được (-q, -w):** Tên file chứa byte điều khiển (ví dụ Bell `\007`) được chuyển thành dấu `?` với `-q` và giữ nguyên với `-w`.
10. **Quy tắc ghi đè cờ (Precedence & Overrides):** Kiểm thử thành công các cặp cờ đối nghịch: `-d` vs `-R`, `-l` vs `-n`, `-k` vs `-h`, `-q` vs `-w`.
11. **Thứ tự nhiều toán hạng:** File thường luôn hiển thị trước, các thư mục hiển thị sau cùng tiêu đề.
12. **Xử lý lỗi & Mã thoát:** Khi gặp file không tồn tại, in lỗi ra `stderr` và trả về mã thoát `> 0`.

*Chương trình cũng đã được kiểm tra tính an toàn bộ nhớ qua AddressSanitizer (`-fsanitize=address,undefined`), kết quả không xảy ra lỗi rò rỉ bộ nhớ (zero leaks) và không có lỗi truy cập vùng nhớ bất hợp pháp.*

## 8. Quản lý mã nguồn và kho lưu trữ

* Mã nguồn của dự án được quản lý chuyên nghiệp bằng hệ thống Git và lưu trữ công khai trên nền tảng GitHub:  
  **URL GitHub:** [https://github.com/PhuocLevi/LeVanPhuoc_24IT212_midterm](https://github.com/PhuocLevi/LeVanPhuoc_24IT212_midterm)
* Tệp `.gitignore` được cấu hình nghiêm ngặt nhằm đảm bảo không bao giờ commit các tệp nhị phân (`myls`, `ls`), các file đối tượng đối ứng (`*.o`), hay các tệp cấu hình IDE tạm thời.
* Lịch sử commit được thực hiện tuần tự, mạch lạc, tuân thủ quy chuẩn Conventional Commits.

## 9. Kết luận

Đồ án đã xây dựng thành công chương trình `myls` bằng ngôn ngữ C trên hệ điều hành **NetBSD 10.1** (và tương thích với Linux), mô phỏng đầy đủ tập con các chức năng của lệnh `ls` theo đúng tài liệu NetBSD Manual Page được cung cấp.

Chương trình triển khai hoàn chỉnh **19 tùy chọn**, hỗ trợ liệt kê tệp và thư mục, hiển thị thông tin chi tiết, tính toán số block, định dạng dung lượng dễ đọc, sắp xếp dữ liệu linh hoạt, xử lý liên kết tượng trưng và duyệt đệ quy mạnh mẽ.

Mã nguồn được tổ chức thành **5 mô-đun chuẩn mực (`.c` và `.h`)**, có `Makefile` tự động hóa quá trình biên dịch và bộ kiểm thử tự động 38 ca kiểm thử. Chương trình biên dịch sạch sẽ không có cảnh báo (`warning`), chạy ổn định và **hoàn toàn không bị lỗi segmentation fault**.
