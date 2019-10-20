#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"
#include "al2o3_vfile/vfile.h"

#include "render_basics/theforge/handlemanager.h"
#include "render_basics/texture.h"
#include "render_basics/shader.h"
#include "render_basics/graphicsencoder.h"
#include "render_basics/rootsignature.h"
#include "render_basics/pipeline.h"
#include "synthwaveviztests.h"


typedef struct SynthWaveVizTests {

	Render_RendererHandle renderer;
	Render_TextureHandle colourTargetTexture;
	Render_TextureHandle depthTargetTexture;

	TinyImageFormat currentDestFormat;
	Render_RootSignatureHandle compositeRootSignature;
	Render_PipelineHandle compositePipeline;
	Render_ShaderHandle compositeShader;


} SynthWaveVizTests;

static bool CreateShaders(SynthWaveVizTests *vd) {
	static char const *const VertexShader = "struct VSOutput {\n"
																					"\tfloat4 Position 	: SV_Position;\n"
																					"\tfloat2 UV   			: TexCoord0;\n"
																					"};\n"
																					"\n"
																					"VSOutput VS_main(in uint vertexId : SV_VertexID)\n"
																					"{\n"
																					"    VSOutput result;\n"
																					"\n"
																					"\tresult.UV = float2(uint2(vertexId, vertexId << 1) & 2);\n"
																					"\tresult.Position = float4(lerp(float2(-1,1), float2(1, -1), result.UV), 0, 1);\n"
																					"\treturn result;\n"
																					"}";

	static char const *const FragmentShader = "struct FSInput {\n"
																						"\tfloat4 Position 	: SV_Position;\n"
																						"\tfloat2 UV   			: TexCoord0;\n"
																						"};\n"
																						"\n"
																						"float4 FS_main(FSInput input) : SV_Target\n"
																						"{\n"
																						"\treturn float4(input.UV,0,1);\n"
																						"}\n";

	static char const *const vertEntryPoint = "VS_main";
	static char const *const fragEntryPoint = "FS_main";

	VFile_Handle vfile = VFile_FromMemory(VertexShader, strlen(VertexShader) + 1, false);
	if (!vfile) {
		return false;
	}
	VFile_Handle ffile = VFile_FromMemory(FragmentShader, strlen(FragmentShader) + 1, false);
	if (!ffile) {
		VFile_Close(vfile);
		return false;
	}
	vd->compositeShader = Render_CreateShaderFromVFile(vd->renderer, vfile, "VS_main", ffile,"FS_main");

	VFile_Close(vfile);
	VFile_Close(ffile);

	return Render_ShaderHandleIsValid(vd->compositeShader);
}

AL2O3_EXTERN_C SynthWaveVizTestsHandle SynthWaveVizTests_Create(Render_RendererHandle renderer, uint32_t width, uint32_t height) {
	SynthWaveVizTests* svt = (SynthWaveVizTests*) MEMORY_CALLOC(1, sizeof(SynthWaveVizTests));
	svt->renderer = renderer;

	if(CreateShaders(svt) == false) {
		MEMORY_FREE(svt);
		return NULL;
	}

	SynthWaveVizTests_Resize(svt, width, height);

	Render_RootSignatureDesc rootSignatureDesc = {
			.shaderCount = 1,
			.shaders = { &svt->compositeShader },
			.staticSamplerCount = 0,
	};

	svt->compositeRootSignature = Render_RootSignatureCreate(renderer, &rootSignatureDesc);
	if (!Render_RootSignatureHandleIsValid(svt->compositeRootSignature)) {
		SynthWaveVizTests_Destroy(svt);
		return NULL;
	}


	return svt;
}

AL2O3_EXTERN_C void SynthWaveVizTests_Resize(SynthWaveVizTestsHandle ctx, uint32_t width, uint32_t height) {
	Render_TextureDestroy(ctx->renderer, ctx->colourTargetTexture);

	Render_TextureCreateDesc colourTargetDesc = {
			.format =TinyImageFormat_R10G10B10A2_UNORM,
			.usageflags = (Render_TextureUsageFlags) (Render_TUF_SHADER_READ | Render_TUF_ROP_WRITE | Render_TUF_ROP_READ),
			.width = width,
			.height = height,
			.depth = 1,
			.slices = 1,
			.sampleCount = 1,
			.sampleQuality = 1,
			.initialData = NULL,
			.debugName = "SynthWaveColourTarget",
			.renderTargetClearValue = { 1, 1, 0, 0}
	};

	ctx->colourTargetTexture = Render_TextureSyncCreate(ctx->renderer, &colourTargetDesc);

	Render_TextureCreateDesc depthTargetDesc = {
			.format =TinyImageFormat_D32_SFLOAT,
			.usageflags = (Render_TextureUsageFlags) (Render_TUF_ROP_WRITE | Render_TUF_ROP_READ),
			.width = width,
			.height = height,
			.depth = 1,
			.slices = 1,
			.sampleCount = 1,
			.sampleQuality = 1,
			.initialData = NULL,
			.debugName = "SynthWaveDepthTarget",
			.renderTargetClearValue = { .depth = 1.0f }
	};
	ctx->depthTargetTexture = Render_TextureSyncCreate(ctx->renderer, &depthTargetDesc);

}

AL2O3_EXTERN_C void SynthWaveVizTests_Destroy(SynthWaveVizTestsHandle ctx) {

	Render_PipelineDestroy(ctx->renderer, ctx->compositePipeline);
	Render_RootSignatureDestroy(ctx->renderer, ctx->compositeRootSignature);

	Render_ShaderDestroy(ctx->renderer, ctx->compositeShader);

	Render_TextureDestroy(ctx->renderer, ctx->depthTargetTexture);
	Render_TextureDestroy(ctx->renderer, ctx->colourTargetTexture);

	MEMORY_FREE(ctx);
}

AL2O3_EXTERN_C void SynthWaveVizTests_Update(SynthWaveVizTestsHandle ctx, double deltaMS) {

}

AL2O3_EXTERN_C void SynthWaveVizTests_Render(SynthWaveVizTestsHandle ctx, Render_GraphicsEncoderHandle encoder) {

	// no need to transition the depth texture at the moment...

	Render_TextureHandle renderTargets[] = { ctx->colourTargetTexture, ctx->depthTargetTexture };
	Render_TextureTransitionType const targetTransitions[] = { Render_TTT_RENDER_TARGET};
	Render_TextureTransitionType const textureTransitions[] = { RENDER_TTT_SHADER_ACCESS};

	Render_GraphicsEncoderTransition(encoder, 0, NULL, NULL,
																	 1, renderTargets, targetTransitions);

	Render_GraphicsEncoderBindRenderTargets(encoder,
																					2,
																					renderTargets,
																					true,
																					true,
																					true);
	// target bound and ready to go



	// transition after use
	Render_GraphicsEncoderBindRenderTargets(encoder, 0, NULL, false, false, false);
	Render_GraphicsEncoderTransition(encoder, 0, NULL, NULL,1, renderTargets, textureTransitions);

}
AL2O3_EXTERN_C void SynthWaveVizTests_Composite(SynthWaveVizTestsHandle ctx,
		Render_GraphicsEncoderHandle encoder,
		Render_TextureHandle dest ) {

	Render_TextureHandle renderTargets[] = { dest };
	Render_GraphicsEncoderBindRenderTargets(encoder,
																					1,
																					renderTargets,
																					false,
																					true,
																					true);

	TinyImageFormat destFormat = Render_TextureGetFormat(dest);
	if(destFormat != ctx->currentDestFormat) {
		if (	Render_PipelineHandleIsValid(ctx->compositePipeline)) {
			Render_PipelineHandleRelease(ctx->compositePipeline);
		}

		TinyImageFormat colourFormats[] = { destFormat };

		Render_GraphicsPipelineDesc compositeGfxPipeDesc = {
				.shader = ctx->compositeShader,
				.rootSignature = ctx->compositeRootSignature,
				.vertexLayout = Render_GetStockVertexLayout(ctx->renderer, Render_SVL_3D_COLOUR),
				.blendState = Render_GetStockBlendState(ctx->renderer, Render_SBS_PM_PORTER_DUFF),
				.depthState = Render_GetStockDepthState(ctx->renderer, Render_SDS_IGNORE),
				.rasteriserState = Render_GetStockRasterisationState(ctx->renderer, Render_SRS_NOCULL),
				.colourRenderTargetCount = 1,
				.colourFormats = colourFormats,
				.depthStencilFormat = TinyImageFormat_UNDEFINED,
				.sampleCount = 1,
				.sampleQuality = 0,
				.primitiveTopo = Render_PT_TRI_LIST
		};

		ctx->compositePipeline = Render_GraphicsPipelineCreate(ctx->renderer, &compositeGfxPipeDesc);
		if (!Render_PipelineHandleIsValid(ctx->compositePipeline)) {
			return;
		}
		ctx->currentDestFormat = destFormat;
	}

	Render_GraphicsEncoderBindPipeline(encoder, ctx->compositePipeline);
	Render_GraphicsEncoderDraw(encoder, 6, 0);

}