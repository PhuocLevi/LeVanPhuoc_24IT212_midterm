# Midterm Project – Implement ls(1)

**Course:** Systems Programming (Lập trình hệ thống)  
**Student Name:** Lê Văn Phước  
**Student ID:** 24IT212  
**GitHub Repository:** [https://github.com/PhuocLevi/LeVanPhuoc_24IT212_midterm](https://github.com/PhuocLevi/LeVanPhuoc_24IT212_midterm)  
**Specification:** NetBSD 10.1 General Commands Manual — `ls(1)` (Subset)

---

## 1. Project Overview

This project is a modular, robust, and POSIX-compliant implementation of the UNIX `ls(1)` directory contents listing utility, developed from scratch in C according to the NetBSD 10.1 manual page specification.

The program interacts directly with the UNIX filesystem using low-level POSIX and BSD system calls (`stat`, `lstat`, `opendir`, `readdir`, `closedir`, `readlink`, `getpwuid`, `getgrgid`, etc.), implementing all required command-line options, flag overrides, environment variables, and sorting algorithms while adhering to rigorous software engineering practices.

---

## 2. Project Architecture & Modular Design

The codebase is organized into modular source files (`.c`) and corresponding header files (`.h`):

```
LeVanPhuoc_24IT212_midterm/
├── Makefile                # Build system (all, clean, test)
├── .gitignore              # Ignores compiled binaries, .o files, and build artifacts
├── README.md               # Project report & documentation
├── include/                # Header files (.h)
│   ├── ls.h                # Core definitions, options struct, file entry struct
│   ├── display.h           # Formatted output (-l, columns, permissions, indicators)
│   ├── sort.h              # Sorting algorithms and comparator functions
│   ├── traversal.h         # Directory inspection, operands dispatch, recursion (-R)
│   └── util.h              # Memory safety, block calculation, humanize numbers
├── src/                    # Source files (.c)
│   ├── main.c              # CLI argument parsing (getopt), overrides, environment
│   ├── display.c           # Mode formatting, column width calculation, output display
│   ├── sort.c              # qsort comparators (size, time, lexicographical, reverse)
│   ├── traversal.c         # Directory reading, operands handling, recursive listing
│   └── util.c              # Safe wrappers, BLOCKSIZE parser, humanize_size
└── tests/                  # Automated test suite
    └── test_ls.sh          # Comprehensive automated regression tests (38 tests)
```

### Module Responsibilities:

1. **`include/ls.h`**:
   - Contains feature test macros (`_POSIX_C_SOURCE 200809L`, `_DEFAULT_SOURCE`, `_BSD_SOURCE`, `_XOPEN_SOURCE 700`).
   - Defines `ls_options_t` storing all parsed command-line flags and environment parameters.
   - Defines `file_entry_t` storing metadata for each file: name, path, `struct stat`, error status, symlink target, and directory classification.

2. **`src/main.c`**:
   - Parses command-line arguments using `getopt()`.
   - Handles environment variables (`BLOCKSIZE`, `TZ`).
   - Detects whether standard output is connected to a terminal via `isatty(STDOUT_FILENO)`.
   - Enforces flag precedence and mutual overrides.
   - Detects superuser (`getuid() == 0`) to default to `-A`.

3. **`src/traversal.c` / `include/traversal.h`**:
   - Handles operand processing: classifies non-directory vs. directory operands, ensures non-directories are displayed first, and sorts directory operands separately.
   - Implements directory reading with entry filtering (`-a` vs `-A` vs default).
   - Implements recursive traversal (`-R`) with cycle prevention (avoids `.` and `..`).
   - Manages symbolic link indirection according to `-d`.

4. **`src/sort.c` / `include/sort.h`**:
   - Implements sorting strategies:
     - Lexicographical (default).
     - File size descending (`-S`), ties broken lexicographically.
     - Modification/status change/access time descending (`-t`), using nanosecond precision with lexicographical tie-breaking.
     - Unsorted (`-f`).
     - Reverse order inversion (`-r`).

5. **`src/display.c` / `include/display.h`**:
   - Constructs the 10-character file mode string: file type (`-`, `d`, `l`, `b`, `c`, `s`, `p`, `w`) and permissions (including SUID `s`/`S`, SGID `s`/`S`, and Sticky bit `t`/`T`).
   - Computes dynamic column widths across entries for aligned long listings (`-l`).
   - Displays device numbers (`major, minor`) for character and block special files.
   - Displays symlink destination (`-> target`).
   - Appends type indicator characters (`-F`).
   - Computes and displays `total <blocks>` for directories in `-l` and on terminal output with `-s`.

6. **`src/util.c` / `include/util.h`**:
   - Safe dynamic memory wrappers (`safe_malloc`, `safe_realloc`, `safe_strdup`) to prevent NULL dereferences.
   - Path concatenation (`path_join`).
   - Parses the `BLOCKSIZE` environment variable (supporting unit suffixes `K`, `M`, `G` and minimum 512-byte block size).
   - Calculates block count in units of 512 bytes, 1024 bytes (`-k`), or custom `BLOCKSIZE`.
   - Implements `humanize_size()` matching NetBSD's `humanize_number(3)`.
   - Implements character filtering for non-printable characters (`-q` vs `-w`).

---

## 3. Implemented Features & NetBSD Option Coverage

All 18 options specified in the synopsis `ls [-AacdFfhiklnqRrSstuw] [file ...]` are implemented:

| Option | Description | Implementation Details |
| :---: | :--- | :--- |
| `-A` | List all entries except for `.` and `..` | Automatically activated when running as super-user (`uid == 0`). Excludes literal `.` and `..` but lists other dot-files. |
| `-a` | Include all directory entries starting with `.` | Displays all hidden entries, including `.` and `..`. Overrides `-A`. |
| `-c` | Use time of status change (`st_ctime`) | Used for sorting (`-t`) or printing (`-l`). Overrides `-u`. |
| `-d` | Directories listed as plain files | Prevents descending into directories; symbolic links in operands are not indirected. Overrides `-R`. |
| `-F` | Display type indicators | Appends `/` (directory), `*` (executable regular file), `@` (symlink), `=` (socket), `\|` (FIFO), `%` (whiteout). |
| `-f` | Output is not sorted | Disables sorting; retains `readdir()` directory order. Enables `-a` by default. |
| `-h` | Human-readable sizes for `-s` and `-l` | Formats byte counts using base-1024 units (`B`, `K`, `M`, `G`, `T`). Overrides `-k`. |
| `-i` | Print file serial number (inode) | Displays `st_ino` before file information. |
| `-k` | Sizes reported in kilobytes | Uses 1024-byte block units for `-s`. Overrides `-h`. |
| `-l` | List in long format | Displays 10-char mode, links, owner, group, size (or major/minor), date, and name (with `-> target`). Overrides `-n`. |
| `-n` | Numeric UID and GID in long format | Formats user and group as numeric IDs instead of resolving via `/etc/passwd` or `/etc/group`. Overrides `-l`. |
| `-q` | Replace non-printable characters with `?` | Replaces bytes where `isprint() == 0` with `?`. Default when standard output is a terminal. Overrides `-w`. |
| `-R` | Recursively list subdirectories | Recursively inspects all subdirectories (skipping `.` and `..` to avoid infinite loops). Overrides `-d`. |
| `-r` | Reverse sort order | Inverts the comparison order for lexicographical, size, or time sorting. |
| `-S` | Sort by size, largest first | Compares `st_size` descending. Resolves ties using lexicographical order. |
| `-s` | Display number of file system blocks | Computes allocated blocks rounded up to the nearest unit block. Outputs `total <sum>` header. |
| `-t` | Sort by timestamp | Sorts newest first by modification time (or ctime with `-c`, atime with `-u`), using nanosecond precision. |
| `-u` | Use last access time (`st_atime`) | Used for sorting (`-t`) or printing (`-l`). Overrides `-c`. |
| `-w` | Force raw printing of non-printable characters | Emits raw byte characters directly. Default when output is redirected (pipe/file). Overrides `-q`. |

### Option Overriding Rules:
- `-w` and `-q`: The last specified option determines the non-printable character format.
- `-l` and `-n`: The last specified option determines the long listing format (names vs numeric IDs).
- `-c` and `-u`: The last specified option selects the timestamp used.
- `-R` and `-d`: The last specified option controls recursion vs treating directories as plain files.
- `-h` and `-k`: The rightmost flag overrides the previous one for block and size reporting.
- `-S` and `-t`: The last specified flag determines the sorting criteria.

---

## 4. Edge Cases & Robustness

The implementation includes comprehensive defenses against runtime failures and edge cases:

1. **Zero Segmentation Faults**:
   - All dynamic memory allocations are checked using `safe_malloc`, `safe_realloc`, and `safe_strdup`.
   - Column width calculation safely handles empty directories, single operands, and missing files without NULL pointer dereferences.

2. **Error Recovery & Exit Codes**:
   - When encountering unreadable directories or non-existent files, the error is written to `stderr` (`ls: <path>: <error>`), remaining operands continue processing, and the program exits with code `1` (`EXIT_FAILURE`).

3. **Symbolic Link Handling**:
   - Without `-d`: Command-line operands that are symbolic links pointing to directories are resolved via `stat()` and listed as directories.
   - With `-d`: Symbolic link operands are not dereferenced and are listed as plain symlinks.
   - Broken symlinks are handled gracefully by falling back to `lstat()`.

4. **Special Permission Bits**:
   - SUID: Displays `s` if owner-executable, or `S` if not owner-executable.
   - SGID: Displays `s` if group-executable, or `S` if not group-executable.
   - Sticky bit: Displays `t` if other-executable, or `T` if not other-executable.

5. **Device Files Support**:
   - Character special files (`/dev/null`, `/dev/urandom`) and block special files correctly format major and minor device numbers (`major, minor`) in the size column.

6. **Environment Variable Handling**:
   - `BLOCKSIZE`: Parses custom sizes (e.g., `export BLOCKSIZE=1k`, `BLOCKSIZE=2048`). Minimum size is clamped to 512 bytes.
   - `TZ`: Automatically respected by `localtime_r()`.

7. **Clean Memory Lifecycle**:
   - All allocated file entry structures and directory handles (`closedir`) are freed systematically.
   - Verified clean with AddressSanitizer (`-fsanitize=address,undefined`).

---

## 5. Compilation & Running

### Prerequisites:
- GCC compiler (`gcc`) supporting standard C99/GNU99.
- GNU Make (`make`).
- Linux / UNIX environment.

### Build Commands:

```bash
# Compile the project
make

# Clean build artifacts (.o files and binary)
make clean

# Run the automated test suite
make test
```

### Usage Examples:

```bash
# Basic listing (one entry per line)
./ls

# Long format with human-readable sizes and hidden files
./ls -lah

# Sort files by size, largest first
./ls -lS

# Sort files by last modification time
./ls -lt

# Long listing with numeric UID/GID
./ls -n

# Display block counts in kilobytes
./ls -sk

# Recursive listing
./ls -R include/

# Inspect directory itself without recursing into it
./ls -ld include/

# Display type indicators
./ls -F
```

---

## 6. Automated Testing & Verification

A dedicated bash test suite is located in `tests/test_ls.sh`, containing 38 automated test cases grouped into 12 test categories:

- **Group 1:** Basic Operands & Defaults (one entry per line, default exclusion of hidden files).
- **Group 2:** Hidden Files (`-a` includes `.` and `..`, `-A` excludes `.` and `..`).
- **Group 3:** Long Format (`-l` total lines, permissions, symlink targets; `-n` numeric IDs).
- **Group 4:** Sorting (`-S` size ordering, `-t` modification time, `-r` reversal, `-f` unsorted).
- **Group 5:** Indicators (`-F` with `/`, `*`, `@`).
- **Group 6:** Sizing and Blocks (`-s` block counts, `-sk` kilobytes, `-lh` human-readable).
- **Group 7:** Inodes (`-i` inode serial numbers).
- **Group 8:** Directory Flags (`-d` plain file listing, `-R` recursive traversal).
- **Group 9:** Non-printable Characters (`-q` question mark substitution, `-w` raw mode).
- **Group 10:** Flag Precedence & Mutual Overrides (`-d` vs `-R`, `-l` vs `-n`, `-k` vs `-h`).
- **Group 11:** Operand Ordering (non-directory operands displayed first, directory operands second).
- **Group 12:** Robustness & Exit Codes (non-existent operands, invalid options, error messages).

### Test Suite Execution Output:

```
======================================================
  Running ls(1) NetBSD Test Suite - Student: 24IT212   
======================================================

Test Group 1: Basic Operands & Defaults
  [PASS] Default list of directory
  [PASS] Default hides dotfiles
  [PASS] Default hides dot directories
  [PASS] Single file operand
  [PASS] Single file output

Test Group 2: Hidden Files (-a and -A)
  [PASS] -a shows '.'
  [PASS] -a shows '..'
  [PASS] -a shows hidden file
  [PASS] -A excludes '.'
  [PASS] -A excludes '..'
  [PASS] -A includes hidden file

Test Group 3: Long Format (-l and -n)
  [PASS] -l contains total line
  [PASS] -l shows permissions
  [PASS] -l shows symlink destination
  [PASS] -n shows numeric UID

Test Group 4: Sorting (-S, -t, -r, -f)
  [PASS] -S sorts largest file first
  [PASS] -r reverses lexicographical ordering
  [PASS] -t sorts most recently modified file first
  [PASS] -f unsorted output succeeds

Test Group 5: Indicators (-F)
  [PASS] -F appends / to directory
  [PASS] -F appends @ to symlink
  [PASS] -F appends * to executable

Test Group 6: Size and Block formats (-s, -k, -h)
  [PASS] -s prints block count
  [PASS] -sk reports kilobytes
  [PASS] -h reports human readable sizes

Test Group 7: Inode number (-i)
  [PASS] -i displays inode numbers

Test Group 8: Directory flags (-d and -R)
  [PASS] -d lists dir as plain file
  [PASS] -R recursively lists subdirectories

Test Group 9: Non-printable characters (-q and -w)
  [PASS] -q replaces non-printable with '?'

Test Group 10: Flag Precedence and Overrides
  [PASS] -d after -R cancels recursion
  [PASS] -R after -d enables recursion
  [PASS] -n after -l displays numeric ID
  [PASS] -l after -n displays username
  [PASS] -h after -k uses human format

Test Group 11: Operand Ordering (files before dirs)
  [PASS] Non-directory operands displayed before directory operands

Test Group 12: Robustness and Exit Codes
  [PASS] Exits > 0 on non-existent file
  [PASS] Exits > 0 on invalid option
  [PASS] Exits > 0 when one operand is missing

======================================================
  Test Suite Finished: 38 Passed, 0 Failed
======================================================
```
