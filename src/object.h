#pragma once

#include "globals.h"

void object_populate(Object *object, Vector2 position, const SpriteSheet *tile_sheet, IVector2 texture_offset, bool centered);

bool object_is_colliding(Object *a, Object *b);
Rectangle object_get_collision_shape(Object *object);
