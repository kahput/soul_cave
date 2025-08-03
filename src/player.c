#include "player.h"

#include "core/logger.h"
#include "globals.h"
#include "object.h"

#include <raylib.h>
#include <raymath.h>

#define PLAYER_GRID (GRID_SIZE / 2.f)

static Vector2 previous_direction = { 0 };
static float MOVE_SPEED = 256.0f; // Speed of grid transition animation
static bool is_moving = false;
static Vector2 start_position = { 0 };
static Vector2 target_position = { 0 };
static float move_timer = 0.0f;
static float move_duration = 0.0f;

static uint32_t current_animation = 0;
static float animation_timer = 0.0f;
static float animation_duration = .4f;

// Tile pushing variables
static bool is_pushing_tile = false;
static Vector2 target_tile_start = { 0 };
static Vector2 target_tile_target = { 0 };
static uint32_t pushing_tile_layer = 0;
static uint32_t pushing_tile_index = 0;

void player_populate(Object *player);

typedef struct {
	bool can_move;
	bool is_pushing;
	Vector2 tile_to_push_pos;
	uint32_t tile_layer;
	uint32_t tile_index;
} MoveResult;

void player_initialize(GameState *state) {
	object_populate(&state->player, PLAYER_SPAWN_POSITION, &state->player_sheet, (IVector2){ 1, 0 }, true);
	player_populate(&state->player);
	state->player_light_radius = GRID_SIZE * 2.f;
	current_animation = 1;

	// Snap player to grid on initialization
	state->player.transform.position.x = roundf(state->player.transform.position.x / PLAYER_GRID) * PLAYER_GRID;
	state->player.transform.position.y = roundf(state->player.transform.position.y / PLAYER_GRID) * PLAYER_GRID;
}

bool can_push_tile(GameState *state, Vector2 tile_pos, Vector2 push_direction) {
	// Calculate where the tile would move to
	Vector2 tile_target = {
		tile_pos.x + push_direction.x * GRID_SIZE,
		tile_pos.y + push_direction.y * GRID_SIZE
	};

	// Create collision rectangle for the tile at its target position
	Rectangle pushed_tile_collision = {
		.x = tile_target.x,
		.y = tile_target.y,
		.width = GRID_SIZE,
		.height = GRID_SIZE
	};

	// Check if the target position is free
	for (uint32_t i = 0; i < LAYERS; i++) {
		for (uint32_t j = 0; j < state->level->count; j++) {
			Object *other_tile = &state->level->tiles[i][j].object;
			if (other_tile->shape.type != COLLISION_TYPE_NONE) {
				// Skip the tile we're trying to push
				if (Vector2Distance(other_tile->transform.position, tile_pos) < 1.0f) {
					continue;
				}

				Rectangle other_collision = object_get_collision_shape(other_tile);
				if (CheckCollisionRecs(pushed_tile_collision, other_collision)) {
					return false;
				}
			}
		}
	}
	return true;
}

MoveResult check_player_movement(GameState *state, Vector2 target_pos, Vector2 direction) {
	MoveResult result = { false, false, { 0 }, 0, 0 };

	// Create player collision shape at target position
	Rectangle player_collision = {
		.x = target_pos.x - state->player.shape.width / 2.f,
		.y = target_pos.y - state->player.shape.height,
		.width = state->player.shape.width,
		.height = state->player.shape.height
	};

	// Check for collisions with tiles
	for (uint32_t i = 0; i < LAYERS; i++) {
		for (uint32_t j = 0; j < state->level->count; j++) {
			Tile *tile = &state->level->tiles[i][j];
			Object *tile_object = &state->level->tiles[i][j].object;

			if (tile_object->shape.type != COLLISION_TYPE_NONE) {
				Rectangle tile_collision = object_get_collision_shape(tile_object);
				if (CheckCollisionRecs(player_collision, tile_collision)) {
					// We hit a tile - check if it's pushable
					if (tile->tile_id == PUSHABLE_TILE) {
						if (can_push_tile(state, tile_object->transform.position, direction)) {
							result.can_move = true;
							result.is_pushing = true;
							result.tile_to_push_pos = tile_object->transform.position;
							result.tile_layer = i;
							result.tile_index = j;
							return result;
						} else {
							// Can't push the tile, movement blocked
							return result;
						}
					} else {
						// Non-pushable tile, movement blocked
						return result;
					}
				}
			}
		}
	}

	// No collision, free to move
	result.can_move = true;
	return result;
}

void player_update(GameState *state, float dt) {
	Object *player = &state->player;
	static uint32_t animation_index = 0;

	animation_timer += dt;

	if (animation_timer >= animation_duration) {
		animation_index = animation_index ? 0 : 3;
		current_animation += current_animation <= 3 ? 3 : -3;
		IVector2 animation = {
			.x = current_animation % 3,
			.y = current_animation / 3,
		};

		int32_t multi = 0;

		if (player->sprite.src.width >= 0)
			multi = 1;
		else if (player->sprite.src.width < 0)
			multi = -1;
		object_populate(player, player->transform.position, &state->player_sheet, animation, true);
		player_populate(player);
		player->sprite.src.width *= multi;
		animation_timer = 0.0f;
	}

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
				player->transform.position.x + input_direction.x * PLAYER_GRID,
				player->transform.position.y + input_direction.y * PLAYER_GRID
			};

			// Check movement and potential tile pushing
			MoveResult move_result = check_player_movement(state, new_target, input_direction);

			if (move_result.can_move) {
				// Start movement
				is_moving = true;
				start_position = player->transform.position;
				target_position = new_target;
				move_timer = 0.0f;
				move_duration = GRID_SIZE / MOVE_SPEED; // Time to move one tile

				// Handle tile pushing
				if (move_result.is_pushing) {
					is_pushing_tile = true;
					target_tile_start = move_result.tile_to_push_pos;
					target_tile_target = (Vector2){
						move_result.tile_to_push_pos.x + input_direction.x * GRID_SIZE,
						move_result.tile_to_push_pos.y + input_direction.y * GRID_SIZE
					};
					pushing_tile_layer = move_result.tile_layer;
					pushing_tile_index = move_result.tile_index;
				} else {
					is_pushing_tile = false;
				}

				// Update animation based on direction

				if (!Vector2Equals(input_direction, previous_direction)) {
					IVector2 animation = {
						.x = input_direction.x != 0 ? 2 + animation_index : input_direction.y == -1 ? 1 + animation_index
																									: 0 + animation_index,
						.y = 0
					};
					current_animation = animation.x + animation.y * 3;
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

			IVector2 player_coord = {
				.x = floor(player->transform.position.x / GRID_SIZE),
				.y = floor(player->transform.position.y / GRID_SIZE),
			};
			uint32_t player_new_index = player_coord.x + player_coord.y * state->level->columns;

			// LOG_INFO("Player Coord { %d, %d }", player_coord.x, player_coord.y);
			// LOG_INFO("Player position { %.2f, %.2f }", target_position.x / GRID_SIZE, target_position.y / GRID_SIZE);

			// Complete tile push if we were pushing
			if (is_pushing_tile) {
				// IVector2 old_coord = {
				// 	.x = pushing_tile_index % state->level->columns,
				// 	.y = pushing_tile_index / state->level->columns,
				// };
				IVector2 new_coord = {
					.x = target_tile_target.x / GRID_SIZE,
					.y = target_tile_target.y / GRID_SIZE,
				};
				uint32_t new_tile_index = new_coord.x + new_coord.y * state->level->columns;
				// LOG_INFO("{ %d, %d } -> { %d, %d }", old_coord.x, old_coord.y, new_coord.x, new_coord.y);

				Tile *pushed_tile = &state->level->tiles[pushing_tile_layer][pushing_tile_index];
				Tile *target_tile = &state->level->tiles[pushing_tile_layer][new_tile_index];
				target_tile->tile_id = pushed_tile->tile_id;
				target_tile->object = pushed_tile->object;
				target_tile->object.transform.position = target_tile_target;

				for (uint32_t layer = 0; layer < LAYERS; layer++) {
					Tile *tile = &state->level->tiles[layer][new_tile_index];
					if (tile->tile_id == PLATE_TILE) {
						// tile->tile_id = INVALID_ID;
						// tile->object = (Object){ 0 };
						state->player_light_radius += GRID_SIZE;
						// Sound sound = LoadSound("./assets/sounds/Pillar_Push.wav");
						// PlaySound(sound);
					}
				}

				pushed_tile->tile_id = INVALID_ID;
				pushed_tile->object = (Object){ 0 };
				is_pushing_tile = false;
			}

			is_moving = false;
			move_timer = 0.0f;
		} else {
			// Interpolate position (you can use different easing functions here)
			float t = move_timer / move_duration;

			// Linear interpolation for player
			player->transform.position = Vector2Lerp(start_position, target_position, t);

			// Interpolate pushed tile if pushing
			if (is_pushing_tile) {
				// float smooth_t = t * t * (3.0f - 2.0f * t); // Smoothstep
				Object *pushed_tile = &state->level->tiles[pushing_tile_layer][pushing_tile_index].object;
				pushed_tile->transform.position = Vector2Lerp(target_tile_start, target_tile_target, t);
			}

			// Alternative: Smooth easing (uncomment to use)
			// float smooth_t = t * t * (3.0f - 2.0f * t); // Smoothstep
			// player->transform.position = Vector2Lerp(start_position, target_position, smooth_t);
		}
	}

	// Update camera to follow player
	state->camera.target = (Vector2){
		Clamp(state->player.transform.position.x, RESOLUTION_WIDTH / 2.f, (state->level->columns * GRID_SIZE) - RESOLUTION_WIDTH / 2.f),
		Clamp(state->player.transform.position.y, RESOLUTION_HEIGHT / 2.f, (state->level->rows * GRID_SIZE) - RESOLUTION_WIDTH / 2.f + GRID_SIZE),
	};
}

void player_populate(Object *player) {
	player->sprite.origin.y = player->sprite.src.height;
	player->shape = (CollisionShape){
		.type = COLLISION_TYPE_RECTANGLE,
		.transform = {
		  .position = {
			.x = -player->sprite.src.width / 4.f * player->transform.scale.x,
			.y = -player->sprite.src.height / 4.f * player->transform.scale.y,
		  },
		  .scale = { 1.f, 1.f },
		},
		.width = PLAYER_GRID / 2.f,
		.height = 8.f
	};
}
