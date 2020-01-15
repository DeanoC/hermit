// License Summary: MIT see LICENSE file
#pragma once

#include "al2o3_cmath/vector.hpp"
#include "render_basics/api.h"

enum WorldType {
	WT_MOE,
};

struct WorldMOE {
	int16_t food;
};

struct World2DRender;

struct World2D {
	uint32_t width;
	uint32_t height;

	void* world;
	bool dirty;
	World2DRender* render;
};

World2D* World2D_Create(WorldType type, uint32_t width, uint32_t height);
void World2D_Destroy(World2D* world);
bool World2D_MakeRenderable(World2D* world, Render_RendererHandle renderer);
inline bool World2D_IsRenderable(World2D* world) { world ? world->render : false; }

template<typename T> T const * World2D_ElementsAs(World2D const * world) {
	ASSERT(world);
	return (T const *)world->world;
}

template<typename T> T * World2D_MutableElementsAs(World2D * world) {
	ASSERT(world);
	world->dirty = true;
	return (T*)world->world;
}

void World2D_Render(World2D* world, Render_GraphicsEncoderHandle encoder);
