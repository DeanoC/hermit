// License Summary: MIT see LICENSE file
#pragma once

#include "render_basics/api.h"
#include "render_basics/view.h"
#include "render_meshmodrender/render.h"
class MeshModRenderTests {
public:
	static MeshModRenderTests* Create(Render_RendererHandle renderer, Render_FrameBufferHandle destination);
	static void Destroy(MeshModRenderTests* mmrt);

	void update(double deltaMS, Render_View const& view);
	void render(Render_GraphicsEncoderHandle encoder);

	void setStyle(MeshModRender_RenderStyle style);
protected:
	MeshModRender_Manager* manager;

	Math_Mat4F cubeMatrix;
	MeshMod_MeshHandle cubeMesh;
	MeshModRender_MeshHandle cubeRenderableMesh;

	Render_GpuView gpuView;
};


