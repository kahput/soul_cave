#include "renderer.h"

#include <raylib.h>
#include <raymath.h>

#include "core/arena.h"
#include "core/logger.h"

#include "game_types.h"
#include "level.h"
#include "object.h"

#include <math.h>

static const int32_t RESOLUTION_WIDTH = 1024;
static const int32_t RESOLUTION_HEIGHT = 768;
static const int32_t SCREEN_WIDTH = 1024;
static const int32_t SCREEN_HEIGHT = 768;

static const int32_t MAX_LINE_LENGTH = 512;

static Vector2 PLAYER_SPAWN_POSITION = { .x = 3.f * TILE_SIZE * TILE_SCALE, .y = 10.f * TILE_SIZE * TILE_SCALE };
static float PLAYER_SPEED = 50.f * TILE_SCALE; // Pixels per second

// Add a GameMode enum
typedef enum {
	MODE_PLAY,
	MODE_EDIT
} GameMode;

typedef struct {
	Arena *level_arena;
	TileSheet tile_sheet;
	Object player;
	Level *level;
	GameMode mode;
	Camera2D camera;
	uint32_t held_tile;
} GameState;

void game_initialize(GameState *state);
void game_update(GameState *state, float dt);

void handle_play_mode(GameState *state, float dt);

int main(void) {
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "raylib [core] example - keyboard input");
	RenderTexture2D target = LoadRenderTexture(RESOLUTION_WIDTH, RESOLUTION_HEIGHT);
	SetTargetFPS(60);

	GameState state = { .level_arena = arena_alloc() };
	game_initialize(&state);

	while (!WindowShouldClose()) {
		float dt = GetFrameTime();

		game_update(&state, dt);

		BeginTextureMode(target);
		BeginMode2D(state.camera);
		ClearBackground(RAYWHITE);

		renderer_begin_frame((void *)0);

		for (uint32_t i = 0; i < state.level->count; i++) {
			renderer_submit(&state.level->tiles[i].object);
		}

		if (state.mode == MODE_PLAY) {
			Object render_player = state.player;
			render_player.transform.position.x = roundf(state.player.transform.position.x);
			render_player.transform.position.y = roundf(state.player.transform.position.y);

			renderer_submit(&render_player);
		}
		renderer_end_frame();
		EndMode2D();

		if (state.mode == MODE_EDIT) {
		}

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

	state->mode = MODE_PLAY;
	state->held_tile = 48;

	state->camera = (Camera2D){
		.offset = { RESOLUTION_WIDTH / 2.f, RESOLUTION_HEIGHT / 2.f },
		.rotation = 0.0f,
		.zoom = 1.f
	};

	object_populate(&state->player, PLAYER_SPAWN_POSITION, &state->tile_sheet, (IVector2){ 1, 7 }, true);
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

	state->level = level_load(state->level_arena, "./assets/levels/level_01.txt", &state->tile_sheet);
	if (state->level) {
		state->camera.target = (Vector2){
			(state->level->rows * TILE_SIZE * TILE_SCALE) / 2.f,
			(state->level->count * TILE_SIZE * TILE_SCALE) / 2.f,
		};
	}
}

void game_update(GameState *state, float dt) {
	if (IsKeyPressed(KEY_TAB)) {
		state->mode = (state->mode == MODE_PLAY) ? MODE_EDIT : MODE_PLAY;
		if (state->mode == MODE_PLAY) {
			state->player.transform.position = PLAYER_SPAWN_POSITION;
			state->camera.target = state->player.transform.position;
		}
	}

	// --- Update based on mode ---
	switch (state->mode) {
		case MODE_PLAY: {
			handle_play_mode(state, dt);
		} break;
		case MODE_EDIT: {
		} break;
	}
}

void handle_play_mode(GameState *state, float dt) {
	Vector2 direction = Vector2Normalize((Vector2){
	  .x = IsKeyDown(KEY_D) - IsKeyDown(KEY_A),
	  .y = IsKeyDown(KEY_S) - IsKeyDown(KEY_W),
	});

	Vector2 velocity = Vector2Scale(direction, PLAYER_SPEED * dt);

	state->player.transform.position = Vector2Add(state->player.transform.position, velocity);
	for (uint32_t i = 0; i < state->level->count; i++) {
		Object *tile = &state->level->tiles[i].object;

		if (tile->shape.type != COLLISION_TYPE_NONE && object_is_colliding(&state->player, tile)) {
			Rectangle player_collision_shape = object_get_collision_shape(&state->player);
			Rectangle tile_collision_shape = object_get_collision_shape(tile);

			float overlap_left = (tile_collision_shape.x + tile_collision_shape.width) - player_collision_shape.x; // move right
			float overlap_right = (player_collision_shape.x + player_collision_shape.width) - tile_collision_shape.x; // move left
			float overlap_up = (tile_collision_shape.y + tile_collision_shape.height) - player_collision_shape.y; // move down
			float overlap_down = (player_collision_shape.y + player_collision_shape.height) - tile_collision_shape.y; // move up

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

	state->camera.target = (Vector2){
		Clamp(state->player.transform.position.x, RESOLUTION_WIDTH / 2.f, RESOLUTION_WIDTH / 2.f),
		Clamp(state->player.transform.position.y, RESOLUTION_HEIGHT / 2.f, RESOLUTION_HEIGHT / 2.f),
	};
	;
}
