#include "player.h"

#include "globals.h"
#include "object.h"

#include <raylib.h>
#include <raymath.h>

static Vector2 previous_direction = { 0 };
static float MOVE_SPEED = 200.0f; // Speed of grid transition animation
static bool is_moving = false;
static Vector2 start_position = { 0 };
static Vector2 target_position = { 0 };
static float move_timer = 0.0f;
static float move_duration = 0.0f;

void player_populate(Object *player);

void player_initialize(GameState *state) {
	object_populate(&state->player, PLAYER_SPAWN_POSITION, &state->player_sheet, (IVector2){ 0, 0 }, true);
	player_populate(&state->player);

	// Snap player to grid on initialization
	state->player.transform.position.x = roundf(state->player.transform.position.x / GRID_SIZE) * GRID_SIZE;
	state->player.transform.position.y = roundf(state->player.transform.position.y / GRID_SIZE) * GRID_SIZE;
}

bool can_move_to_position(GameState *state, Vector2 target_pos) {
	// Create a temporary collision shape at the target position
	Rectangle test_collision = {
		.x = target_pos.x - GRID_SIZE / 2.0f,
		.y = target_pos.y - 16.0f, // Half of player collision height
		.width = GRID_SIZE,
		.height = 16.0f
	};

	// Check collision with all tiles
	for (uint32_t i = 0; i < LAYERS; i++) {
		for (uint32_t j = 0; j < state->level->count; j++) {
			Object *tile = &state->level->tiles[i][j].object;
			if (tile->shape.type != COLLISION_TYPE_NONE) {
				Rectangle tile_collision = object_get_collision_shape(tile);
				if (CheckCollisionRecs(test_collision, tile_collision)) {
					return false;
				}
			}
		}
	}
	return true;
}

void player_update(GameState *state, float dt) {
	Object *player = &state->player;

	if (!is_moving) {
		// Check for input only when not currently moving
		Vector2 input_direction = { 0 };

		if (IsKeyDown(KEY_D))
			input_direction.x = 1;
		else if (IsKeyDown(KEY_A))
			input_direction.x = -1;
		else if (IsKeyDown(KEY_S))
			input_direction.y = 1;
		else if (IsKeyDown(KEY_W))
			input_direction.y = -1;

		if (input_direction.x != 0 || input_direction.y != 0) {
			// Calculate target position (one grid cell in the input direction)
			Vector2 new_target = {
				player->transform.position.x + input_direction.x * GRID_SIZE,
				player->transform.position.y + input_direction.y * GRID_SIZE
			};

			// Check if we can move to the target position
			if (can_move_to_position(state, new_target)) {
				// Start movement
				is_moving = true;
				start_position = player->transform.position;
				target_position = new_target;
				move_timer = 0.0f;
				move_duration = GRID_SIZE / MOVE_SPEED; // Time to move one tile

				// Update animation based on direction
				if (!Vector2Equals(input_direction, previous_direction)) {
					IVector2 animation = {
						.x = input_direction.x != 0 ? 2 : input_direction.y == -1 ? 1
																				  : 0,
						.y = 0
					};
					object_populate(player, player->transform.position, &state->player_sheet, animation, true);
					player_populate(player);

					// Handle sprite flipping for horizontal movement
					if (input_direction.x == -1 && player->sprite.src.width >= 0)
						player->sprite.src.width *= -1;
					else if (input_direction.x == 1 && player->sprite.src.width < 0)
						player->sprite.src.width *= -1;
				}

				previous_direction = input_direction;
			}
		}
	} else {
		// Currently moving - update position with interpolation
		move_timer += dt;

		if (move_timer >= move_duration) {
			// Movement complete
			player->transform.position = target_position;
			is_moving = false;
			move_timer = 0.0f;
		} else {
			// Interpolate position (you can use different easing functions here)
			float t = move_timer / move_duration;

			// Linear interpolation
			player->transform.position = Vector2Lerp(start_position, target_position, t);

			// Alternative: Smooth easing (uncomment to use)
			// float smooth_t = t * t * (3.0f - 2.0f * t); // Smoothstep
			// player->transform.position = Vector2Lerp(start_position, target_position, smooth_t);
		}
	}

	// Update camera to follow player
	state->camera.target = (Vector2){
		Clamp(state->player.transform.position.x, RESOLUTION_WIDTH / 2.f, 10000),
		Clamp(state->player.transform.position.y, RESOLUTION_HEIGHT / 2.f, 10000),
	};
}

void player_populate(Object *player) {
	player->sprite.origin.y = player->sprite.src.height;
	player->shape = (CollisionShape){
		.type = COLLISION_TYPE_RECTANGLE,
		.transform = {
		  .position = {
			.x = -player->sprite.src.width / 4.f * player->transform.scale.x,
			.y = -player->sprite.src.height / 2.f * player->transform.scale.y,
		  },
		  .scale = { 1.f, 1.f },
		},
		.width = TILE_SIZE,
		.height = 32 / 2.f
	};
}
