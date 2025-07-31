#include "renderer.h"

#include <raylib.h>
#include <stdint.h>
#include <string.h>

static Color DEBUG_COLOR = {0, 228, 48, 200};

// typedef struct _renderer {
// 	Camera2D camera;
// } Renderer;

// static Renderer g_renderer = {0};

void renderer_begin_frame(Camera2D *camera) {
	// memcpy(&g_renderer.camera, camera, sizeof(Camera2D));
}
void renderer_end_frame() {}

void renderer_submit(Object *object) {
	Rectangle dest_sprite = {
	  .x = object->position.x,
	  .y = object->position.y,
	  .width = object->sprite.src.width * object->scale.x,
	  .height = object->sprite.src.height * object->scale.y,
	};
	Rectangle dest_shape = {
	  .x = object->position.x,
	  .y = object->position.y,
	  .width = object->shape.width * object->scale.x,
	  .height = object->shape.height * object->scale.y,
	};

	Vector2 scaled_sprite_origin = {
	  .x = object->sprite.origin.x * object->scale.x,
	  .y = object->sprite.origin.y * object->scale.y,
	};

	Vector2 scaled_shape_origin = {
	  .x = object->shape.origin.x * object->scale.x,
	  .y = object->shape.origin.y * object->scale.y,
	};

	DrawTexturePro(object->sprite.texture, object->sprite.src, dest_sprite, scaled_sprite_origin, object->rotation, WHITE);

	DrawCircle(object->position.x, object->position.y, 5.f, (Color){230, 41, 55, 200});
	DrawRectanglePro(dest_shape, scaled_sprite_origin, object->rotation, DEBUG_COLOR);
}
