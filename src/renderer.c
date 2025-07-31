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
	Rectangle dest = {
	  .x = object->position.x,
	  .y = object->position.y,
	  .width = object->sprite.src.width * object->scale.x,
	  .height = object->sprite.src.height * object->scale.y,
	};

	Vector2 scaled_origin = {
	  .x = object->sprite.origin.x * object->scale.x,
	  .y = object->sprite.origin.y * object->scale.y,
	};

	DrawTexturePro(object->sprite.texture, object->sprite.src, dest, scaled_origin, object->rotation, WHITE);

	DrawCircle(object->position.x, object->position.y, 5.f, (Color){230, 41, 55, 200});
	DrawRectanglePro(*(Rectangle*)&object->shape, scaled_origin, object->rotation, DEBUG_COLOR);
}
