// License Summary: MIT see LICENSE file
#pragma once

#include "render_basics/api.h"
#include "render_basics/view.h"
#include "render_meshmodrender/render.h"
#include "al2o3_cmath/vector.hpp"
#include "al2o3_cadt/vector.hpp"

struct MeshModRenderMesh {
	MeshMod_MeshHandle mesh;

	Math::Vec3F pos;
	Math::Vec3F scale;
	Math::Vec3F eulerRots;

	MeshModRender_MeshHandle renderableMesh;
	Math_Mat4F matrix;
	Math_Mat4F inverseMatrix;
};

class MeshModRenderTests {
public:
	static MeshModRenderTests* Create(Render_RendererHandle renderer, Render_FrameBufferHandle destination);
	static void Destroy(MeshModRenderTests* mmrt);

	void update(double deltaMS, Render_View const& view);
	void render(Render_GraphicsEncoderHandle encoder);

	void setStyle(MeshModRender_RenderStyle style);
protected:
	MeshModRender_Manager* manager;

	Cadt::Vector<MeshModRenderMesh>* meshVector;
	MeshMod_RegistryHandle registry;

	Render_GpuView gpuView;
};


