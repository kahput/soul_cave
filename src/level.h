#pragma once

#include "core/arena.h"

#include "object.h"

typedef struct {
	Object object;
	uint32_t tile_id;
} Tile;

typedef struct {
	uint32_t columns, rows, count;
	Tile *tiles;
} Level;

Level *level_load(Arena *arena, const char *path, const TileSheet *tile_sheet);
void level_save(const Level *level, const char *path);
