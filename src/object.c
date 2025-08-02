#include "object.h"
#include "game_types.h"

Rectangle object_get_collision_shape(Object *object) {
	return (Rectangle){
		.x = object->transform.position.x + object->shape.transform.position.x,
		.y = object->transform.position.y + object->shape.transform.position.y,
		.width = object->shape.width * object->shape.transform.scale.x * object->transform.scale.x,
		.height = object->shape.height * object->shape.transform.scale.y * object->transform.scale.y,
	};
}

bool object_is_colliding(Object *a, Object *b) {
	Rectangle collision_shape_a = object_get_collision_shape(a);
	Rectangle collision_shape_b = object_get_collision_shape(b);

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
