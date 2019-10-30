#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"
#include "al2o3_vfile/vfile.h"

#include "render_basics/theforge/handlemanager.h"
#include "render_basics/texture.h"
#include "render_basics/shader.h"
#include "render_basics/graphicsencoder.h"
#include "render_basics/rootsignature.h"
#include "render_basics/pipeline.h"
#include "render_basics/descriptorset.h"
#include "tiny_imageformat/tinyimageformat_base.h"
#include "tiny_imageformat/tinyimageformat_encode.h"
#include "synthwaveviztests.h"

typedef struct SynthWaveVizTests {

	Render_RendererHandle renderer;
	Render_TextureHandle colourTargetTexture;
	Render_TextureHandle depthTargetTexture;

	TinyImageFormat currentDestFormat;
	Render_RootSignatureHandle compositeRootSignature;
	Render_DescriptorSetHandle compositeDescriptorSet;
	Render_PipelineHandle compositePipeline;
	Render_ShaderHandle compositeShader;

	Render_TextureHandle skyGradientTexture;
	Render_ShaderHandle skyGradientShader;
	Render_RootSignatureHandle skyGradientRootSignature;
	Render_DescriptorSetHandle skyGradientDescriptorSet;
	Render_PipelineHandle skyGradientPipeline;
} SynthWaveVizTests;

static bool CreateComposite(SynthWaveVizTests *svt) {
	static char const *const CompositeVertexShader = "struct VSOutput {\n"
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

	static char const *const CompositeFragmentShader = "struct FSInput {\n"
																										 "\tfloat4 Position 	: SV_Position;\n"
																										 "\tfloat2 UV   			: TexCoord0;\n"
																										 "};\n"
																										 "\n"
																										 "Texture2D colourTexture : register(t0, space0);\n"
																										 "SamplerState bilinearSampler : register(s0, space0);\n"
																										 "float4 FS_main(FSInput input) : SV_Target\n"
																										 "{\n"
																										 "\treturn colourTexture.Sample(bilinearSampler, input.UV);\n"
																										 "}\n";
	VFile_Handle compositevfile = VFile_FromMemory(CompositeVertexShader, strlen(CompositeVertexShader) + 1, false);
	VFile_Handle compositeffile = VFile_FromMemory(CompositeFragmentShader, strlen(CompositeFragmentShader) + 1, false);
	svt->compositeShader = Render_CreateShaderFromVFile(svt->renderer, compositevfile, "VS_main", compositeffile, "FS_main");
	VFile_Close(compositevfile);
	VFile_Close(compositeffile);

	if(!Render_ShaderHandleIsValid(svt->compositeShader)) {
		return false;
	}

	Render_SamplerHandle linearSampler = Render_GetStockSampler(svt->renderer, Render_SST_LINEAR);
	char const * linearSamplerName = "bilinearSampler";
	Render_RootSignatureDesc const rootSignatureDesc = {
			.shaderCount = 1,
			.shaders = &svt->compositeShader,
			.staticSamplerCount = 1,
			.staticSamplerNames = &linearSamplerName,
			.staticSamplers = &linearSampler
	};

	svt->compositeRootSignature = Render_RootSignatureCreate(svt->renderer, &rootSignatureDesc);
	if (!Render_RootSignatureHandleIsValid(svt->compositeRootSignature)) {
		return false;
	}
	Render_DescriptorSetDesc const compositeDSdesc = {
			.rootSignature = svt->compositeRootSignature,
			.updateFrequency = Render_DUF_NEVER,
			1
	};
	svt->compositeDescriptorSet = Render_DescriptorSetCreate(svt->renderer, &compositeDSdesc);
	if (!Render_DescriptorSetHandleIsValid(svt->compositeDescriptorSet)) {
		return false;
	}
	Render_DescriptorDesc const compositeSourceTexturedesc = {
			.name = "colourTexture",
			.type = Render_DT_TEXTURE,
			.texture = svt->colourTargetTexture,
	};
	Render_DescriptorUpdate(svt->compositeDescriptorSet, 0, 1, &compositeSourceTexturedesc);

	return true;
}

static bool CreateSkyGradient(SynthWaveVizTests *svt) {
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
																						"Texture1D colourTexture : register(t0, space0);\n"
																						"SamplerState bilinearSampler : register(s0, space0);\n"
																						"float4 FS_main(FSInput input) : SV_Target\n"
																						"{\n"
																						"\tfloat4 sample = colourTexture.Sample(bilinearSampler, 1-input.UV.y);\n"
																						"\treturn sample;\n"
																						"}\n";
	VFile_Handle vfile = VFile_FromMemory(VertexShader, strlen(VertexShader) + 1, false);
	VFile_Handle ffile = VFile_FromMemory(FragmentShader, strlen(FragmentShader) + 1, false);
	svt->skyGradientShader = Render_CreateShaderFromVFile(svt->renderer, vfile, "VS_main", ffile, "FS_main");
	VFile_Close(vfile);
	VFile_Close(ffile);

	if(!Render_ShaderHandleIsValid(svt->compositeShader)){
		return false;
	}

	// the main thing for the sky gradient is a 1D colour table
	// this is looked up via NDC y (-1 to 1) mapped to 0 to 1
	// start simple
	// -1 to 0 = black with green lines
	// 0 to 1 = purple to dark blue
	// TinyImageFormat_R10G10B10A2_UNORM to match background
	// LookUpTableSize samples

#define LookUpTableSize 1024
	uint32_t output[LookUpTableSize];
	TinyImageFormat_EncodeOutput encoded = {
			.pixel = output
	};

	float colourTable[4 * LookUpTableSize];

	// -1 to 0
	int greenCheck = ((LookUpTableSize/2) * (256/4)) + (90 * 256);
	int greenIncrement = (LookUpTableSize/2) * (256/2);
	int accum = 0;
	for(uint32_t i = 0;i < LookUpTableSize/2;++i) {
		if(accum >= greenCheck) {
			colourTable[i * 4 + 0] = 0;
			colourTable[i * 4 + 1] = 1;
			colourTable[i * 4 + 2] = 0;
			colourTable[i * 4 + 3] = 1;
			greenIncrement = (greenIncrement * 130) >> 8;
			greenCheck += greenIncrement;
		} else {
			colourTable[i * 4 + 0] = 0;
			colourTable[i * 4 + 1] = 0;
			colourTable[i * 4 + 2] = 0;
			colourTable[i * 4 + 3] = 1;
			accum += 256;
		}
	}
	for(uint32_t i = LookUpTableSize/2;i < (LookUpTableSize/2)+1;++i) {
		colourTable[i*4+0] = 1;
		colourTable[i*4+1] = 1;
		colourTable[i*4+2] = 1;
		colourTable[i*4+3] = 1;
	}
	// 0 to 1
	uint32_t startI = (LookUpTableSize/2)+1;
	for(uint32_t i = startI;i < LookUpTableSize;++i) {
		colourTable[i*4+0] = 1-(((float)i-startI)/(LookUpTableSize/2));
		colourTable[i*4+1] = 0.05f;
		colourTable[i*4+2] = 0.4f;
		colourTable[i*4+3] = 1;
	}

	bool encodeOkay = TinyImageFormat_EncodeLogicalPixelsF(TinyImageFormat_R10G10B10A2_UNORM, colourTable, LookUpTableSize, &encoded);
	if(!encodeOkay) {
		return false;
	}

	Render_TextureCreateDesc const createDesc = {
		.format = TinyImageFormat_R10G10B10A2_UNORM,
		.usageflags = Render_TUF_SHADER_READ,
		.width = LookUpTableSize,
		.height = 1,
		.depth = 1,
		.slices = 1,
		.mipLevels = 1,
		.sampleCount = 0,
		.sampleQuality = 0,
		.initialData = output,
		.debugName = "SynthWave sky gradient"
	};
	svt->skyGradientTexture = Render_TextureSyncCreate(svt->renderer, &createDesc);
	if(!Render_TextureHandleIsValid(svt->skyGradientTexture)) {
		return false;
	}

	Render_SamplerHandle linearSampler = Render_GetStockSampler(svt->renderer, Render_SST_LINEAR);
	char const * linearSamplerName = "bilinearSampler";
	Render_RootSignatureDesc const rootSignatureDesc = {
			.shaderCount = 1,
			.shaders = &svt->skyGradientShader,
			.staticSamplerCount = 1,
			.staticSamplerNames = &linearSamplerName,
			.staticSamplers = &linearSampler
	};

	svt->skyGradientRootSignature = Render_RootSignatureCreate(svt->renderer, &rootSignatureDesc);
	if (!Render_RootSignatureHandleIsValid(svt->skyGradientRootSignature)) {
		return false;
	}
	Render_DescriptorSetDesc const skyGradientDSdesc = {
			.rootSignature = svt->skyGradientRootSignature,
			.updateFrequency = Render_DUF_NEVER,
			1
	};
	svt->skyGradientDescriptorSet = Render_DescriptorSetCreate(svt->renderer, &skyGradientDSdesc);
	if (!Render_DescriptorSetHandleIsValid(svt->skyGradientDescriptorSet)) {
		return false;
	}
	Render_DescriptorDesc const skyGradientSourceTexturedesc = {
			.name = "colourTexture",
			.type = Render_DT_TEXTURE,
			.texture = svt->skyGradientTexture,
	};
	Render_DescriptorUpdate(svt->skyGradientDescriptorSet, 0, 1, &skyGradientSourceTexturedesc);

	TinyImageFormat colourFormats[] = {TinyImageFormat_R10G10B10A2_UNORM};

	Render_GraphicsPipelineDesc skyGradientGfxPipeDesc = {
			.shader = svt->skyGradientShader,
			.rootSignature = svt->skyGradientRootSignature,
			.vertexLayout = NULL,
			.blendState = Render_GetStockBlendState(svt->renderer, Render_SBS_OPAQUE),
			.depthState = Render_GetStockDepthState(svt->renderer, Render_SDS_IGNORE),
			.rasteriserState = Render_GetStockRasterisationState(svt->renderer, Render_SRS_NOCULL),
			.colourRenderTargetCount = 1,
			.colourFormats = colourFormats,
			.depthStencilFormat = TinyImageFormat_UNDEFINED,
			.sampleCount = 1,
			.sampleQuality = 0,
			.primitiveTopo = Render_PT_TRI_LIST
	};

	svt->skyGradientPipeline = Render_GraphicsPipelineCreate(svt->renderer, &skyGradientGfxPipeDesc);
	if (!Render_PipelineHandleIsValid(svt->skyGradientPipeline)) {
		return false;
	}

	return true;
}

AL2O3_EXTERN_C SynthWaveVizTestsHandle SynthWaveVizTests_Create(Render_RendererHandle renderer,
																																uint32_t width,
																																uint32_t height) {
	SynthWaveVizTests *svt = (SynthWaveVizTests *) MEMORY_CALLOC(1, sizeof(SynthWaveVizTests));
	svt->renderer = renderer;

	SynthWaveVizTests_Resize(svt, width, height);

	if (CreateComposite(svt) == false) {
		SynthWaveVizTests_Destroy(svt);
		return NULL;
	}

	if( CreateSkyGradient(svt) == false) {
		SynthWaveVizTests_Destroy(svt);
		return NULL;
	}

	return svt;
}

AL2O3_EXTERN_C void SynthWaveVizTests_Resize(SynthWaveVizTestsHandle ctx, uint32_t width, uint32_t height) {
	Render_TextureDestroy(ctx->renderer, ctx->colourTargetTexture);

	Render_TextureCreateDesc colourTargetDesc = {
			.format = TinyImageFormat_R10G10B10A2_UNORM,
			.usageflags = (Render_TextureUsageFlags) (Render_TUF_SHADER_READ | Render_TUF_ROP_WRITE | Render_TUF_ROP_READ),
			.width = width,
			.height = height,
			.depth = 1,
			.slices = 1,
			.sampleCount = 1,
			.sampleQuality = 1,
			.initialData = NULL,
			.debugName = "SynthWaveColourTarget",
			.renderTargetClearValue = {0, 0, 0, 1}
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
			.renderTargetClearValue = {.depth = 1.0f}
	};
	ctx->depthTargetTexture = Render_TextureSyncCreate(ctx->renderer, &depthTargetDesc);

}

AL2O3_EXTERN_C void SynthWaveVizTests_Destroy(SynthWaveVizTestsHandle ctx) {

	Render_DescriptorSetDestroy(ctx->renderer, ctx->skyGradientDescriptorSet);
	Render_PipelineDestroy(ctx->renderer, ctx->skyGradientPipeline);
	Render_RootSignatureDestroy(ctx->renderer, ctx->skyGradientRootSignature);
	Render_ShaderDestroy(ctx->renderer, ctx->skyGradientShader);
	Render_TextureDestroy(ctx->renderer, ctx->skyGradientTexture);

	Render_DescriptorSetDestroy(ctx->renderer, ctx->compositeDescriptorSet);
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

	Render_TextureHandle renderTargets[] = {ctx->colourTargetTexture, ctx->depthTargetTexture};
	Render_TextureTransitionType const targetTransitions[] = {Render_TTT_RENDER_TARGET};
	Render_TextureTransitionType const textureTransitions[] = {RENDER_TTT_SHADER_ACCESS};

	Render_GraphicsEncoderTransition(encoder, 0, NULL, NULL,
																	 1, renderTargets, targetTransitions);

	Render_GraphicsEncoderBindRenderTargets(encoder,
																					2,
																					renderTargets,
																					true,
																					true,
																					true);
	// target bound and ready to go

	Render_GraphicsEncoderBindDescriptorSet(encoder, ctx->skyGradientDescriptorSet, 0);
	Render_GraphicsEncoderBindPipeline(encoder, ctx->skyGradientPipeline);
	Render_GraphicsEncoderDraw(encoder, 3, 0);


	// transition after use
	Render_GraphicsEncoderBindRenderTargets(encoder, 0, NULL, false, false, false);
	Render_GraphicsEncoderTransition(encoder, 0, NULL, NULL, 1, renderTargets, textureTransitions);

}

AL2O3_EXTERN_C void SynthWaveVizTests_Composite(SynthWaveVizTestsHandle ctx,
																								Render_GraphicsEncoderHandle encoder,
																								Render_TextureHandle dest) {

	TinyImageFormat destFormat = Render_TextureGetFormat(dest);

	if (destFormat != ctx->currentDestFormat) {
		Render_PipelineDestroy(ctx->renderer, ctx->compositePipeline);

		TinyImageFormat colourFormats[] = {destFormat};

		Render_GraphicsPipelineDesc compositeGfxPipeDesc = {
				.shader = ctx->compositeShader,
				.rootSignature = ctx->compositeRootSignature,
				.vertexLayout = NULL,
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
			LOGERROR("Pipeline failed creation");
			return;
		}
		ctx->currentDestFormat = destFormat;
	}

	Render_TextureHandle renderTargets[] = {dest};
	Render_GraphicsEncoderBindRenderTargets(encoder,
																					1,
																					renderTargets,
																					false,
																					true,
																					true);
	Render_GraphicsEncoderBindDescriptorSet(encoder, ctx->compositeDescriptorSet, 0);
	Render_GraphicsEncoderBindPipeline(encoder, ctx->compositePipeline);
	Render_GraphicsEncoderDraw(encoder, 3, 0);

}