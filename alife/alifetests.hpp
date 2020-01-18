// License Summary: MIT see LICENSE file
#pragma once

#include "render_basics/api.h"
#include "render_basics/view.h"
#include "world2d.hpp"

struct World2DRender {
	Render_RendererHandle renderer;
	Render_DescriptorSetHandle descriptorSet;
	Render_BufferHandle indexBuffer;
	Render_BufferHandle vertexBuffer;
	Render_PipelineHandle pipeline;
	Render_ShaderHandle shader;
	Render_RootSignatureHandle rootSignature;
};

class ALifeTests {
public:
	static ALifeTests* Create(Render_RendererHandle renderer, Render_ROPLayout const * targetLayout);
	static void Destroy(ALifeTests* alt);

	void update(double deltaMS, Render_View const& view);
	void render(Render_GraphicsEncoderHandle encoder);

protected:
//	Cadt::Vector<MeshModRenderMesh>* meshVector;
	Render_GpuView gpuView;

	World2D* world2d;
	World2DRender* worldRender;
	struct Cuda* cudaCore;

};
