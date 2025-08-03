#include "level.h"

#include "core/arena.h"
#include "core/logger.h"

#include "globals.h"
#include "object.h"
#include "renderer.h"

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 512

void level_draw(GameState *state) {
	for (uint32_t i = 0; i < LAYERS; i++) {
		for (uint32_t j = 0; j < state->level->count; j++) {
			Tile *tile = state->level->tiles[i] + j;

			if (tile->tile_id != INVALID_ID) {
				if (tile->tile_id == PUSHABLE_TILE) {
					if (state->mode == MODE_PLAY) {
						float tile_sort = tile->object.transform.position.y +
							tile->object.sprite.transform.position.y +
							(tile->object.sprite.src.height * tile->object.sprite.transform.scale.y * tile->object.transform.scale.y);
						// DrawCircle(tile->object.transform.position.x + tile->object.sprite.transform.position.x, tile_sort, 10.f, ORANGE);
						float player_sort = state->player.transform.position.y + state->player.sprite.transform.position.y;
						// DrawCircle(state->player.transform.position.x + state->player.sprite.transform.position.x, player_sort, 10.f, ORANGE);
						if (player_sort < tile_sort)
							continue;
					}

					uint32_t grid_x = tile->tile_id % state->tile_sheet.columns;
					uint32_t grid_y = tile->tile_id / state->tile_sheet.columns;
					Object pillar_top = { 0 };
					Vector2 position = {
						.x = tile->object.transform.position.x,
						.y = tile->object.transform.position.y - GRID_SIZE
					};
					object_populate(&pillar_top, position, &state->tile_sheet, (IVector2){ grid_x, grid_y - 1 }, false);
					renderer_submit(&pillar_top);
				}
				renderer_submit(&tile->object);
			}
		}
	}
}

// Alternative parsing function that avoids strdup/free
static void parse_tile_layers(const char *token, char layers[LAYERS][32]) {
	const char *start = token;
	uint32_t layer = 0;
	uint32_t pos = 0;

	// Initialize all layers to empty strings
	for (uint32_t i = 0; i < LAYERS; i++) {
		layers[i][0] = '\0';
	}

	while (*start && layer < LAYERS) {
		const char *end = strchr(start, ':');
		size_t len;

		if (end) {
			len = end - start;
		} else {
			len = strlen(start);
		}

		if (len < 31) { // Leave room for null terminator
			strncpy(layers[layer], start, len);
			layers[layer][len] = '\0';
		}

		layer++;
		if (end) {
			start = end + 1;
		} else {
			break;
		}
	}
}

Level *level_load(Arena *arena, const char *path, const SpriteSheet *tile_sheet) {
	Level *level = arena_push_type(arena, Level);
	FILE *file;
	if ((file = fopen(path, "r")) == NULL) {
		LOG_ERROR("FILE: %s", strerror(errno));
		return NULL;
	}

	char buffer[MAX_LINE_LENGTH];
	uint32_t max_file_line = 0, max_file_column = 0;

	// First pass: determine dimensions
	for (; fgets(buffer, sizeof(buffer), file); max_file_line++) {
		char *token = strtok(buffer, " \t\r\n");
		uint32_t column_count = 0;
		while (token) {
			column_count++;
			token = strtok(NULL, " \t\r\n");
		}
		max_file_column = column_count > max_file_column ? column_count : max_file_column;
	}

	rewind(file);
	level->columns = max_file_column;
	level->rows = max_file_line;
	level->count = level->capcity = level->columns * level->rows;

	for (uint32_t i = 0; i < LAYERS; i++)
		level->tiles[i] = arena_push_array_zero(arena, Tile, level->columns * level->rows);

	// Second pass: parse the data
	for (uint32_t y = 0; fgets(buffer, sizeof(buffer), file); y++) {
		char *token = strtok(buffer, " \t\r\n");
		uint32_t x = 0;

		while (token && x < level->columns) {
			uint32_t index = x + y * level->columns;

			// Parse layers from the token
			char layers[LAYERS][32];
			parse_tile_layers(token, layers);

			// Process each layer
			for (uint32_t layer = 0; layer < LAYERS; layer++) {
				int32_t texture_id = INVALID_ID;

				if (layers[layer][0] != '\0') {
					texture_id = atoi(layers[layer]);
					if (texture_id >= (int32_t)(tile_sheet->columns * tile_sheet->rows)) {
						LOG_WARN("LEVEL: Token [%d, %d] layer %d has invalid value %d", x, y, layer, texture_id);
						texture_id = INVALID_ID;
					}
				}

				Tile *tile = level->tiles[layer] + index;
				tile->tile_id = texture_id;

				if (tile->tile_id != INVALID_ID) {
					IVector2 texture_offset = {
						.x = texture_id % tile_sheet->columns,
						.y = texture_id / tile_sheet->columns,
					};
					object_populate(&tile->object,
						(Vector2){ x * tile_sheet->tile_size * TILE_SCALE,
						  y * tile_sheet->tile_size * TILE_SCALE },
						tile_sheet, texture_offset, false);
					if (layer % 2 == 0)
						tile->object.shape.type = COLLISION_TYPE_NONE;
				}
			}

			token = strtok(NULL, " \t\r\n");
			x++;
		}

		// Fill remaining columns with invalid tiles if line was shorter
		while (x < level->columns) {
			uint32_t index = x + y * level->columns;
			for (uint32_t layer = 0; layer < LAYERS; layer++) {
				level->tiles[layer][index].tile_id = INVALID_ID;
			}
			x++;
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
			for (uint32_t layer = 0; layer < LAYERS; ++layer) {
				uint32_t index = x + y * level->columns;
				int32_t saved_tile = level->tiles[layer][index].tile_id;
				fprintf(file, "%d", saved_tile);
				if (layer < LAYERS - 1) {
					fprintf(file, ":");
				}
			}
			if (x < level->columns - 1) {
				fprintf(file, " ");
			}
		}
		fprintf(file, "\n");
	}

	fclose(file);
	LOG_INFO("LEVEL: Saved level to %s", path);
}
