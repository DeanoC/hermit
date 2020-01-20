// License Summary: MIT see LICENSE file

#include "al2o3_platform/platform.h"
#include "alifetests.hpp"
#include "al2o3_memory/memory.h"
#include "world2d.hpp"
#include "al2o3_vfile/vfile.hpp"
#include "render_basics/descriptorset.h"
#include "render_basics/buffer.h"
#include "render_basics/pipeline.h"
#include "render_basics/graphicsencoder.h"
#include "render_basics/rootsignature.h"
#include "render_basics/shader.h"
#include "cudacore.hpp"

namespace {

void DestroyRenderable(World2DRender *render);

World2DRender* MakeRenderable(World2D const* world, Render_RendererHandle renderer, Render_ROPLayout const* ropLayout) {

	World2DRender* render = (World2DRender*) MEMORY_CALLOC(1, sizeof(World2DRender));
	if(!render) return nullptr;

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
			curVertexPtr[2] = (float)y;
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

	static Render_BufferUniformDesc const ubDesc{
			sizeof(render->uniforms),
			true
	};

	render->uniformBuffer = Render_BufferCreateUniform(render->renderer, &ubDesc);
	if (!Render_BufferHandleIsValid(render->uniformBuffer)) {
		DestroyRenderable(render);
		return nullptr;
	}

	VFile::ScopedFile vfile = VFile::FromFile("resources/alife/world2dmoe_vertex.hlsl", Os_FM_Read);
	if (!vfile) {
		DestroyRenderable(render);
		return nullptr;
	}
	VFile::ScopedFile ffile = VFile::FromFile("resources/alife/world2dmoe_fragment.hlsl", Os_FM_Read);
	if (!ffile) {
		DestroyRenderable(render);
		return nullptr;
	}


	render->shader = Render_CreateShaderFromVFile(render->renderer, vfile, "VS_main", ffile, "FS_main");
	if (!Render_ShaderHandleIsValid(render->shader)) {
		DestroyRenderable(render);
		return nullptr;
	}

	Render_RootSignatureDesc rootSignatureDesc{};
	rootSignatureDesc.shaderCount = 1;
	rootSignatureDesc.shaders = &render->shader;
	rootSignatureDesc.staticSamplerCount = 0;
	render->rootSignature = Render_RootSignatureCreate(render->renderer, &rootSignatureDesc);
	if (!Render_RootSignatureHandleIsValid(render->rootSignature)) {
		DestroyRenderable(render);
		return nullptr;
	}

	TinyImageFormat colourFormats[] = { ropLayout->colourFormats[0] };

	Render_GraphicsPipelineDesc gfxPipeDesc{};
	gfxPipeDesc.shader = render->shader;
	gfxPipeDesc.rootSignature = render->rootSignature;
	gfxPipeDesc.vertexLayout = Render_GetStockVertexLayout(render->renderer, Render_SVL_3D_UV);
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
		DestroyRenderable(render);
		return nullptr;
	}

	Render_DescriptorSetDesc const setDesc = {
			render->rootSignature,
			Render_DUF_PER_FRAME,
			1
	};
	render->descriptorSet = Render_DescriptorSetCreate(render->renderer, &setDesc);
	if (!Render_DescriptorSetHandleIsValid(render->descriptorSet)) {
		DestroyRenderable(render);
		return nullptr;
	}

	Render_DescriptorDesc params[1];
	params[0].name = "View";
	params[0].type = Render_DT_BUFFER;
	params[0].buffer = render->uniformBuffer;
	params[0].offset = 0;
	params[0].size = sizeof(render->uniforms);
	Render_DescriptorPresetFrequencyUpdated(render->descriptorSet, 0, 1, params);

	return render;
}

void DestroyRenderable(World2DRender *render) {
	if(!render) return;

	Render_DescriptorSetDestroy(render->renderer, render->descriptorSet);
	Render_PipelineDestroy(render->renderer, render->pipeline);
	Render_ShaderDestroy(render->renderer, render->shader);
	Render_RootSignatureDestroy(render->renderer, render->rootSignature);

	Render_BufferDestroy(render->renderer, render->uniformBuffer);
	Render_BufferDestroy(render->renderer, render->indexBuffer);
	Render_BufferDestroy(render->renderer, render->vertexBuffer);

	MEMORY_FREE(render);
}
void RenderWorld2D(World2D const * world, World2DRender* render, Render_GraphicsEncoderHandle encoder) {

	// upload the uniforms
	Render_BufferUpdateDesc uniformUpdate = {
			&render->uniforms,
			0,
			sizeof(render->uniforms)
	};
	Render_BufferUpload(render->uniformBuffer, &uniformUpdate);

	Render_GraphicsEncoderBindDescriptorSet(encoder, render->descriptorSet, 0);
	Render_GraphicsEncoderBindIndexBuffer(encoder, render->indexBuffer, 0);
	Render_GraphicsEncoderBindVertexBuffer(encoder, render->vertexBuffer, 0);
	Render_GraphicsEncoderBindPipeline(encoder, render->pipeline);

	uint32_t const totalIndexCount = (world->width-1) * (world->height-1) * 3;
	Render_GraphicsEncoderDrawIndexed(encoder, totalIndexCount, 0, 0);
}

}; // end anon namespace

ALifeTests* ALifeTests::Create(Render_RendererHandle renderer, Render_ROPLayout const * targetLayout) {
	ALifeTests* alt = (ALifeTests*) MEMORY_CALLOC(1, sizeof(ALifeTests));
	if(!alt) {
		return nullptr;
	}

	alt->cudaCore = CUDACore_Create();

	alt->world2d = World2D_Create(WT_MOE, 64, 64);
	if(!alt->world2d) {
		Destroy(alt);
		return nullptr;
	}

	alt->worldRender = MakeRenderable(alt->world2d, renderer, targetLayout);
	if(!alt->worldRender){
		Destroy(alt);
		return nullptr;
	}

	return alt;
}

void ALifeTests::Destroy(ALifeTests* alt) {
	if(!alt) return;

	DestroyRenderable(alt->worldRender);
	World2D_Destroy(alt->world2d);
	CUDACore_Destroy(alt->cudaCore);
	MEMORY_FREE(alt);
}

void ALifeTests::update(double deltaMS, Render_View const& view) {
	if(worldRender) {
		worldRender->uniforms.view.worldToViewMatrix = Math_LookAtMat4F(view.position, view.lookAt, view.upVector);

		float const f = 1.0f / tanf(view.perspectiveFOV / 2.0f);
		worldRender->uniforms.view.viewToNDCMatrix = {
				f / view.perspectiveAspectWoverH, 0.0f, 0.0f, 0.0f,
				0.0f, f, 0.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f,
				0.0f, 0.0f, view.nearOffset, 0.0f
		};

		worldRender->uniforms.view.worldToNDCMatrix = Math_MultiplyMat4F(worldRender->uniforms.view.worldToViewMatrix, worldRender->uniforms.view.viewToNDCMatrix);
	}
}

void ALifeTests::render(Render_GraphicsEncoderHandle encoder) {
	if(worldRender) {
		RenderWorld2D(world2d, worldRender, encoder);
	}
}
