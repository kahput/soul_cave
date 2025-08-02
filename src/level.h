#pragma once

#include "core/arena.h"

#include "object.h"

#define LAYERS 3
#define INVALID_ID -1

typedef struct {
	Object object;
	int32_t tile_id;
} Tile;

typedef struct {
	uint32_t columns, rows;

	uint32_t capcity, count;
	Tile *tiles[LAYERS];
} Level;

Level *level_load(Arena *arena, const char *path, const TileSheet *tile_sheet);
void level_save(const Level *level, const char *path);
