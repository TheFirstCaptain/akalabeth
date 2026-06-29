#ifndef AK_BASIC_LISTING_H
#define AK_BASIC_LISTING_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int number;
    const char *text;
    size_t text_length;
    size_t source_line;
} AkBasicLine;

typedef int (*AkBasicLineVisitor)(const AkBasicLine *line, void *context);

int ak_basic_visit_lines(const char *source, AkBasicLineVisitor visitor, void *context);
size_t ak_basic_count_lines(const char *source);
int ak_basic_find_line(const char *source, int number, AkBasicLine *out_line);
int ak_basic_line_contains(const AkBasicLine *line, const char *needle);
size_t ak_basic_count_lines_in_range(const char *source, int first, int last);

#ifdef __cplusplus
}
#endif

#endif

