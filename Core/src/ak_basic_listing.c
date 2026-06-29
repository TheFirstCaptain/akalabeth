#include "ak_basic_listing.h"

#include <ctype.h>
#include <string.h>

static int parse_number(const char **cursor, int *number) {
    int value = 0;
    const char *p = *cursor;

    if (!isdigit((unsigned char)*p)) {
        return 0;
    }

    while (isdigit((unsigned char)*p)) {
        value = value * 10 + (*p - '0');
        p++;
    }

    *cursor = p;
    *number = value;
    return 1;
}

int ak_basic_visit_lines(const char *source, AkBasicLineVisitor visitor, void *context) {
    const char *line_start = source;
    size_t source_line = 1;

    if (source == NULL || visitor == NULL) {
        return 0;
    }

    while (*line_start != '\0') {
        const char *line_end = strchr(line_start, '\n');
        const char *cursor = line_start;
        size_t indent = 0;
        int number = 0;

        if (line_end == NULL) {
            line_end = line_start + strlen(line_start);
        }

        while (*cursor == ' ') {
            cursor++;
            indent++;
        }

        if (indent <= 1 && parse_number(&cursor, &number)) {
            AkBasicLine line;
            while (*cursor == ' ' || *cursor == '\t') {
                cursor++;
            }
            line.number = number;
            line.text = cursor;
            line.text_length = (size_t)(line_end - cursor);
            if (line.text_length > 0 && line.text[line.text_length - 1] == '\r') {
                line.text_length--;
            }
            line.source_line = source_line;

            if (!visitor(&line, context)) {
                return 0;
            }
        }

        if (*line_end == '\0') {
            break;
        }

        line_start = line_end + 1;
        source_line++;
    }

    return 1;
}

static int count_visitor(const AkBasicLine *line, void *context) {
    size_t *count = (size_t *)context;
    (void)line;
    (*count)++;
    return 1;
}

size_t ak_basic_count_lines(const char *source) {
    size_t count = 0;
    ak_basic_visit_lines(source, count_visitor, &count);
    return count;
}

typedef struct {
    int number;
    AkBasicLine *out_line;
    int found;
} FindContext;

static int find_visitor(const AkBasicLine *line, void *context) {
    FindContext *find = (FindContext *)context;
    if (line->number == find->number) {
        *find->out_line = *line;
        find->found = 1;
        return 0;
    }
    return 1;
}

int ak_basic_find_line(const char *source, int number, AkBasicLine *out_line) {
    FindContext context;

    if (source == NULL || out_line == NULL) {
        return 0;
    }

    context.number = number;
    context.out_line = out_line;
    context.found = 0;
    ak_basic_visit_lines(source, find_visitor, &context);
    return context.found;
}

int ak_basic_line_contains(const AkBasicLine *line, const char *needle) {
    size_t needle_length;
    size_t i;

    if (line == NULL || needle == NULL) {
        return 0;
    }

    needle_length = strlen(needle);
    if (needle_length == 0 || needle_length > line->text_length) {
        return 0;
    }

    for (i = 0; i <= line->text_length - needle_length; i++) {
        if (memcmp(line->text + i, needle, needle_length) == 0) {
            return 1;
        }
    }

    return 0;
}

typedef struct {
    int first;
    int last;
    size_t count;
} RangeContext;

static int range_visitor(const AkBasicLine *line, void *context) {
    RangeContext *range = (RangeContext *)context;
    if (line->number >= range->first && line->number <= range->last) {
        range->count++;
    }
    return 1;
}

size_t ak_basic_count_lines_in_range(const char *source, int first, int last) {
    RangeContext context;
    context.first = first;
    context.last = last;
    context.count = 0;
    ak_basic_visit_lines(source, range_visitor, &context);
    return context.count;
}
