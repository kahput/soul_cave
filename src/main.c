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

static const int32_t RESOLUTION_WIDTH = 1024;
static const int32_t RESOLUTION_HEIGHT = 768;
static const int32_t SCREEN_WIDTH = 1024;
static const int32_t SCREEN_HEIGHT = 768;

static const int32_t TILE_SIZE = 16;
static const int32_t TILE_GAP = 1;
static const int32_t TILE_SCALE = 4;

static const int32_t MAX_LINE_LENGTH = 512;

static float PLAYER_SPEED = 50.f * TILE_SCALE; // Pixels per second

typedef struct {
	Arena *level_arena;

	TileSheet tile_sheet;
	Object player;
	Level *level;

} GameState;

bool aabb_collision(Object *a, Object *b);
Level *game_load_level(Arena *arena, const char *path, const TileSheet *tile_sheet);

void game_initialize(GameState *state);
void game_update(GameState *state, float dt);

void object_populate(Object *object, Vector2 position, const TileSheet *tile_sheet, IVector2 texture_offset, bool centered);
Rectangle get_collision_shape(Object *object);

int main(void) {
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "raylib [core] example - keyboard input");
	RenderTexture2D target = LoadRenderTexture(RESOLUTION_WIDTH, RESOLUTION_HEIGHT);
	SetTargetFPS(60);

	GameState state = { .level_arena = arena_alloc() };
	game_initialize(&state);

	// Camera2D camera = { 0 };
	// camera.target = (Vector2){ state.player.transform.position.x + TILE_SIZE * TILE_SCALE, state.player.transform.position.y + TILE_SIZE * TILE_SCALE };
	// camera.offset = (Vector2){ RESOLUTION_WIDTH / 2.0f, RESOLUTION_HEIGHT / 2.0f };
	// camera.rotation = 0.0f;
	// camera.zoom = 1.0f;

	while (!WindowShouldClose()) {
		float dt = GetFrameTime();

		game_update(&state, dt);
		// camera.target = (Vector2){ state.player.transform.position.x + TILE_SIZE * TILE_SCALE, state.player.transform.position.y + TILE_SIZE * TILE_SCALE };

		BeginTextureMode(target);
		// BeginMode2D(camera);
		ClearBackground(RAYWHITE);

		renderer_begin_frame((void *)0);

		for (uint32_t i = 0; i < state.level->count; i++) {
			renderer_submit(state.level->objects + i);
		}

		Object render_player = state.player;
		render_player.transform.position.x = roundf(state.player.transform.position.x);
		render_player.transform.position.y = roundf(state.player.transform.position.y);

		renderer_submit(&render_player);
		renderer_end_frame();

		// EndMode2D();
		EndTextureMode();

		BeginDrawing();
		ClearBackground(BLACK);

		float scale = fminf((float)SCREEN_WIDTH / RESOLUTION_WIDTH, (float)SCREEN_HEIGHT / RESOLUTION_HEIGHT);

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

void game_initialize(GameState *state) {
	Texture texture = LoadTexture("./assets/tiles/tilemap.png");
	state->tile_sheet = (TileSheet){
		.texture = texture,
		.tile_size = TILE_SIZE,
		.gap = TILE_GAP,
		.columns = (texture.width + TILE_GAP) / (TILE_SIZE + TILE_GAP),
		.rows = (texture.height + TILE_GAP) / (TILE_SIZE + TILE_GAP),
	};

	object_populate(&state->player, (Vector2){ .x = TILE_SIZE, TILE_SIZE }, &state->tile_sheet, (IVector2){ 1, 7 }, true);
	state->player.sprite.origin.y = state->player.sprite.src.height;
	state->player.shape = (CollisionShape){
		.type = COLLISION_TYPE_RECTANGLE,
		.transform = {
		  .position = {
			.x = -state->player.sprite.src.width / 2.f * state->player.transform.scale.x,
			.y = -state->player.sprite.src.height / 2.f * state->player.transform.scale.y,
		  },
		  .scale = { 1.f, 1.f },
		},
		.width = TILE_SIZE,
		.height = TILE_SIZE / 2.f
	};

	state->level = game_load_level(state->level_arena, "./assets/levels/level_01.txt", &state->tile_sheet);
}

void game_update(GameState *state, float dt) {
	Vector2 direction = Vector2Normalize((Vector2){
	  .x = IsKeyDown(KEY_D) - IsKeyDown(KEY_A),
	  .y = IsKeyDown(KEY_S) - IsKeyDown(KEY_W),
	});

	Vector2 velocity = Vector2Scale(direction, PLAYER_SPEED * dt);

	state->player.transform.position = Vector2Add(state->player.transform.position, velocity);
	for (uint32_t i = 0; i < state->level->count; i++) {
		Object *tile = state->level->objects + i;

		if (tile->shape.type != COLLISION_TYPE_NONE && aabb_collision(&state->player, tile)) {
			Rectangle player_shape = get_collision_shape(&state->player);
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

			state->player.transform.position.x += penetration.x;
			state->player.transform.position.y += penetration.y;
		}
	}
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
		.type = COLLISION_TYPE_RECTANGLE,
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

Level *game_load_level(Arena *arena, const char *path, const TileSheet *tile_sheet) {
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

			Object *tile = level->objects + level->count++;

			if (texture_id == 48) {
				bool variant = GetRandomValue(0, 10) == 0;
				texture_offset.x += variant;

				object_populate(tile, (Vector2){ x * tile_sheet->tile_size * TILE_SCALE, y * tile_sheet->tile_size * TILE_SCALE }, tile_sheet, texture_offset, false);
				tile->shape.type = COLLISION_TYPE_NONE;
			} else
				object_populate(tile, (Vector2){ x * tile_sheet->tile_size * TILE_SCALE, y * tile_sheet->tile_size * TILE_SCALE }, tile_sheet, texture_offset, false);

			token = strtok(NULL, " \t\r\n");
		}
	}

	fclose(file);

	return level;
}
