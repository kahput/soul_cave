#pragma once

#include <raylib.h>

typedef union {
	struct {
		float x;
		float y;
		float width;
		float height;
	};

} CollisionShape;

typedef struct {
	Rectangle src;
	Vector2 origin;

	Texture texture;
} Sprite;

typedef struct {
	Vector2 position, scale;
	float rotation;

	Sprite sprite;
	CollisionShape shape;
} Object;

void renderer_begin_frame(Camera2D *camera);
void renderer_end_frame();

void renderer_submit(Object *object);
