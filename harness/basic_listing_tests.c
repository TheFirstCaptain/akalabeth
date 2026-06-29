#include "ak_basic_listing.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static char *read_file(const char *path) {
    FILE *file = fopen(path, "rb");
    long length;
    char *buffer;

    assert(file != NULL);
    assert(fseek(file, 0, SEEK_END) == 0);
    length = ftell(file);
    assert(length >= 0);
    assert(fseek(file, 0, SEEK_SET) == 0);

    buffer = (char *)malloc((size_t)length + 1);
    assert(buffer != NULL);
    assert(fread(buffer, 1, (size_t)length, file) == (size_t)length);
    buffer[length] = '\0';
    assert(fclose(file) == 0);
    return buffer;
}

static void test_working_listing_structure(void) {
    char *source = read_file("../AKLABETH.TXT");
    AkBasicLine line;

    assert(ak_basic_count_lines(source) == 383);
    assert(ak_basic_find_line(source, 0, &line));
    assert(ak_basic_line_contains(&line, "ONERR"));
    assert(ak_basic_find_line(source, 60250, &line));
    assert(ak_basic_line_contains(&line, "GOTO 60210"));

    assert(ak_basic_find_line(source, 60042, &line));
    assert(ak_basic_line_contains(&line, "SKELETON"));
    assert(ak_basic_line_contains(&line, "BALROG"));

    assert(ak_basic_find_line(source, 60070, &line));
    assert(ak_basic_line_contains(&line, "FOOD"));
    assert(ak_basic_line_contains(&line, "MAGIC AMULET"));

    free(source);
}

static void test_major_line_ranges_are_present(void) {
    char *source = read_file("../AKLABETH.TXT");

    assert(ak_basic_count_lines_in_range(source, 100, 190) == 10);
    assert(ak_basic_count_lines_in_range(source, 200, 490) == 103);
    assert(ak_basic_count_lines_in_range(source, 500, 590) == 21);
    assert(ak_basic_count_lines_in_range(source, 1000, 1700) == 97);
    assert(ak_basic_count_lines_in_range(source, 4000, 4999) == 31);
    assert(ak_basic_count_lines_in_range(source, 60000, 60250) == 45);

    free(source);
}

static void test_archive_listing_cross_check(void) {
    char *working_source = read_file("../AKLABETH.TXT");
    char *archive_source = read_file("../AKLABETH-org.TXT");
    AkBasicLine working_line;
    AkBasicLine archive_line;

    assert(ak_basic_count_lines(archive_source) == ak_basic_count_lines(working_source));
    assert(ak_basic_find_line(archive_source, 7000, &archive_line));
    assert(ak_basic_find_line(working_source, 7000, &working_line));
    assert(ak_basic_line_contains(&archive_line, "HOME"));
    assert(ak_basic_line_contains(&working_line, "HOME"));

    free(working_source);
    free(archive_source);
}

int main(void) {
    test_working_listing_structure();
    test_major_line_ranges_are_present();
    test_archive_listing_cross_check();
    return 0;
}
