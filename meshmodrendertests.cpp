// License Summary: MIT see LICENSE file
#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"
#include "render_basics/framebuffer.h"
#include "render_meshmodshapes/shapes.h"
#include "render_meshmodrender/render.h"
#include "meshmodrendertests.hpp"

MeshModRenderTests* MeshModRenderTests::Create(Render_RendererHandle renderer, Render_FrameBufferHandle frameBuffer) {

	MeshModRenderTests* mmrt = (MeshModRenderTests*) MEMORY_CALLOC(1, sizeof(MeshModRenderTests));
	if(!mmrt) {
		return nullptr;
	}
	mmrt->manager = MeshModRender_ManagerCreate(renderer, Render_FrameBufferColourFormat(frameBuffer), TinyImageFormat_UNDEFINED);
	if(!mmrt->manager) {
		Destroy(mmrt);
		return nullptr;
	}

	mmrt->cubeMesh = MeshModShapes_CubeCreate({0});
	mmrt->cubeRenderableMesh = MeshModRender_MeshCreate(mmrt->manager, mmrt->cubeMesh);

	return mmrt;
}
void MeshModRenderTests::Destroy(MeshModRenderTests* mmrt) {

	MeshModRender_MeshDestroy(mmrt->manager, mmrt->cubeRenderableMesh);
	MeshMod_MeshDestroy(mmrt->cubeMesh);
	MeshModRender_ManagerDestroy(mmrt->manager);

	MEMORY_FREE(mmrt);
}

void MeshModRenderTests::update(double deltaMS, Render_View const& view) {
	gpuView.worldToViewMatrix =	Math_LookAtMat4F(view.position, view.lookAt, view.upVector);

	float const f = 1.0f / tanf(view.perspectiveFOV / 2.0f);
	gpuView.viewToNDCMatrix = {
			f / view.perspectiveAspectWoverH, 0.0f, 0.0f, 0.0f,
			0.0f, f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f,
			0.0f, 0.0f, view.nearOffset, 0.0f
	};

	gpuView.worldToNDCMatrix = Math_MultiplyMat4F(gpuView.worldToViewMatrix, gpuView.viewToNDCMatrix);
}

void MeshModRenderTests::render(Render_GraphicsEncoderHandle encoder) {
	MeshModRender_ManagerSetView(manager, &gpuView);

	MeshModRender_MeshUpdate(manager, cubeRenderableMesh);
	MeshModRender_MeshRender(manager, encoder, cubeRenderableMesh);
}
