#!/bin/sh
# ==============================================================================
# Comprehensive Test Suite for ls(1) implementation
# NetBSD 10.1 subset specification
# Author: Lê Văn Phước (24IT212)
# ==============================================================================

set -u

LS_BIN="./myls"
if [ ! -x "$LS_BIN" ]; then
    LS_BIN="./ls"
fi
TEST_DIR="test_sandbox_tmp"
PASS_COUNT=0
FAIL_COUNT=0

# Colors for test output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

assert_success() {
    desc="$1"
    shift
    if "$@"; then
        printf "  [${GREEN}PASS${NC}] %s\n" "$desc"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        printf "  [${RED}FAIL${NC}] %s (Command failed: %s)\n" "$desc" "$*"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
}

assert_failure() {
    desc="$1"
    shift
    if ! "$@"; then
        printf "  [${GREEN}PASS${NC}] %s\n" "$desc"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        printf "  [${RED}FAIL${NC}] %s (Command expected to fail but succeeded: %s)\n" "$desc" "$*"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
}

assert_output_contains() {
    desc="$1"
    expected="$2"
    shift 2
    output=$("$@")
    if echo "$output" | grep -q -E -- "$expected"; then
        printf "  [${GREEN}PASS${NC}] %s\n" "$desc"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        printf "  [${RED}FAIL${NC}] %s (Expected substring '%s' not found)\n" "$desc" "$expected"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
}

assert_output_not_contains() {
    desc="$1"
    unexpected="$2"
    shift 2
    output=$("$@")
    if ! echo "$output" | grep -q -E -- "$unexpected"; then
        printf "  [${GREEN}PASS${NC}] %s\n" "$desc"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        printf "  [${RED}FAIL${NC}] %s (Unexpected substring '%s' found)\n" "$desc" "$unexpected"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
}

setup_sandbox() {
    rm -rf "$TEST_DIR"
    mkdir -p "$TEST_DIR"
    mkdir -p "$TEST_DIR/subdir1"
    mkdir -p "$TEST_DIR/subdir2"
    mkdir -p "$TEST_DIR/.hidden_dir"

    touch "$TEST_DIR/alpha.txt"
    touch "$TEST_DIR/beta.txt"
    touch "$TEST_DIR/gamma.txt"
    touch "$TEST_DIR/.hidden_file"
    echo "Small file" > "$TEST_DIR/small.txt"
    dd if=/dev/zero of="$TEST_DIR/large.bin" bs=1024 count=64 2>/dev/null

    chmod 755 "$TEST_DIR/subdir1"
    chmod 644 "$TEST_DIR/alpha.txt"
    chmod 755 "$TEST_DIR/beta.txt"

    ln -s "$TEST_DIR/small.txt" "$TEST_DIR/link_to_small"
    ln -s "$TEST_DIR/subdir1" "$TEST_DIR/link_to_sub1"
    ln -s "$TEST_DIR/non_existent" "$TEST_DIR/broken_link"

    # Non-printable character file (Bell \007)
    touch "$TEST_DIR/file_with_`printf '\007'`_bell"
}

cleanup_sandbox() {
    rm -rf "$TEST_DIR"
}

trap cleanup_sandbox EXIT INT TERM

printf "${BLUE}======================================================${NC}\n"
printf "${BLUE}  Running ls(1) NetBSD Test Suite - Student: 24IT212   ${NC}\n"
printf "${BLUE}======================================================${NC}\n"

setup_sandbox

# ------------------------------------------------------------------------------
# 1. Basic functionality & operands
# ------------------------------------------------------------------------------
printf "\n${YELLOW}Test Group 1: Basic Operands & Defaults${NC}\n"
assert_output_contains "Default list of directory" "alpha.txt" $LS_BIN "$TEST_DIR"
assert_output_not_contains "Default hides dotfiles" ".hidden_file" $LS_BIN "$TEST_DIR"
assert_output_not_contains "Default hides dot directories" ".hidden_dir" $LS_BIN "$TEST_DIR"
assert_success "Single file operand" $LS_BIN "$TEST_DIR/alpha.txt"
assert_output_contains "Single file output" "alpha.txt" $LS_BIN "$TEST_DIR/alpha.txt"

# ------------------------------------------------------------------------------
# 2. Hidden entries: -a and -A
# ------------------------------------------------------------------------------
printf "\n${YELLOW}Test Group 2: Hidden Files (-a and -A)${NC}\n"
assert_output_contains "-a shows '.'" "^\.$" $LS_BIN -a "$TEST_DIR"
assert_output_contains "-a shows '..'" "^\.\.$" $LS_BIN -a "$TEST_DIR"
assert_output_contains "-a shows hidden file" ".hidden_file" $LS_BIN -a "$TEST_DIR"
assert_output_not_contains "-A excludes '.'" "^\.$" $LS_BIN -A "$TEST_DIR"
assert_output_not_contains "-A excludes '..'" "^\.\.$" $LS_BIN -A "$TEST_DIR"
assert_output_contains "-A includes hidden file" ".hidden_file" $LS_BIN -A "$TEST_DIR"

# ------------------------------------------------------------------------------
# 3. Long listing: -l and -n
# ------------------------------------------------------------------------------
printf "\n${YELLOW}Test Group 3: Long Format (-l and -n)${NC}\n"
assert_output_contains "-l contains total line" "^total" $LS_BIN -l "$TEST_DIR"
assert_output_contains "-l shows permissions" "-rw-r--r--" $LS_BIN -l "$TEST_DIR"
assert_output_contains "-l shows symlink destination" "link_to_small ->" $LS_BIN -l "$TEST_DIR"
assert_output_contains "-n shows numeric UID" "[0-9]+" $LS_BIN -n "$TEST_DIR"

# ------------------------------------------------------------------------------
# 4. Sorting options: -S, -t, -r, -f
# ------------------------------------------------------------------------------
printf "\n${YELLOW}Test Group 4: Sorting (-S, -t, -r, -f)${NC}\n"
# -S: large.bin (64K) must appear before small.txt (11 bytes)
LARGE_FIRST=$($LS_BIN -S "$TEST_DIR" | grep -E "(large\.bin|small\.txt)" | head -n1)
if [ "$LARGE_FIRST" = "large.bin" ]; then
    printf "  [${GREEN}PASS${NC}] -S sorts largest file first\n"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    printf "  [${RED}FAIL${NC}] -S expected large.bin first, got %s\n" "$LARGE_FIRST"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# -r: reverses sort
REV_FIRST=$($LS_BIN -r "$TEST_DIR" | grep -v "^\." | tail -n1)
NORM_FIRST=$($LS_BIN "$TEST_DIR" | grep -v "^\." | head -n1)
if [ "$REV_FIRST" = "$NORM_FIRST" ]; then
    printf "  [${GREEN}PASS${NC}] -r reverses lexicographical ordering\n"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    printf "  [${RED}FAIL${NC}] -r order mismatch: %s vs %s\n" "$REV_FIRST" "$NORM_FIRST"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# -t: newest file first
touch "$TEST_DIR/alpha.txt"
NEWEST=$($LS_BIN -t "$TEST_DIR" | grep -E "(alpha\.txt|beta\.txt)" | head -n1)
if [ "$NEWEST" = "alpha.txt" ]; then
    printf "  [${GREEN}PASS${NC}] -t sorts most recently modified file first\n"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    printf "  [${RED}FAIL${NC}] -t expected alpha.txt first, got %s\n" "$NEWEST"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

assert_success "-f unsorted output succeeds" $LS_BIN -f "$TEST_DIR"

# ------------------------------------------------------------------------------
# 5. Type indicators (-F)
# ------------------------------------------------------------------------------
printf "\n${YELLOW}Test Group 5: Indicators (-F)${NC}\n"
assert_output_contains "-F appends / to directory" "subdir1/" $LS_BIN -F "$TEST_DIR"
assert_output_contains "-F appends @ to symlink" "link_to_small@" $LS_BIN -F "$TEST_DIR"
assert_output_contains "-F appends * to executable" "beta.txt\*" $LS_BIN -F "$TEST_DIR"

# ------------------------------------------------------------------------------
# 6. Sizing and blocks (-s, -k, -h)
# ------------------------------------------------------------------------------
printf "\n${YELLOW}Test Group 6: Size and Block formats (-s, -k, -h)${NC}\n"
assert_output_contains "-s prints block count" "[0-9]+ alpha\.txt" $LS_BIN -s "$TEST_DIR"
assert_output_contains "-sk reports kilobytes" "[0-9]+ large\.bin" $LS_BIN -sk "$TEST_DIR"
assert_output_contains "-h reports human readable sizes" "64K" $LS_BIN -lh "$TEST_DIR"

# ------------------------------------------------------------------------------
# 7. Inodes (-i)
# ------------------------------------------------------------------------------
printf "\n${YELLOW}Test Group 7: Inode number (-i)${NC}\n"
assert_output_contains "-i displays inode numbers" "^[0-9]+ " $LS_BIN -i "$TEST_DIR"

# ------------------------------------------------------------------------------
# 8. Directory flags (-d and -R)
# ------------------------------------------------------------------------------
printf "\n${YELLOW}Test Group 8: Directory flags (-d and -R)${NC}\n"
assert_output_contains "-d lists dir as plain file" "^$TEST_DIR$" $LS_BIN -d "$TEST_DIR"
assert_output_contains "-R recursively lists subdirectories" "subdir1:" $LS_BIN -R "$TEST_DIR"

# ------------------------------------------------------------------------------
# 9. Non-printable characters (-q vs -w)
# ------------------------------------------------------------------------------
printf "\n${YELLOW}Test Group 9: Non-printable characters (-q and -w)${NC}\n"
assert_output_contains "-q replaces non-printable with '?'" "file_with_\?_bell" $LS_BIN -q "$TEST_DIR"

# ------------------------------------------------------------------------------
# 10. Flag precedence and overrides
# ------------------------------------------------------------------------------
printf "\n${YELLOW}Test Group 10: Flag Precedence and Overrides${NC}\n"
assert_output_not_contains "-d after -R cancels recursion" "subdir1:" $LS_BIN -R -d "$TEST_DIR"
assert_output_contains "-R after -d enables recursion" "subdir1:" $LS_BIN -d -R "$TEST_DIR"
assert_output_contains "-n after -l displays numeric ID" "[0-9]+" $LS_BIN -l -n "$TEST_DIR"
assert_output_contains "-h after -k uses human format" "64K" $LS_BIN -k -h -l "$TEST_DIR"

# ------------------------------------------------------------------------------
# 11. Multiple and mixed operands
# ------------------------------------------------------------------------------
printf "\n${YELLOW}Test Group 11: Operand Ordering (files before dirs)${NC}\n"
MIXED_OUT=$($LS_BIN "$TEST_DIR/alpha.txt" "$TEST_DIR/subdir1")
ALPHA_LINE=$(echo "$MIXED_OUT" | grep -n "alpha.txt" | cut -d: -f1)
SUBDIR_LINE=$(echo "$MIXED_OUT" | grep -n "subdir1:" | cut -d: -f1)
if [ "$ALPHA_LINE" -lt "$SUBDIR_LINE" ]; then
    printf "  [${GREEN}PASS${NC}] Non-directory operands displayed before directory operands\n"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    printf "  [${RED}FAIL${NC}] Expected non-dir before dir operand\n"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

# ------------------------------------------------------------------------------
# 12. Error handling & exit statuses
# ------------------------------------------------------------------------------
printf "\n${YELLOW}Test Group 12: Robustness and Exit Codes${NC}\n"
assert_failure "Exits > 0 on non-existent file" $LS_BIN "$TEST_DIR/does_not_exist"
assert_failure "Exits > 0 on invalid option" $LS_BIN -Z "$TEST_DIR"
assert_failure "Exits > 0 when one operand is missing" $LS_BIN "$TEST_DIR/alpha.txt" "$TEST_DIR/does_not_exist"

# ------------------------------------------------------------------------------
# Summary
# ------------------------------------------------------------------------------
printf "\n${BLUE}======================================================${NC}\n"
printf "${BLUE}  Test Suite Finished: %d Passed, %d Failed${NC}\n" "$PASS_COUNT" "$FAIL_COUNT"
printf "${BLUE}======================================================${NC}\n"

if [ "$FAIL_COUNT" -eq 0 ]; then
    exit 0
else
    exit 1
fi
