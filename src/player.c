#include "player.h"

#include "object.h"

#include <raylib.h>
#include <raymath.h>

static float PLAYER_SPEED = 50.f * TILE_SCALE; // Pixels per second

void player_initialize(GameState *state) {
	object_populate(&state->player, PLAYER_SPAWN_POSITION, &state->player_sheet, (IVector2){ 0, 0 }, true);
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
}

void player_update(GameState *state, float dt) {
	Vector2 direction = Vector2Normalize((Vector2){
	  .x = IsKeyDown(KEY_D) - IsKeyDown(KEY_A),
	  .y = IsKeyDown(KEY_S) - IsKeyDown(KEY_W),
	});

	Object *player = &state->player;

	IVector2 animation = {
		.x = direction.x != 0 ? 2 : direction.y == -1 ? 1
													  : 0,
		.y = 0
	};

	object_populate(player, player->transform.position, &state->player_sheet, animation, true);
	state->player.sprite.origin.y = state->player.sprite.src.height;
	state->player.shape = (CollisionShape){
		.type = COLLISION_TYPE_RECTANGLE,
		.transform = {
		  .position = {
			.x = -state->player.sprite.src.width / 4.f * state->player.transform.scale.x,
			.y = -state->player.sprite.src.height / 2.f * state->player.transform.scale.y,
		  },
		  .scale = { 1.f, 1.f },
		},
		.width = TILE_SIZE,
		.height = 32 / 2.f
	};

	Vector2 velocity = Vector2Scale(direction, PLAYER_SPEED * dt);

	state->player.transform.position = Vector2Add(state->player.transform.position, velocity);
	for (uint32_t i = 0; i < LAYERS; i++) {
		for (uint32_t j = 0; j < state->level->count; j++) {
			Object *tile = &state->level->tiles[i][j].object;

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
	}

	state->camera.target = (Vector2){
		Clamp(state->player.transform.position.x, RESOLUTION_WIDTH / 2.f, 10000),
		Clamp(state->player.transform.position.y, RESOLUTION_HEIGHT / 2.f, 10000),
	};
}
