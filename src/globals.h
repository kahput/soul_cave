#pragma once

#include "core/arena.h"

#include <raylib.h>

#include <stdint.h>

#define LAYERS 6
#define INVALID_ID -1

static const int32_t RESOLUTION_WIDTH = 1024;
static const int32_t RESOLUTION_HEIGHT = 768;
static const int32_t SCREEN_WIDTH = 1280;
static const int32_t SCREEN_HEIGHT = 720;

static const int32_t TILE_SIZE = 16;
static const int32_t TILE_GAP = 0;
static const int32_t TILE_SCALE = 4;

static uint32_t MAX_LEVELS = 3;

#define GRID_SIZE (TILE_SIZE * TILE_SCALE)
#define EDITOR_PAN_SPEED (100.f * TILE_SCALE)

#define LEVEL_MAX_COLUMNS 24
#define LEVEL_MAX_ROWS 24

#define PLAYER_SPAWN_POSITION \
	(Vector2) { .x = 12.f * GRID_SIZE, .y = 23.f * GRID_SIZE }

#define PUSHABLE_TILE 16
#define ORB_TILE 32
#define PRESSURE_PLATE_TILE 24
#define RIGHT_PORTAL_TILE 35
#define LEFT_PORTAL_TILE 14

typedef struct {
	uint32_t rows, columns;
	uint32_t tile_size, gap;

	Texture texture;
} SpriteSheet;

typedef struct {
	Vector2 position;
	float rotation;
	Vector2 scale;
} Transform2D;

typedef enum {
	COLLISION_TYPE_NONE,
	COLLISION_TYPE_RECTANGLE,
	COLLISION_TYPE_CIRCLE,
	COLLISION_TYPE_TRIANGLE,

	COLLISION_TYPE_COUNT
} CollisionType;

typedef struct {
	CollisionType type;

	Transform2D transform;
	union {
		struct {
			float width;
			float height;
		};
	};

} CollisionShape;

typedef struct {
	Transform2D transform;
	Rectangle src;
	Texture texture;
	Vector2 origin;
} Sprite;

typedef struct {
	int32_t x, y;
} IVector2;

typedef struct {
	Transform2D transform;

	Sprite sprite;
	CollisionShape shape;
} Object;

typedef struct {
	Object object;
	int32_t tile_id;
} Tile;

typedef struct {
	uint32_t columns, rows;

	uint32_t capcity, count;
	Tile *tiles[LAYERS];
} Level;

// Add a GameMode enum
typedef enum {
	MODE_NONE,
	MODE_PLAY,
	MODE_EDIT,
	MODE_TRANSITION,

	MODE_COUNT,
} GameMode;

// Add these to your GameState struct (probably in a header file)
typedef enum {
	TRANSITION_NONE,
	TRANSITION_FADE_OUT,
	TRANSITION_PAUSE_MESSAGE,
	TRANSITION_FADE_IN
} TransitionPhase;

typedef struct {
	TransitionPhase phase;
	float timer;
	float fade_duration;
	float message_duration;
	char message[256];
	uint32_t next_level;
} TransitionState;

// Better approach: Load sounds once during initialization instead of every time you play them
// Add these to your GameState struct:
typedef struct {
	Sound pillar_push;
	Sound click;
	Sound level_complete;

	Music background_music;
	// Add more sounds as needed
} GameSounds;

typedef struct {
	Arena *level_arena;
	SpriteSheet tile_sheet, player_sheet;

	GameSounds sounds;

	Object player;
	uint32_t pressure_plate_count, actived_pressure_plate_count;
	float player_light_radius;

	Level *level;
	uint32_t num_level;

	GameMode mode;
	Camera2D camera;
	int32_t current_tile, current_layer;

	TransitionState transition;
} GameState;
