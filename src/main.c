#include "renderer.h"

#include <math.h>
#include <raylib.h>
#include <raymath.h>

#include "core/arena.h"
#include "core/logger.h"

#include "game_types.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const int32_t RESOLUTION_WIDTH = 256;
static const int32_t RESOLUTION_HEIGHT = 192;
static const int32_t SCREEN_WIDTH = 1024;
static const int32_t SCREEN_HEIGHT = 768;

static const int32_t TILE_SIZE = 16;
static const int32_t TILE_GAP = 1;
static const int32_t TILE_SCALE = 1;

static const int32_t MAX_LINE_LENGTH = 512;

static float PLAYER_SPEED = 50.0f; // Pixels per second

typedef struct {
	Arena *level_arena;
} GameState;

bool aabb_collision(Object *a, Object *b);
Level *game_load_level(Arena *arena, const char *path, const TileSheet *tile_sheet, uint32_t level_width, uint32_t level_height);
void object_populate(Object *object, Vector2 position, const TileSheet *tile_sheet, IVector2 texture_offset, bool centered);
Rectangle get_collision_shape(Object *object);

int main(void) {
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "raylib [core] example - keyboard input");
	RenderTexture2D target = LoadRenderTexture(RESOLUTION_WIDTH, RESOLUTION_HEIGHT);
	SetTargetFPS(60);

	Texture texture = LoadTexture("./assets/tiles/tilemap.png");
	TileSheet tile_sheet = {
		.texture = texture,
		.tile_size = TILE_SIZE,
		.gap = TILE_GAP,
		.columns = (texture.width + TILE_GAP) / (TILE_SIZE + TILE_GAP),
		.rows = (texture.height + TILE_GAP) / (TILE_SIZE + TILE_GAP),
	};

	GameState state = { .level_arena = arena_alloc() };

	Object player = { 0 };
	object_populate(&player, (Vector2){ .x = TILE_SIZE, TILE_SIZE }, &tile_sheet, (IVector2){ 1, 7 }, true);
	player.sprite.origin.y = player.sprite.src.height;
	player.shape = (CollisionShape){
		.transform = {
			.position = {
				.x = -player.sprite.src.width / 2.f * player.transform.scale.x,
				.y = -player.sprite.src.height / 2.f * player.transform.scale.y,
			},
			.scale = { 1.f, 1.f },
		},
		.width = TILE_SIZE,
		.height = TILE_SIZE / 2.f
	};

	Level *level = game_load_level(state.level_arena, "./assets/levels/level_01.txt", &tile_sheet, SCREEN_WIDTH, SCREEN_HEIGHT);

	while (!WindowShouldClose()) {
		float dt = GetFrameTime();

		Vector2 direction = Vector2Normalize((Vector2){
			.x = IsKeyDown(KEY_D) - IsKeyDown(KEY_A),
			.y = IsKeyDown(KEY_S) - IsKeyDown(KEY_W),
		});

		Vector2 velocity = Vector2Scale(direction, PLAYER_SPEED * dt);

		player.transform.position = Vector2Add(player.transform.position, velocity);
		for (uint32_t i = 0; i < level->count; i++) {
			Object *tile = level->objects + i;

			if (aabb_collision(&player, tile)) {
				Rectangle player_shape = get_collision_shape(&player);
				Rectangle tile_shape = get_collision_shape(tile);

				float overlap_left = (tile_shape.x + tile_shape.width) - player_shape.x; // move right
				float overlap_right = (player_shape.x + player_shape.width) - tile_shape.x; // move left
				float overlap_up = (tile_shape.y + tile_shape.height) - player_shape.y; // move down
				float overlap_down = (player_shape.y + player_shape.height) - tile_shape.y; // move up

				float min_overlap_x = fminf(overlap_left, overlap_right);
				float min_overlap_y = fminf(overlap_up, overlap_down);

				// choose smaller overlap
				Vector2 penetration;
				if (min_overlap_x < min_overlap_y)
					penetration = (overlap_left < overlap_right) ? (Vector2){ overlap_left, 0 } : (Vector2){ -overlap_right, 0 };
				else
					penetration = (overlap_up < overlap_down) ? (Vector2){ 0, overlap_up } : (Vector2){ 0, -overlap_down };

				player.transform.position.x += penetration.x;
				player.transform.position.y += penetration.y;
			}
		}

		BeginTextureMode(target);
		ClearBackground(RAYWHITE);

		renderer_begin_frame((void *)0);

		for (uint32_t i = 0; i < level->count; i++) {
			renderer_submit(level->objects + i);
		}

		Object render_player = player;
		render_player.transform.position.x = roundf(player.transform.position.x);
		render_player.transform.position.y = roundf(player.transform.position.y);

		renderer_submit(&render_player);
		renderer_end_frame();

		EndTextureMode();

		BeginDrawing();
		ClearBackground(BLACK);

		float scale = fminf((float)SCREEN_WIDTH / RESOLUTION_WIDTH,
			(float)SCREEN_HEIGHT / RESOLUTION_HEIGHT);

		Rectangle source = {
			.x = 0.0f,
			.y = 0.0f,
			.width = RESOLUTION_WIDTH,
			.height = -RESOLUTION_HEIGHT,
		};
		Rectangle dest = {
			.x = (SCREEN_WIDTH - (RESOLUTION_WIDTH * scale)) / 2.f,
			.y = (SCREEN_HEIGHT - (RESOLUTION_HEIGHT * scale)) / 2.f,
			.width = RESOLUTION_WIDTH * scale,
			.height = RESOLUTION_HEIGHT * scale,
		};

		DrawTexturePro(target.texture, source, dest, (Vector2){ 0 }, 0.0f, WHITE);
		EndDrawing();
	}

	CloseWindow();

	return 0;
}

Rectangle get_collision_shape(Object *object) {
	return (Rectangle){
		.x = object->transform.position.x + object->shape.transform.position.x,
		.y = object->transform.position.y + object->shape.transform.position.y,
		.width = object->shape.width * object->shape.transform.scale.x * object->transform.scale.x,
		.height = object->shape.height * object->shape.transform.scale.y * object->transform.scale.y,
	};
}

bool aabb_collision(Object *a, Object *b) {
	Rectangle collision_shape_a = get_collision_shape(a);
	Rectangle collision_shape_b = get_collision_shape(b);

	return CheckCollisionRecs(collision_shape_a, collision_shape_b);
}

void object_populate(Object *object, Vector2 position, const TileSheet *tile_sheet, IVector2 texture_offset, bool centered) {
	*object = (Object){
		.transform = {
			.position = position,
			.scale = { TILE_SCALE, TILE_SCALE },
			.rotation = 0.0f,
		},
		.sprite = {
			.transform = {
				.scale = {
					.x = 1.f,
					.y = 1.f,
				},
			},
			.texture = tile_sheet->texture,
			.src = {
				.x = (float)(tile_sheet->tile_size + tile_sheet->gap) * texture_offset.x,
				.y = (float)(tile_sheet->tile_size + tile_sheet->gap) * texture_offset.y,
				.width = tile_sheet->tile_size,
				.height = tile_sheet->tile_size,
			},
			.origin = {
				.x = 0.f,
				.y = 0.f,
			},
		},
	};

	object->shape = (CollisionShape){
		.transform = {
			.scale = {
				.x = 1.f,
				.y = 1.f,
			},
		},
		.width = object->sprite.src.width,
		.height = object->sprite.src.height,
	};

	if (centered) {
		object->sprite.origin = (Vector2){
			.x = object->sprite.src.width / 2.f,
			.y = object->sprite.src.height / 2.f,
		};
		object->shape.transform.position = (Vector2){
			.x = -object->shape.width / 2.f * object->transform.scale.x,
			.y = -object->shape.height / 2.f * object->transform.scale.y,
		};

	} else {
		object->sprite.origin = (Vector2){
			.x = 0.0f,
			.y = 0.0f,
		};
		object->shape.transform.position = (Vector2){
			.x = 0.0f,
			.y = 0.0f,
		};
	}
}

Level *game_load_level(Arena *arena, const char *path, const TileSheet *tile_sheet, uint32_t level_width, uint32_t level_height) {
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

	level->capacity = max_file_line * max_file_column;
	level->count = 0;
	level->objects = arena_push_array_zero(arena, Object, level->capacity);

	for (uint32_t y = 0; fgets(buffer, sizeof(buffer), file); y++) {
		char *token = strtok(buffer, " \t\r\n");

		for (uint32_t x = 0; x < max_file_column; x++) {
			uint32_t index = x + y * max_file_column;
			if (token == NULL) {
				LOG_WARN("LEVEL: Token [%d, %d] missing", x, y);
				token = strtok(NULL, " \t\r\n");
				continue;
			}

			else if (isdigit(*token) == 0 || atoi(token) >= (int32_t)(tile_sheet->columns * tile_sheet->rows)) {
				LOG_WARN("LEVEL: Token [%d, %d] is invalid value", x, y);
				token = strtok(NULL, " \t\r\n");
				continue;
			}

			uint8_t x_off = atoi(token) % tile_sheet->columns;
			uint8_t y_off = atoi(token) / tile_sheet->columns;

			object_populate(&level->objects[level->count++], (Vector2){ x * tile_sheet->tile_size * TILE_SCALE, y * tile_sheet->tile_size * TILE_SCALE }, tile_sheet, (IVector2){ x_off, y_off }, false);
			token = strtok(NULL, " \t\r\n");
		}
	}

	fclose(file);

	return level;
}
