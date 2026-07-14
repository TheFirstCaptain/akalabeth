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

static void test_behavior_map_source_anchors(void) {
    char *source = read_file("../AKLABETH.TXT");
    AkBasicLine line;

    assert(ak_basic_find_line(source, 8, &line));
    assert(ak_basic_line_contains(&line, "RND ( -  ABS (LN))"));

    assert(ak_basic_find_line(source, 40, &line));
    assert(ak_basic_line_contains(&line, "TE%(X,Y)"));
    assert(ak_basic_line_contains(&line, "RND (1) ^ 5"));

    assert(ak_basic_find_line(source, 100, &line));
    assert(ak_basic_line_contains(&line, "HGR"));

    assert(ak_basic_find_line(source, 202, &line));
    assert(ak_basic_line_contains(&line, "DNG%"));

    assert(ak_basic_find_line(source, 500, &line));
    assert(ak_basic_line_contains(&line, "RND ( -  ABS (LN)"));

    assert(ak_basic_find_line(source, 1000, &line));
    assert(ak_basic_line_contains(&line, "COMMAND?"));

    assert(ak_basic_find_line(source, 1090, &line));
    assert(ak_basic_line_contains(&line, "YOU HAVE STARVED"));

    assert(ak_basic_find_line(source, 1500, &line));
    assert(ak_basic_line_contains(&line, "GOSUB 60080"));

    assert(ak_basic_find_line(source, 1650, &line));
    assert(ak_basic_line_contains(&line, "ATTACK"));

    assert(ak_basic_find_line(source, 1680, &line));
    assert(ak_basic_line_contains(&line, "PW(5)"));

    assert(ak_basic_find_line(source, 2005, &line));
    assert(ak_basic_line_contains(&line, "MZ%(X,0)"));

    assert(ak_basic_find_line(source, 4000, &line));
    assert(ak_basic_line_contains(&line, "MZ%(MM,0)"));

    assert(ak_basic_find_line(source, 4500, &line));
    assert(ak_basic_line_contains(&line, "MM = 2 OR MM = 7"));

    assert(ak_basic_find_line(source, 6000, &line));
    assert(ak_basic_line_contains(&line, "WE MOURN THE PASSING"));

    assert(ak_basic_find_line(source, 7000, &line));
    assert(ak_basic_line_contains(&line, "HOME"));

    assert(ak_basic_find_line(source, 7050, &line));
    assert(ak_basic_line_contains(&line, "TASK"));

    assert(ak_basic_find_line(source, 60020, &line));
    assert(ak_basic_line_contains(&line, "HIT POINTS"));

    assert(ak_basic_find_line(source, 60042, &line));
    assert(ak_basic_line_contains(&line, "BALROG"));

    assert(ak_basic_find_line(source, 60070, &line));
    assert(ak_basic_line_contains(&line, "MAGIC AMULET"));

    assert(ak_basic_find_line(source, 60220, &line));
    assert(ak_basic_line_contains(&line, "FOOD"));

    free(source);
}

int main(void) {
    test_working_listing_structure();
    test_major_line_ranges_are_present();
    test_behavior_map_source_anchors();
    return 0;
}
