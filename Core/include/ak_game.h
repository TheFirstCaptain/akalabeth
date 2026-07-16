#ifndef AK_GAME_H
#define AK_GAME_H

#include <stddef.h>

#include "ak_random.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AK_GAME_STAT_COUNT 6
#define AK_GAME_ITEM_COUNT 6
#define AK_GAME_MAX_EVENTS 8
#define AK_GAME_OVERWORLD_SIZE 21
#define AK_GAME_DUNGEON_SIZE 11
#define AK_GAME_MONSTER_COUNT 10

typedef enum {
    AK_GAME_MODE_STARTUP_LUCKY_NUMBER = 0,
    AK_GAME_MODE_STARTUP_LEVEL,
    AK_GAME_MODE_CHARACTER_REVIEW,
    AK_GAME_MODE_CLASS_SELECT,
    AK_GAME_MODE_OVERWORLD,
    AK_GAME_MODE_TOWN,
    AK_GAME_MODE_DUNGEON,
    AK_GAME_MODE_COMBAT,
    AK_GAME_MODE_DEATH,
    AK_GAME_MODE_QUEST,
    AK_GAME_MODE_VICTORY
} AkGameMode;

typedef enum {
    AK_GAME_LOCATION_STARTUP = 0,
    AK_GAME_LOCATION_OVERWORLD,
    AK_GAME_LOCATION_TOWN,
    AK_GAME_LOCATION_DUNGEON,
    AK_GAME_LOCATION_CASTLE,
    AK_GAME_LOCATION_DEATH
} AkGameLocation;

typedef enum {
    AK_GAME_DIRECTION_NORTH = 0,
    AK_GAME_DIRECTION_EAST,
    AK_GAME_DIRECTION_SOUTH,
    AK_GAME_DIRECTION_WEST
} AkGameDirection;

typedef enum {
    AK_GAME_CLASS_NONE = 0,
    AK_GAME_CLASS_FIGHTER,
    AK_GAME_CLASS_MAGE
} AkGameClass;

typedef enum {
    AK_GAME_STAT_HIT_POINTS = 0,
    AK_GAME_STAT_STRENGTH,
    AK_GAME_STAT_DEXTERITY,
    AK_GAME_STAT_STAMINA,
    AK_GAME_STAT_WISDOM,
    AK_GAME_STAT_GOLD
} AkGameStat;

typedef enum {
    AK_GAME_ITEM_FOOD = 0,
    AK_GAME_ITEM_RAPIER,
    AK_GAME_ITEM_AXE,
    AK_GAME_ITEM_SHIELD,
    AK_GAME_ITEM_BOW,
    AK_GAME_ITEM_MAGIC_AMULET
} AkGameItem;

typedef enum {
    AK_GAME_TILE_OPEN = 0,
    AK_GAME_TILE_MOUNTAIN = 1,
    AK_GAME_TILE_FIELD = 2,
    AK_GAME_TILE_TOWN = 3,
    AK_GAME_TILE_DUNGEON = 4,
    AK_GAME_TILE_CASTLE = 5
} AkGameTile;

typedef enum {
    AK_GAME_DUNGEON_OPEN = 0,
    AK_GAME_DUNGEON_WALL = 1,
    AK_GAME_DUNGEON_TRAP = 2,
    AK_GAME_DUNGEON_SECRET_DOOR = 3,
    AK_GAME_DUNGEON_HIDDEN_DOOR = 4,
    AK_GAME_DUNGEON_CHEST = 5,
    AK_GAME_DUNGEON_LADDER_DOWN = 7,
    AK_GAME_DUNGEON_LADDER_UP = 8,
    AK_GAME_DUNGEON_LADDER_DOWN_ALT = 9
} AkGameDungeonTile;

typedef enum {
    AK_GAME_COMMAND_NONE = 0,
    AK_GAME_COMMAND_SET_LUCKY_NUMBER,
    AK_GAME_COMMAND_SET_LEVEL,
    AK_GAME_COMMAND_ACCEPT_CHARACTER,
    AK_GAME_COMMAND_REROLL_CHARACTER,
    AK_GAME_COMMAND_SELECT_CLASS,
    AK_GAME_COMMAND_MOVE_NORTH,
    AK_GAME_COMMAND_MOVE_EAST,
    AK_GAME_COMMAND_MOVE_SOUTH,
    AK_GAME_COMMAND_MOVE_WEST,
    AK_GAME_COMMAND_FORWARD,
    AK_GAME_COMMAND_TURN_LEFT,
    AK_GAME_COMMAND_TURN_RIGHT,
    AK_GAME_COMMAND_TURN_AROUND,
    AK_GAME_COMMAND_ENTER,
    AK_GAME_COMMAND_EXIT,
    AK_GAME_COMMAND_PASS,
    AK_GAME_COMMAND_BUY_ITEM,
    AK_GAME_COMMAND_ATTACK,
    AK_GAME_COMMAND_CAST_MAGIC,
    AK_GAME_COMMAND_ACKNOWLEDGE,
    AK_GAME_COMMAND_RESET
} AkGameCommandType;

typedef enum {
    AK_GAME_RESULT_OK = 0,
    AK_GAME_RESULT_INVALID_ARGUMENT,
    AK_GAME_RESULT_INVALID_COMMAND,
    AK_GAME_RESULT_REJECTED,
    AK_GAME_RESULT_UNSUPPORTED_RULE
} AkGameResultCode;

typedef enum {
    AK_GAME_EVENT_NONE = 0,
    AK_GAME_EVENT_MODE_CHANGED,
    AK_GAME_EVENT_LOCATION_CHANGED,
    AK_GAME_EVENT_PROMPT_CHANGED,
    AK_GAME_EVENT_STATE_CHANGED,
    AK_GAME_EVENT_COMMAND_ACCEPTED,
    AK_GAME_EVENT_RULE_UNIMPLEMENTED,
    AK_GAME_EVENT_ERROR
} AkGameEventType;

typedef struct {
    AkGameCommandType type;
    int value;
    AkGameItem item;
    AkGameClass player_class;
} AkGameCommand;

typedef struct {
    AkGameEventType type;
    AkGameMode mode;
    AkGameLocation location;
    AkGameCommandType command;
    int value;
} AkGameEvent;

typedef struct {
    AkGameResultCode code;
    size_t event_count;
    AkGameEvent events[AK_GAME_MAX_EVENTS];
} AkGameCommandResult;

typedef struct {
    AkGameMode mode;
    AkGameLocation location;
    AkGameDirection facing;
    AkGameClass player_class;
    int lucky_number;
    int level_of_play;
    int stats[AK_GAME_STAT_COUNT];
    int inventory[AK_GAME_ITEM_COUNT];
    int food_tenths;
    int overworld[AK_GAME_OVERWORLD_SIZE][AK_GAME_OVERWORLD_SIZE];
    int dungeon[AK_GAME_DUNGEON_SIZE][AK_GAME_DUNGEON_SIZE];
    int monster_active[AK_GAME_MONSTER_COUNT + 1];
    int monster_x[AK_GAME_MONSTER_COUNT + 1];
    int monster_y[AK_GAME_MONSTER_COUNT + 1];
    int monster_hit_points[AK_GAME_MONSTER_COUNT + 1];
    int overworld_x;
    int overworld_y;
    int dungeon_x;
    int dungeon_y;
    int dungeon_level;
    int quest_target;
    int command_count;
    AkRandom random;
} AkGameState;

void ak_game_init(AkGameState *state);
void ak_game_result_init(AkGameCommandResult *result);
AkGameResultCode ak_game_apply_command(
    AkGameState *state,
    const AkGameCommand *command,
    AkGameCommandResult *result
);

const char *ak_game_mode_name(AkGameMode mode);
const char *ak_game_location_name(AkGameLocation location);
const char *ak_game_item_name(AkGameItem item);
const char *ak_game_monster_name(int monster);
const char *ak_game_tile_name(AkGameTile tile);
const char *ak_game_dungeon_tile_name(AkGameDungeonTile tile);

#ifdef __cplusplus
}
#endif

#endif
