#include "renderer.h"

#include <raylib.h>
#include <raymath.h>

#include "core/arena.h"
#include "core/logger.h"

#include "globals.h"
#include "level.h"
#include "object.h"
#include "player.h"

#include <math.h>
#include <stdio.h>

Vector2 mouse_screen_to_world(Camera2D *camera);

void game_initialize(GameState *state);
void game_update(GameState *state, float dt);

void handle_play_mode(GameState *state, float dt);
void handle_edit_mode(GameState *state, float dt);

void draw_tiles(GameState *state);
void draw_editor_ui(GameState *state);

int main(void) {
	InitWindow(RESOLUTION_WIDTH, RESOLUTION_HEIGHT, "raylib [core] example - keyboard input");
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
		level_draw(&state);

		if (state.mode == MODE_EDIT) {
			Vector2 mouse_world = mouse_screen_to_world(&state.camera);

			uint32_t grid_x = Clamp((float)mouse_world.x / (float)GRID_SIZE, 0.0f, state.level->columns - 1);
			uint32_t grid_y = Clamp((float)mouse_world.y / (float)GRID_SIZE, 0.0f, state.level->rows - 1);

			DrawRectangleLinesEx((Rectangle){ grid_x * GRID_SIZE, grid_y * GRID_SIZE, GRID_SIZE, GRID_SIZE },
				1.f * TILE_SCALE, BLACK);
		}

		if (state.mode == MODE_PLAY) {
			Object render_player = state.player;
			render_player.transform.position.x = roundf(state.player.transform.position.x);
			render_player.transform.position.y = roundf(state.player.transform.position.y);

			renderer_submit(&render_player);
			for (uint32_t i = 1; i < LAYERS; i++) {
				for (uint32_t j = 0; j < state.level->count; j++) {
					Tile *tile = state.level->tiles[i] + j;

					if (tile->tile_id == PUSHABLE_TILE) {
						float tile_sort = tile->object.transform.position.y +
							tile->object.sprite.transform.position.y +
							(tile->object.sprite.src.height * tile->object.sprite.transform.scale.y * tile->object.transform.scale.y);
						float player_sort = state.player.transform.position.y +
							state.player.sprite.transform.position.y;
						if (player_sort >= tile_sort)
							continue;

						if (tile->tile_id == PUSHABLE_TILE) {
							uint32_t grid_x = tile->tile_id % state.tile_sheet.columns;
							uint32_t grid_y = tile->tile_id / state.tile_sheet.columns;
							Object pillar_top = { 0 };
							Vector2 position = {
								.x = tile->object.transform.position.x,
								.y = tile->object.transform.position.y - GRID_SIZE
							};
							object_populate(&pillar_top, position, &state.tile_sheet, (IVector2){ grid_x, grid_y - 1 }, false);
							renderer_submit(&pillar_top);
						}
						renderer_submit(&tile->object);
					}
				}
			}
		}
		renderer_end_frame();
		EndMode2D();

		EndTextureMode();

		BeginDrawing();
		ClearBackground(BLACK);
		if (state.mode == MODE_EDIT) {
			draw_editor_ui(&state);
		}

		float scale = fminf((float)GetScreenWidth() / RESOLUTION_WIDTH, (float)GetScreenHeight() / RESOLUTION_HEIGHT);

		Rectangle source = {
			.x = 0.0f,
			.y = 0.0f,
			.width = RESOLUTION_WIDTH,
			.height = -RESOLUTION_HEIGHT,
		};
		Rectangle dest = {
			.x = 0.0f,
			.y = (GetScreenHeight() - (RESOLUTION_HEIGHT * scale)) / 2.f,
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
	Texture tile_sheet = LoadTexture("./assets/tiles/Map_Tilesheet.png");
	state->tile_sheet = (SpriteSheet){
		.texture = tile_sheet,
		.tile_size = TILE_SIZE,
		.gap = TILE_GAP,
		.columns = (tile_sheet.width + TILE_GAP) / (TILE_SIZE + TILE_GAP),
		.rows = (tile_sheet.height + TILE_GAP) / (TILE_SIZE + TILE_GAP),
	};
	Texture player_sheet = LoadTexture("./assets/tiles/Eidolon_Sheet.png");
	state->player_sheet = (SpriteSheet){
		.texture = player_sheet,
		.tile_size = 32,
		.gap = TILE_GAP,
		.columns = (player_sheet.width + TILE_GAP) / (32 + TILE_GAP),
		.rows = (player_sheet.height + TILE_GAP) / (32 + TILE_GAP),
	};

	state->mode = MODE_PLAY;
	state->current_tile = 25, state->current_layer = 0;

	state->camera = (Camera2D){
		.offset = { RESOLUTION_WIDTH / 2.f, RESOLUTION_HEIGHT / 2.f },
		.rotation = 0.0f,
		.zoom = 1.f
	};

	player_initialize(state);

	state->level = level_load(state->level_arena, "./assets/levels/level_01.txt", &state->tile_sheet);
	if (state->level) {
		state->camera.target = (Vector2){
			(state->level->rows * GRID_SIZE) / 2.f,
			(state->level->count * GRID_SIZE) / 2.f,
		};
	}
}

void game_update(GameState *state, float dt) {
	if (IsKeyPressed(KEY_TAB)) {
		state->mode = (state->mode == MODE_PLAY) ? MODE_EDIT : MODE_PLAY;
		if (state->mode == MODE_PLAY) {
			state->player.transform.position = PLAYER_SPAWN_POSITION;
			state->camera.target = state->player.transform.position;
			state->camera.zoom = 1.f;
			SetWindowSize(RESOLUTION_WIDTH, RESOLUTION_HEIGHT);
		} else
			SetWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
	}

	// --- Update based on mode ---
	switch (state->mode) {
		case MODE_PLAY: {
			handle_play_mode(state, dt);
		} break;
		case MODE_EDIT: {
			handle_edit_mode(state, dt);
		} break;
	}
}

void handle_play_mode(GameState *state, float dt) {
	player_update(state, dt);
}

void handle_edit_mode(GameState *state, float dt) {
	/// Camera Zoom
	if (IsKeyDown(KEY_LEFT_CONTROL))
		state->camera.zoom = expf(logf(state->camera.zoom) + ((float)GetMouseWheelMove() * 0.1f));

	if (state->camera.zoom > 3.0f)
		state->camera.zoom = 3.0f;
	else if (state->camera.zoom < 0.1f)
		state->camera.zoom = 0.1f;

	// Camera Panning
	Vector2 delta = GetMouseWheelMoveV();
	delta = Vector2Scale(delta, -1);
	if (delta.x != 0.0f || delta.y != 0.0f) {
		state->camera.target = Vector2Add(state->camera.target, Vector2Scale(delta, EDITOR_PAN_SPEED * dt / state->camera.zoom));
	}
	// --- Optional: WASD panning ---
	Vector2 pan_direction = {
		.x = (float)(IsKeyDown(KEY_D) - IsKeyDown(KEY_A)),
		.y = (float)(IsKeyDown(KEY_S) - IsKeyDown(KEY_W))
	};
	if (pan_direction.x != 0 || pan_direction.y != 0) {
		pan_direction = Vector2Normalize(pan_direction);
		state->camera.target = Vector2Add(
			state->camera.target,
			Vector2Scale(pan_direction, EDITOR_PAN_SPEED * dt / state->camera.zoom));
	}

	for (uint32_t key = KEY_ONE; key < LAYERS + KEY_ONE; key++) {
		if (IsKeyPressed(key))
			state->current_layer = key - KEY_ONE;
	}

	// // --- Tile Selection from Palette ---
	float scale = fminf((float)GetScreenWidth() / RESOLUTION_WIDTH, (float)GetScreenHeight() / RESOLUTION_HEIGHT);
	Rectangle palette_rect = {
		.x = RESOLUTION_WIDTH * scale,
		.y = 0,
		.width = GetScreenWidth() - (RESOLUTION_WIDTH * scale),
		.height = RESOLUTION_HEIGHT * scale,
	};
	Vector2 mouse_position = GetMousePosition();

	if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && mouse_position.x > palette_rect.x && mouse_position.x < palette_rect.x + palette_rect.width - 20) {
		uint32_t palette_unit_size = 32;
		uint32_t palette_wrap = ((palette_rect.width - 20) / palette_unit_size);
		uint32_t tile_x = (mouse_position.x - palette_rect.x - 10) / palette_unit_size;
		uint32_t tile_y = (mouse_position.y - 32) / palette_unit_size;
		if (tile_x < palette_unit_size) {
			int32_t id = tile_x + tile_y * palette_wrap;
			if (id >= 0 && (uint32_t)id < (state->tile_sheet.columns * state->tile_sheet.rows)) {
				state->current_tile = id;
			}
		}
	}

	// // --- Drawing on the Map ---
	if (mouse_position.x < palette_rect.x) {
		Vector2 world_mouse_position = mouse_screen_to_world(&state->camera);

		uint32_t grid_x = world_mouse_position.x / GRID_SIZE;
		uint32_t grid_y = world_mouse_position.y / GRID_SIZE;

		// LOG_INFO("Position { %.2f, %.2f }", world_mouse_position.x, world_mouse_position.y);
		// LOG_INFO("Grid { %d, %d }", grid_x, grid_y);

		if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
			uint32_t index = grid_x + grid_y * state->level->columns;
			if (index < state->level->count) {
				Tile *tile = state->level->tiles[state->current_layer] + index;
				// Avoid re-populating if the tile is already the one we want
				if (tile->tile_id != state->current_tile) {
					IVector2 texture_offset = {
						.x = state->current_tile % state->tile_sheet.columns,
						.y = state->current_tile / state->tile_sheet.columns,
					};

					tile->tile_id = state->current_tile;
					object_populate(&tile->object, (Vector2){ grid_x * GRID_SIZE, grid_y * GRID_SIZE }, &state->tile_sheet, texture_offset, false);
					if (state->current_layer % 2 == 0)
						tile->object.shape.type = COLLISION_TYPE_NONE;
				}
			}
		}

		if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
			uint32_t index = grid_x + grid_y * state->level->columns;
			if (index < state->level->count) {
				Tile *tile = state->level->tiles[state->current_layer] + index;
				tile->tile_id = INVALID_ID;
				tile->object = (Object){ 0 };
			}
		}
	}

	// --- Saving ---
	if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S)) {
		level_save(state->level, "./assets/levels/level_01.txt");
	}
}

void draw_editor_ui(GameState *state) {
	float scale = fminf((float)GetScreenWidth() / RESOLUTION_WIDTH, (float)GetScreenHeight() / RESOLUTION_HEIGHT);
	Rectangle palette_rect = {
		.x = RESOLUTION_WIDTH * scale,
		.y = 0,
		.width = GetScreenWidth() - (RESOLUTION_WIDTH * scale),
		.height = RESOLUTION_HEIGHT * scale,
	};
	DrawRectangleRec(palette_rect, RAYWHITE);
	DrawText("TILE PALETTE", palette_rect.x + 10, 10, 20, DARKGRAY);

	// Show current layer
	char layer_text[64];
	snprintf(layer_text, sizeof(layer_text), "Layer: %d (Collidable = %b)", state->current_layer + 1, state->current_layer % 2 != 0);
	DrawText(layer_text, palette_rect.x + 200, 12, 10, DARKGRAY);

	for (uint32_t row = 0; row < state->tile_sheet.rows; row++) {
		for (uint32_t column = 0; column < state->tile_sheet.columns; column++) {
			int32_t index = column + row * state->tile_sheet.columns;

			Rectangle src = {
				column * (TILE_SIZE + TILE_GAP),
				row * (TILE_SIZE + TILE_GAP),
				TILE_SIZE,
				TILE_SIZE
			};

			uint32_t palette_unit_size = 32;
			uint32_t palette_wrap = (int32_t)(palette_rect.width - 20) / palette_unit_size;
			Rectangle dest = {
				(palette_rect.x + 10) + ((index % palette_wrap) * palette_unit_size), // Display 5 tiles per row in palette
				(index / palette_wrap) * palette_unit_size + 32,
				palette_unit_size,
				palette_unit_size
			};
			DrawTexturePro(state->tile_sheet.texture, src, dest, (Vector2){ 0 }, 0, WHITE);

			// Highlight selected tile
			if (index == state->current_tile) {
				DrawRectangleLinesEx(dest, 2, YELLOW);
			}
		}
	}
}

Vector2 mouse_screen_to_world(Camera2D *camera) {
	float scale = fminf((float)GetScreenWidth() / RESOLUTION_WIDTH, (float)GetScreenHeight() / RESOLUTION_HEIGHT);
	Rectangle dest = {
		.x = 0.0f,
		.y = (GetScreenHeight() - (RESOLUTION_HEIGHT * scale)) / 2.0f,
		.width = RESOLUTION_WIDTH * scale,
		.height = RESOLUTION_HEIGHT * scale
	};
	Vector2 mouse = GetMousePosition();
	Vector2 virtual_mouse = { (mouse.x - dest.x) / scale, (mouse.y - dest.y) / scale };
	return GetScreenToWorld2D(virtual_mouse, *camera);
}
