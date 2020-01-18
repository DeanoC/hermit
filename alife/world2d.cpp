//
// Created by Computer on 14/01/2020.
//

#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"
#include "world2d.hpp"
#include "al2o3_vfile/vfile.hpp"
#include "render_basics/descriptorset.h"
#include "render_basics/buffer.h"
#include "render_basics/pipeline.h"
#include "render_basics/graphicsencoder.h"
#include "render_basics/rootsignature.h"
#include "render_basics/shader.h"


World2D* World2D_Create(WorldType type, uint32_t width, uint32_t height) {
	World2D* world = (World2D*) MEMORY_CALLOC(1, sizeof(World2D));
	if(!world) goto Fail;

	world->width = width;
	world->height = height;
	switch(type) {
		case WT_MOE:
			world->world = MEMORY_CALLOC(width*height, sizeof(WorldMOE));
			break;
	}

	if(!world->world) goto Fail;

	return world;
Fail:
	World2D_Destroy(world);
	return nullptr;
}

void World2D_Destroy(World2D* world) {
	if(world) {
		if (world->world)
			MEMORY_FREE(world->world);
		MEMORY_FREE(world);
	}
}