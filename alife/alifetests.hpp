// License Summary: MIT see LICENSE file
#pragma once

#include "render_basics/api.h"
#include "render_basics/view.h"
#include "world2d.hpp"

struct World2DRender {
	Render_RendererHandle renderer;
	Render_DescriptorSetHandle descriptorSet;
	Render_PipelineHandle pipeline;
	Render_ShaderHandle shader;
	Render_RootSignatureHandle rootSignature;

	union {
		Render_GpuView view;
		uint8_t spacer[UNIFORM_BUFFER_MIN_SIZE];
	} uniforms;

	Render_BufferHandle uniformBuffer;
	Render_BufferHandle indexBuffer;
	Render_BufferHandle vertexBuffer;

};

class ALifeTests {
public:
	static ALifeTests* Create(Render_RendererHandle renderer, Render_ROPLayout const * targetLayout);
	static void Destroy(ALifeTests* alt);

	void update(double deltaMS, Render_View const& view);
	void render(Render_GraphicsEncoderHandle encoder);

protected:

	World2D* world2d;
	World2DRender* worldRender;
	struct Cuda* accelCuda;
	struct Sycl* accelSycl;

};
