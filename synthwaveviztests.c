#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"
#include "al2o3_platform/visualdebug.h"
#include "al2o3_cmath/vector.h"
#include "render_basics/api.h"
#include "render_basics/texture.h"
#include "synthwaveviztests.h"
#include "render_basics/graphicsencoder.h"


typedef struct SynthWaveVizTests {

	Render_RendererHandle renderer;
	Render_TextureHandle colourTargetTexture;
	Render_TextureHandle depthTargetTexture;

} SynthWaveVizTests;

AL2O3_EXTERN_C SynthWaveVizTestsHandle SynthWaveVizTests_Create(Render_RendererHandle renderer, uint32_t width, uint32_t height) {
	SynthWaveVizTests* mem = (SynthWaveVizTests*) MEMORY_CALLOC(0, sizeof(SynthWaveVizTests));
	mem->renderer = renderer;

	SynthWaveVizTests_Resize(mem, width, height);

	return mem;
}

AL2O3_EXTERN_C void SynthWaveVizTests_Resize(SynthWaveVizTestsHandle ctx, uint32_t width, uint32_t height) {
	Render_TextureDestroy(ctx->renderer, ctx->colourTargetTexture);

	Render_TextureCreateDesc colourTargetDesc = {
			.format =TinyImageFormat_B8G8R8A8_SRGB,
			.usageflags = (Render_TextureUsageFlags) (Render_TUF_SHADER_READ | Render_TUF_ROP_WRITE | Render_TUF_ROP_READ),
			.width = width,
			.height = height,
			.depth = 1,
			.slices = 1,
			.sampleCount = 4,
			.sampleQuality = 1,
			.initialData = NULL,
			.debugName = "SynthWaveColourTarget",
			.renderTargetClearValue = { 1, 1, 0, 1}
	};

	ctx->colourTargetTexture = Render_TextureSyncCreate(ctx->renderer, &colourTargetDesc);

	Render_TextureCreateDesc depthTargetDesc = {
			.format =TinyImageFormat_D32_SFLOAT,
			.usageflags = (Render_TextureUsageFlags) (Render_TUF_ROP_WRITE | Render_TUF_ROP_READ),
			.width = width,
			.height = height,
			.depth = 1,
			.slices = 1,
			.sampleCount = 4,
			.sampleQuality = 1,
			.initialData = NULL,
			.debugName = "SynthWaveDepthTarget",
			.renderTargetClearValue = { .depth = 1.0f }
	};
	ctx->depthTargetTexture = Render_TextureSyncCreate(ctx->renderer, &depthTargetDesc);

}

AL2O3_EXTERN_C void SynthWaveVizTests_Destroy(SynthWaveVizTestsHandle ctx) {

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


	Render_GraphicsEncoderBindRenderTargets(encoder, 0, NULL, false, false, false);

	Render_GraphicsEncoderTransition(encoder, 0, NULL, NULL,
																	 1, renderTargets, textureTransitions);

}

AL2O3_EXTERN_C Render_TextureHandle SynthWaveVizTests_ColourTarget(SynthWaveVizTestsHandle ctx) {
	return ctx->colourTargetTexture;
}
