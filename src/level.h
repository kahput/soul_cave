#pragma once

#include "core/arena.h"

#include "globals.h"

Level *level_load(Arena *arena, const char *path, const SpriteSheet *tile_sheet);
void level_save(const Level *level, const char *path);
