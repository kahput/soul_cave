#pragma once

#include <raylib.h>

#include <stdint.h>

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

typedef struct {
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
	Transform2D transform;

	Sprite sprite;
	CollisionShape shape;
} Object;

typedef struct {
	int32_t x, y;
} IVector2;

typedef struct {
	uint32_t count, capacity;
	Object *objects;
} Level;
