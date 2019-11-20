// License Summary: MIT see LICENSE file
#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"
#include "render_basics/framebuffer.h"
#include "render_meshmodshapes/shapes.h"
#include "render_meshmodrender/render.h"
#include "al2o3_cadt/vector.hpp"
#include "pullinstanced.hpp"

PullInstanced* PullInstanced::Create(Render_RendererHandle renderer, Render_FrameBufferHandle destination) {

	auto pi = (PullInstanced*) MEMORY_CALLOC(1, sizeof(PullInstanced));
	if(!pi) {
		return nullptr;
	}

	auto mmesh = MeshModShapes_DodecahedronCreate({0});

	ps->shader = CreateShaders(vd);
	if(!Render_ShaderHandleIsValid(ps->shader)) {
		RenderTF_PlatonicSolidsDestroy(vd);
		return false;
	}
	static Render_VertexLayout vertexLayout{
			6,
			{
					{TheForge_SS_POSITION, 8, "POSITION", TinyImageFormat_R32G32B32_SFLOAT, 0, 0, 0, TheForge_VAR_VERTEX},
					{TheForge_SS_NORMAL, 9, "NORMAL", TinyImageFormat_R32G32B32_SFLOAT, 0, 1, sizeof(float) * 3, TheForge_VAR_VERTEX},
					{TheForge_SS_COLOR, 5, "COLOR", TinyImageFormat_R8G8B8A8_UNORM, 0, 2, sizeof(float) * 6, TheForge_VAR_VERTEX},

					{TheForge_SS_TEXCOORD0, 12, "INSTANCEROW", TinyImageFormat_R32G32B32A32_SFLOAT, 1, 0, 0, TheForge_VAR_INSTANCE },
					{TheForge_SS_TEXCOORD1, 12, "INSTANCEROW", TinyImageFormat_R32G32B32A32_SFLOAT, 1, 1, sizeof(float)*4, TheForge_VAR_INSTANCE },
					{TheForge_SS_TEXCOORD2, 12, "INSTANCEROW", TinyImageFormat_R32G32B32A32_SFLOAT, 1, 2, sizeof(float)*8, TheForge_VAR_INSTANCE }
			}
	};
	TinyImageFormat colourFormats[] = {Render_FrameBufferColourFormat(vd->target)};

	Render_GraphicsPipelineDesc platonicPipeDesc{};
	platonicPipeDesc.shader = ps->shader;
	platonicPipeDesc.rootSignature = vd->rootSignature;
	platonicPipeDesc.vertexLayout = (Render_VertexLayoutHandle) &vertexLayout;
	platonicPipeDesc.blendState = Render_GetStockBlendState(vd->renderer, Render_SBS_OPAQUE);
	platonicPipeDesc.depthState = Render_GetStockDepthState(vd->renderer, Render_SDS_IGNORE);
	platonicPipeDesc.rasteriserState = Render_GetStockRasterisationState(vd->renderer, Render_SRS_BACKCULL);
	platonicPipeDesc.colourRenderTargetCount = 1;
	platonicPipeDesc.colourFormats = colourFormats;
	platonicPipeDesc.depthStencilFormat = TinyImageFormat_UNDEFINED;
	platonicPipeDesc.sampleCount = 1;
	platonicPipeDesc.sampleQuality = 0;
	platonicPipeDesc.primitiveTopo = Render_PT_TRI_LIST;
	ps->pipeline = Render_GraphicsPipelineCreate(vd->renderer, &platonicPipeDesc);
	if (!Render_PipelineHandleIsValid(ps->pipeline)) {
		RenderTF_PlatonicSolidsDestroy(vd);
		return false;
	}

	// todo use temp allocator
	CADT_VectorHandle vertices = CADT_VectorCreate(sizeof(Vertex));

	uint32_t curIndex = 0;

	ps->solids[(int)SolidType::Tetrahedron].startVertex = curIndex;
	ps->solids[(int)SolidType::Tetrahedron].vertexCount = CreateTetrahedon(vertices);
	curIndex += ps->solids[(int)SolidType::Tetrahedron].vertexCount;

	ps->solids[(int)SolidType::Cube].startVertex = curIndex;
	ps->solids[(int)SolidType::Cube].vertexCount = CreateCube(vertices);
	curIndex += ps->solids[(int)SolidType::Cube].vertexCount;

	ps->solids[(int)SolidType::Octahedron].startVertex = curIndex;
	ps->solids[(int)SolidType::Octahedron].vertexCount = CreateOctahedron(vertices);
	curIndex += ps->solids[(int)SolidType::Octahedron].vertexCount;

	ps->solids[(int)SolidType::Icosahedron].startVertex = curIndex;
	ps->solids[(int)SolidType::Icosahedron].vertexCount = CreateIcosahedron(vertices);
	curIndex += ps->solids[(int)SolidType::Icosahedron].vertexCount;

	ps->solids[(int)SolidType::Dodecahedron].startVertex = curIndex;
	ps->solids[(int)SolidType::Dodecahedron].vertexCount = CreateDodecahedron(vertices);
	curIndex += ps->solids[(int)SolidType::Dodecahedron].vertexCount;

	Render_BufferVertexDesc vbDesc{
			(uint32_t) CADT_VectorSize(vertices),
			sizeof(Vertex),
			false
	};

	ps->gpuVertexData = Render_BufferCreateVertex(vd->renderer, &vbDesc);
	if (!Render_BufferHandleIsValid(ps->gpuVertexData)) {
		RenderTF_PlatonicSolidsDestroy(vd);
		return false;
	}

	// upload the vertex data
	Render_BufferUpdateDesc vertexUpdate = {
			CADT_VectorData(vertices),
			0,
			sizeof(Vertex) * CADT_VectorSize(vertices)
	};
	Render_BufferUpload(ps->gpuVertexData, &vertexUpdate);

	CADT_VectorDestroy(vertices);

	for(auto i=0;i < (int)SolidType::COUNT;++i) {
		ps->solids[i].instanceData = CADT_VectorCreate(sizeof(Instance));
	}


	return pi;
}

void PullInstanced::Destroy(PullInstanced * pi) {
	if(!pi) {
		return;
	}

	MEMORY_FREE(pi);
}