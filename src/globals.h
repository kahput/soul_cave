#pragma once

#include "core/arena.h"

#include <raylib.h>

#include <stdint.h>

#define LAYERS 3
#define INVALID_ID -1

static const int32_t RESOLUTION_WIDTH = 1024;
static const int32_t RESOLUTION_HEIGHT = 768;
static const int32_t SCREEN_WIDTH = 1280;
static const int32_t SCREEN_HEIGHT = 720;

static const int32_t TILE_SIZE = 16;
static const int32_t TILE_GAP = 0;
static const int32_t TILE_SCALE = 4;

#define GRID_SIZE (TILE_SIZE * TILE_SCALE)
#define EDITOR_PAN_SPEED (100.f * TILE_SCALE)

#define PLAYER_SPAWN_POSITION  (Vector2){ .x = 3.f * GRID_SIZE, .y = 10.f * GRID_SIZE }

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
	MODE_PLAY,
	MODE_EDIT
} GameMode;

typedef struct {
	Arena *level_arena;
	SpriteSheet tile_sheet, player_sheet;
	Object player;
	Level *level;
	GameMode mode;
	Camera2D camera;
	int32_t current_tile, current_layer;
} GameState;
