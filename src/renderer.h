#pragma once

// #define COLLISION_SHAPES 1

#include "object.h"

#include <raylib.h>

void renderer_begin_frame(Camera2D *camera);
void renderer_end_frame();

void renderer_submit(Object *object);
