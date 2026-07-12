#ifndef AK_RENDER_H
#define AK_RENDER_H

#include <stddef.h>

#include "ak_game.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AK_RENDER_MAX_COMMANDS 256
#define AK_RENDER_TEXT_SIZE 64

typedef enum {
    AK_RENDER_MODE_TEXT = 0,
    AK_RENDER_MODE_HIRES
} AkRenderMode;

typedef enum {
    AK_RENDER_COLOR_BLACK = 0,
    AK_RENDER_COLOR_WHITE,
    AK_RENDER_COLOR_GREEN,
    AK_RENDER_COLOR_VIOLET,
    AK_RENDER_COLOR_ORANGE,
    AK_RENDER_COLOR_BLUE
} AkRenderColor;

typedef enum {
    AK_RENDER_COMMAND_NONE = 0,
    AK_RENDER_COMMAND_SET_MODE,
    AK_RENDER_COMMAND_CLEAR,
    AK_RENDER_COMMAND_SET_COLOR,
    AK_RENDER_COMMAND_TEXT,
    AK_RENDER_COMMAND_LINE,
    AK_RENDER_COMMAND_PROMPT,
    AK_RENDER_COMMAND_STATUS,
    AK_RENDER_COMMAND_TILE,
    AK_RENDER_COMMAND_CREATURE
} AkRenderCommandType;

typedef struct {
    AkRenderCommandType type;
    AkRenderMode mode;
    AkRenderColor color;
    int x;
    int y;
    int x2;
    int y2;
    int inverse;
    int value;
    char text[AK_RENDER_TEXT_SIZE];
} AkRenderCommand;

typedef struct {
    size_t count;
    int truncated;
    AkRenderCommand commands[AK_RENDER_MAX_COMMANDS];
} AkRenderCommandBuffer;

void ak_render_buffer_init(AkRenderCommandBuffer *buffer);
int ak_render_game(const AkGameState *state, AkRenderCommandBuffer *buffer);

const char *ak_render_command_name(AkRenderCommandType type);
const char *ak_render_mode_name(AkRenderMode mode);

#ifdef __cplusplus
}
#endif

#endif
