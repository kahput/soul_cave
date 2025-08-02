#include "level.h"

#include "core/arena.h"
#include "core/logger.h"

#include "object.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const int32_t MAX_LINE_LENGTH = 512;


Level *level_load(Arena *arena, const char *path, const SpriteSheet *tile_sheet) {
	Level *level = arena_push_type(arena, Level);

	FILE *file;
	if ((file = fopen(path, "r")) == NULL) {
		LOG_ERROR("FILE: %s", strerror(errno));
		return NULL;
	}

	char buffer[MAX_LINE_LENGTH];
	uint32_t max_file_line = 0, max_file_column = 0;

	for (; fgets(buffer, sizeof(buffer), file); max_file_line++) {
		char *token = strtok(buffer, " \t\r\n");

		for (uint32_t x = 0; token; x++) {
			max_file_column = x == max_file_column ? x + 1 : max_file_column;

			// LOG_INFO("Token [%d, %d]: %s", x, max_file_line, token);
			token = strtok(NULL, " \t\r\n");
		}
	}

	rewind(file);

	level->columns = max_file_column, level->rows = max_file_line;
	level->count = level->capcity = level->columns * level->rows;
	for (uint32_t i = 0; i < LAYERS; i++)
		level->tiles[i] = arena_push_array_zero(arena, Tile, level->columns * level->rows);

	for (uint32_t y = 0; fgets(buffer, sizeof(buffer), file); y++) {
		char *token = strtok(buffer, " \t\r\n");

		for (uint32_t x = 0; x < max_file_column; x++) {
			uint32_t index = x + y * max_file_column;
			if (token == NULL) {
				LOG_WARN("LEVEL: Token [%d, %d] missing", x, y);
				token = strtok(NULL, " \t\r\n");
				continue;
			}

			int32_t texture_id = 0;
			if (isdigit(*token) == 0 || (texture_id = atoi(token)) >= (int32_t)(tile_sheet->columns * tile_sheet->rows)) {
				LOG_WARN("LEVEL: Token [%d, %d] is invalid value", x, y);
				token = strtok(NULL, " \t\r\n");
				continue;
			}

			IVector2 texture_offset = {
				.x = texture_id % tile_sheet->columns,
				.y = texture_id / tile_sheet->columns,
			};

			for (uint32_t i = 0; i < LAYERS; i++) {
				Tile *tile = level->tiles[i] + index;

				if (i == 0) {
					tile->tile_id = texture_id;
					object_populate(&tile->object, (Vector2){ x * tile_sheet->tile_size * TILE_SCALE, y * tile_sheet->tile_size * TILE_SCALE }, tile_sheet, texture_offset, false);
					tile->object.shape.type = COLLISION_TYPE_NONE;
				} else
					tile->tile_id = INVALID_ID;
			}

			token = strtok(NULL, " \t\r\n");
		}
	}

	fclose(file);

	return level;
}

void level_save(const Level *level, const char *path) {
	FILE *file = fopen(path, "w");
	if (file == NULL) {
		LOG_ERROR("FILE: %s", strerror(errno));
		return;
	}

	for (uint32_t y = 0; y < level->rows; y++) {
		for (uint32_t x = 0; x < level->columns; x++) {
			uint32_t index = x + y * level->columns;
			fprintf(file, "%d", level->tiles[0][index].tile_id);
			if (x < level->columns - 1) {
				fprintf(file, " ");
			}
		}
		fprintf(file, "\n");
	}

	fclose(file);
	LOG_INFO("LEVEL: Saved level to %s", path);
}
