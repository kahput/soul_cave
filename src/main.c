#include "renderer.h"

#include <raylib.h>
#include <raymath.h>

#include <stdint.h>

static const int32_t SCREEN_WIDTH = 1280;
static const int32_t SCREEN_HEIGHT = 720;
static const int32_t TILE_SIZE = 16;
static const int32_t TILE_GAP = 1;

int main(void) {
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "raylib [core] example - keyboard input");
	SetTargetFPS(60);

	Texture tile_sheet = LoadTexture("./assets/tiles/tilemap.png");

	Object player = {
	  .position = {.x = SCREEN_WIDTH / 2.f, .y = SCREEN_HEIGHT / 2.f},
	  .scale = {4.f, 4.f},
	  .rotation = 45.f,
	  .sprite = {
		.texture = tile_sheet,
		.src = {
		  .x = (float)(TILE_SIZE + TILE_GAP) * 1.f,
		  .y = (float)(TILE_SIZE + TILE_GAP) * 7.f,
		  .width = TILE_SIZE,
		  .height = TILE_SIZE,
		},
		.origin = {
		  .x = 0.f,
		  .y = 0.f,
		},
	  },
	};

	player.sprite.origin = (Vector2){
	  .x = player.sprite.src.width / 2.f,
	  .y = player.sprite.src.height,
	};

	player.shape = (CollisionShape){
	  .x = player.position.x,
	  .y = player.position.y,
	  .width = player.sprite.src.width * player.scale.x,
	  .height = player.sprite.src.height * player.scale.y,
	};

	while (!WindowShouldClose()) {
		BeginDrawing();
		if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D))
			player.position.x += 2.0f;
		if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))
			player.position.x -= 2.0f;
		if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W))
			player.position.y -= 2.0f;
		if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S))
			player.position.y += 2.0f;
		ClearBackground(RAYWHITE);

		renderer_begin_frame((void *)0);
		renderer_submit(&player);
		renderer_end_frame();

		EndDrawing();
	}

	CloseWindow();

	return 0;
}
