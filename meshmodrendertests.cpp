// License Summary: MIT see LICENSE file
#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"
#include "render_basics/framebuffer.h"
#include "render_meshmodshapes/shapes.h"
#include "render_meshmodrender/render.h"
#include "al2o3_cadt/vector.hpp"
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

	mmrt->meshVector = Cadt::Vector<MeshModRenderMesh>::Create();

	mmrt->registry = MeshMod_RegistryCreateWithDefaults();

	for(int i = 0;i < 3;i++) {
		{
			MeshModRenderMesh shape = {
					MeshModShapes_TetrahedonCreate(mmrt->registry),
					{-4, (float)i, 0},
					{1, 1, 1},
					{0, 0, 0},
			};
			shape.renderableMesh = MeshModRender_MeshCreate(mmrt->manager, shape.mesh);
			mmrt->meshVector->push(shape);
		}

		{
			MeshModRenderMesh shape = {
					MeshModShapes_CubeCreate(mmrt->registry),
					{-2, (float)i, 0},
					{1, 1, 1},
					{0, 0, 0},
			};
			shape.renderableMesh = MeshModRender_MeshCreate(mmrt->manager, shape.mesh);
			mmrt->meshVector->push(shape);
		}

		{
			MeshModRenderMesh shape = {
					MeshModShapes_OctahedronCreate(mmrt->registry),
					{0, (float)i, 0},
					{1, 1, 1},
					{0, 0, 0},
			};
			shape.renderableMesh = MeshModRender_MeshCreate(mmrt->manager, shape.mesh);
			mmrt->meshVector->push(shape);
		}

		{
			MeshModRenderMesh shape = {
					MeshModShapes_IcosahedronCreate(mmrt->registry),
					{2, (float)i, 0},
					{1, 1, 1},
					{0, 0, 0},
			};
			shape.renderableMesh = MeshModRender_MeshCreate(mmrt->manager, shape.mesh);
			mmrt->meshVector->push(shape);
		}

		{
			MeshModRenderMesh shape = {
					MeshModShapes_DodecahedronCreate(mmrt->registry),
					{4, (float)i, 0},
					{1, 1, 1},
					{0, 0, 0},
			};
			shape.renderableMesh = MeshModRender_MeshCreate(mmrt->manager, shape.mesh);
			mmrt->meshVector->push(shape);
		}

	}

	{
		MeshModRenderMesh shape = {
				MeshModShapes_DiamondCreate(mmrt->registry),
				{0,-2, 0},
				{1, 1, 1},
				{0, 0, 0},
		};
		shape.renderableMesh = MeshModRender_MeshCreate(mmrt->manager, shape.mesh);
		mmrt->meshVector->push(shape);
	}

	return mmrt;
}
void MeshModRenderTests::Destroy(MeshModRenderTests* mmrt) {

	if (mmrt->meshVector) {
		for (uint32_t i = 0u; i < mmrt->meshVector->size(); ++i) {
			auto mesh = mmrt->meshVector->at(i);

			MeshModRender_MeshDestroy(mmrt->manager, mesh.renderableMesh);
			MeshMod_MeshDestroy(mesh.mesh);
		}
		mmrt->meshVector->destroy();
	}

	MeshMod_RegistryDestroy(mmrt->registry);
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

	for (uint32_t i = 0u; i < meshVector->size(); ++i) {
		auto& mesh = meshVector->at(i);

		Math_Mat4F translateMatrix = Math_TranslationMat4F(mesh.pos);
		Math_Mat4F rotateMatrix = Math_RotateEulerXYZMat4F(mesh.eulerRots);
		Math_Mat4F scaleMatrix = Math_ScaleMat4F(mesh.scale);

		Math_Mat4F matrix = Math_MultiplyMat4F(translateMatrix, rotateMatrix);
		mesh.matrix = Math_MultiplyMat4F(matrix, scaleMatrix);

		Math_Mat4F inverseTranslateMatrix = Math_TranslationMat4F(Math_SubVec3F({0}, mesh.pos));
		Math_Mat4F inverseRotateMatrix = Math_RotateEulerXYZMat4F(Math_SubVec3F({0}, mesh.eulerRots));
//		Math_Mat4F inverseScaleMatrix = Math_ScaleMat4F(mesh.scale);
//		inverseScaleMatrix.v[0] = 1.0f / inverseScaleMatrix.v[0];
//		inverseScaleMatrix.v[5] = 1.0f / inverseScaleMatrix.v[5];
//		inverseScaleMatrix.v[9] = 1.0f / inverseScaleMatrix.v[9];

		Math_Mat4F inverseMatrix = Math_MultiplyMat4F(inverseTranslateMatrix, inverseRotateMatrix);
		mesh.inverseMatrix = inverseMatrix;//Math_MultiplyMat4F(inverseMatrix, inverseScaleMatrix);

		mesh.eulerRots.y += 0.005f * (float)deltaMS;
	}
}

void MeshModRenderTests::render(Render_GraphicsEncoderHandle encoder) {
	MeshModRender_ManagerSetView(manager, &gpuView);

	for (uint32_t i = 0u; i < meshVector->size(); ++i) {
		auto mesh = meshVector->at(i);

		MeshModRender_MeshUpdate(manager, mesh.renderableMesh);
		MeshModRender_MeshRender(manager, encoder, mesh.renderableMesh, mesh.matrix, mesh.inverseMatrix);
	}

}
void MeshModRenderTests::setStyle(MeshModRender_RenderStyle style) {
	for (uint32_t i = 0u; i < meshVector->size(); ++i) {
		auto mesh = meshVector->at(i);
		MeshModRender_MeshSetStyle(manager, mesh.renderableMesh, style);
	}
}