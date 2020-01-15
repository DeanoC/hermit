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

struct World2DRender {
	Render_RendererHandle renderer;
	Render_DescriptorSetHandle descriptorSet;
	Render_BufferHandle indexBuffer;
	Render_BufferHandle vertexBuffer;
	Render_PipelineHandle pipeline;
	Render_ShaderHandle shader;
	Render_RootSignatureHandle rootSignature;

	bool dirty;
};


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
void DestroyRenderable(World2DRender *render) {
	if(!render) return;

	Render_BufferDestroy(render->renderer, render->vertexBuffer);
	MEMORY_FREE(render);
}

bool World2D_MakeRenderable(World2D* world, Render_RendererHandle renderer, Render_ROPLayout const* ropLayout) {
	ASSERT(world);
	ASSERT(world->render == nullptr);

	world->render = (World2DRender*) MEMORY_CALLOC(1, sizeof(World2DRender));
	if(!world->render) return false;

	World2DRender *render = world->render;
	render->renderer = renderer;
	uint32_t const totalVertexCount = world->width * world->height;
	uint32_t const totalIndexCount = (world->width-1) * (world->height-1) * 3;
	uint32_t const indexTypeSize = (totalVertexCount > 0xFFFF) ? 4u : 2u;

	// build render objects

	// construct static vertex buffer
	Render_BufferVertexDesc const vertexDesc {
		totalVertexCount,
		sizeof(float) * 5,
		false
	};
	render->vertexBuffer = Render_BufferCreateVertex(renderer, &vertexDesc);

	if(!Render_BufferHandleIsValid(render->vertexBuffer)) {
		DestroyRenderable(render);
		return false;
	}
	uint8_t* vertexData = nullptr;
	// MAX 64 KB stack else use the heap
	if(totalVertexCount > (0xFFFF / (sizeof(float) * 5))) {
		// short term alloc
		vertexData = (uint8_t*) MEMORY_MALLOC(totalVertexCount * (sizeof(float) * 5));
	} else {
		vertexData = (uint8_t*) STACK_ALLOC(totalVertexCount * (sizeof(float) * 5));
	}

	float * curVertexPtr = (float*)vertexData;
	for(uint32_t y = 0; y < world->height-1;++y) {
		for(int32_t x = -(int32_t)(world->width/2); x < (int32_t)(world->width/2);++x) {
					curVertexPtr[0] = (float)x;
					curVertexPtr[1] = 0.0f;
					curVertexPtr[2] = (float)x;
					curVertexPtr[3] = (float)(x + world->width/2) / (float)(world->width-1);
					curVertexPtr[4] = (float)y / (float)(world->height-1);
					curVertexPtr += 5;
		}
	}
	Render_BufferUpdateDesc const vertexUpdateDesc {
			vertexData,
			0,
			totalVertexCount * sizeof(float) * 5
	};
	Render_BufferUpload(render->vertexBuffer, &vertexUpdateDesc);

	// no need to free stack allocated ram
	if(totalVertexCount > (0xFFFF / (sizeof(float) * 5))) {
		MEMORY_FREE(vertexData);
	}

	// construct the static index buffer
	Render_BufferIndexDesc const indexDesc {
		totalIndexCount,
		indexTypeSize,
		false
	};
	render->indexBuffer = Render_BufferCreateIndex(renderer, &indexDesc);
	if(!Render_BufferHandleIsValid(render->indexBuffer)) {
		DestroyRenderable(render);
		return false;
	}

	uint8_t* indexData = nullptr;
	// MAX 64 KB stack else use the heap
	if(totalIndexCount > (0xFFFF / (indexTypeSize*3))) {
		// short term alloc
		indexData = (uint8_t*) MEMORY_MALLOC(totalIndexCount * indexTypeSize);
	} else {
		indexData = (uint8_t*) STACK_ALLOC(totalIndexCount * indexTypeSize);
	}

	uint8_t * curIndexPtr = indexData;
	for(uint32_t y = 0; y < world->height-1;++y) {
		for(uint32_t x = 0; x < world->width-1;++x) {
			switch(indexTypeSize) {
				case 2:
					((uint16_t*)curIndexPtr)[0] = (y * world->height) + x;
					((uint16_t*)curIndexPtr)[1] = ((y+1) * world->height) + x;
					((uint16_t*)curIndexPtr)[2] = (y * world->height) + (x+1);
					curIndexPtr += 6;
					break;
				case 4: {
					((uint32_t*)curIndexPtr)[0] = (y * world->height) + x;
					((uint32_t*)curIndexPtr)[1] = ((y+1) * world->height) + x;
					((uint32_t*)curIndexPtr)[2] = (y * world->height) + (x+1);
					curIndexPtr += 12;
					break;
				}
				default:
					LOGERROR("Invalid index buffer type size");
					break;
			}
		}
	}
	Render_BufferUpdateDesc const indexUpdateDesc {
		indexData,
		0,
		totalIndexCount * indexTypeSize
	};
	Render_BufferUpload(render->indexBuffer, &indexUpdateDesc);

	// no need to free stack allocated ram
	if(totalIndexCount > (0xFFFF / (indexTypeSize*3))) {
		MEMORY_FREE(indexData);
	}

	VFile::ScopedFile vfile = VFile::FromFile("resources/poscolour_vertex.hlsl", Os_FM_Read);
	if (!vfile) {
		DestroyRenderable(render);
		return false;
	}
	VFile::ScopedFile ffile = VFile::FromFile("resources/copycolour_fragment.hlsl", Os_FM_Read);
	if (!ffile) {
		DestroyRenderable(render);
		return false;
	}


	render->shader = Render_CreateShaderFromVFile(render->renderer, vfile, "VS_main", ffile, "FS_main");
	if (!Render_ShaderHandleIsValid(render->shader)) {
		DestroyRenderable(render);
		return false;
	}

	Render_RootSignatureDesc rootSignatureDesc{};
	rootSignatureDesc.shaderCount = 1;
	rootSignatureDesc.shaders = &render->shader;
	rootSignatureDesc.staticSamplerCount = 0;
	render->rootSignature = Render_RootSignatureCreate(render->renderer, &rootSignatureDesc);
	if (!Render_RootSignatureHandleIsValid(render->rootSignature)) {
		DestroyRenderable(render);
		return false;
	}

	TinyImageFormat colourFormats[] = { ropLayout->colourFormats[0] };

	Render_GraphicsPipelineDesc gfxPipeDesc{};
	gfxPipeDesc.shader = render->shader;
	gfxPipeDesc.rootSignature = render->rootSignature;
	gfxPipeDesc.vertexLayout = Render_GetStockVertexLayout(render->renderer, Render_SVL_3D_COLOUR);
	gfxPipeDesc.blendState = Render_GetStockBlendState(render->renderer, Render_SBS_OPAQUE);
	if(ropLayout->depthFormat == TinyImageFormat_UNDEFINED) {
		gfxPipeDesc.depthState = Render_GetStockDepthState(render->renderer, Render_SDS_IGNORE);
	} else {
		gfxPipeDesc.depthState = Render_GetStockDepthState(render->renderer, Render_SDS_READWRITE_LESS);
	}
	gfxPipeDesc.rasteriserState = Render_GetStockRasterisationState(render->renderer, Render_SRS_BACKCULL);
	gfxPipeDesc.colourRenderTargetCount = 1;
	gfxPipeDesc.colourFormats = colourFormats;
	gfxPipeDesc.depthStencilFormat = ropLayout->depthFormat;
	gfxPipeDesc.sampleCount = 1;
	gfxPipeDesc.sampleQuality = 0;
	gfxPipeDesc.primitiveTopo = Render_PT_TRI_LIST;
	render->pipeline = Render_GraphicsPipelineCreate(render->renderer, &gfxPipeDesc);
	if (!Render_PipelineHandleIsValid(render->pipeline)) {
		return false;
	}

	Render_DescriptorSetDesc const setDesc = {
			render->rootSignature,
			Render_DUF_PER_FRAME,
			1
	};
/*
	render->descriptorSet = Render_DescriptorSetCreate(render->renderer, &setDesc);
	if (!Render_DescriptorSetHandleIsValid(render->descriptorSet)) {
		return false;
	}
	Render_DescriptorDesc params[1];
	params[0].name = "View";
	params[0].type = Render_DT_BUFFER;
	params[0].buffer = manager->viewUniformBuffer;
	params[0].offset = 0;
	params[0].size = sizeof(manager->viewUniforms);
	Render_DescriptorPresetFrequencyUpdated(material.descriptorSet, 0, 1, params);
*/
	return true;
}

void World2D_Render(World2D* world, Render_GraphicsEncoderHandle encoder) {
	ASSERT(world);
	ASSERT(world->render);

	Render_GraphicsEncoderBindDescriptorSet(encoder, world->render->descriptorSet, 0);
	Render_GraphicsEncoderBindIndexBuffer(encoder, world->render->indexBuffer, 0);
	Render_GraphicsEncoderBindVertexBuffer(encoder, world->render->vertexBuffer, 0);
	Render_GraphicsEncoderBindPipeline(encoder, world->render->pipeline);
	uint32_t const totalIndexCount = (world->width-1) * (world->height-1) * 3;
	Render_GraphicsEncoderDrawIndexed(encoder, totalIndexCount, 0, 0);

}
