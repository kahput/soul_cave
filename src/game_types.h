#pragma once

#include <raylib.h>

#include <stdint.h>

static const int32_t TILE_SIZE = 16;
static const int32_t TILE_GAP = 1;
static const int32_t TILE_SCALE = 4;

typedef struct {
	uint32_t rows, columns;
	uint32_t tile_size, gap;

	Texture texture;
} TileSheet;

typedef struct {
	Vector2 position;
	float rotation;
	Vector2 scale;
} Transform2D;

typedef enum {
	COLLISION_TYPE_NONE,
	COLLISION_TYPE_RECTANGLE,
	COLLISION_TYPE_CIRCLE,

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
