#include "renderer.h"
#include "globals.h"

#include <raylib.h>

static Color DEBUG_COLOR = { 153, 0, 179, 107 };

// typedef struct _renderer {
// 	Camera2D camera;
// } Renderer;

// static Renderer g_renderer = {0};

void renderer_begin_frame(Camera2D *camera) {
	// memcpy(&g_renderer.camera, camera, sizeof(Camera2D));
}
void renderer_end_frame() {}

void renderer_submit(Object *object) {
	Rectangle sprite_dest_rect = {
		.x = object->transform.position.x + object->sprite.transform.position.x,
		.y = object->transform.position.y + object->sprite.transform.position.y,
		.width = object->sprite.src.width * object->sprite.transform.scale.x * object->transform.scale.x,
		.height = object->sprite.src.height * object->sprite.transform.scale.y * object->transform.scale.y,
	};
	Vector2 sprite_scaled_origin = {
		.x = object->sprite.origin.x * object->sprite.transform.scale.x * object->transform.scale.x,
		.y = object->sprite.origin.y * object->sprite.transform.scale.x * object->transform.scale.y,
	};
	float sprite_rotation = object->transform.rotation + object->sprite.transform.rotation;

	Rectangle shape_dest_rect = {
		.x = object->transform.position.x + object->shape.transform.position.x,
		.y = object->transform.position.y + object->shape.transform.position.y,
		.width = object->shape.width * object->shape.transform.scale.x * object->transform.scale.x,
		.height = object->shape.height * object->shape.transform.scale.y * object->transform.scale.y,
	};
	float shape_rotation = object->transform.rotation + object->shape.transform.rotation;

	DrawTexturePro(object->sprite.texture, object->sprite.src, sprite_dest_rect, sprite_scaled_origin, sprite_rotation, WHITE);

#ifdef COLLISION_SHAPES
	if (object->shape.type == COLLISION_TYPE_RECTANGLE) {
		DrawRectanglePro(shape_dest_rect, (Vector2){ 0.0f, 0.0f }, shape_rotation, DEBUG_COLOR);
	}
	DrawCircle(object->transform.position.x, object->transform.position.y, sprite_dest_rect.width / 16, (Color){ 230, 41, 55, 200 });
#endif
}
