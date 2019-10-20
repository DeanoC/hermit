#pragma once

#include "render_basics/api.h"

// forward decl
typedef struct Render_Renderer * Render_RendererHandle;


typedef struct SynthWaveVizTests *SynthWaveVizTestsHandle;

AL2O3_EXTERN_C SynthWaveVizTestsHandle SynthWaveVizTests_Create(Render_RendererHandle renderer, uint32_t width, uint32_t height);
AL2O3_EXTERN_C void SynthWaveVizTests_Destroy(SynthWaveVizTestsHandle ctx);
AL2O3_EXTERN_C void SynthWaveVizTests_Resize(SynthWaveVizTestsHandle ctx, uint32_t width, uint32_t height);

AL2O3_EXTERN_C void SynthWaveVizTests_Update(SynthWaveVizTestsHandle ctx, double deltaMS);
AL2O3_EXTERN_C void SynthWaveVizTests_Render(SynthWaveVizTestsHandle ctx, Render_GraphicsEncoderHandle encoder);
AL2O3_EXTERN_C void SynthWaveVizTests_Composite(SynthWaveVizTestsHandle ctx, Render_GraphicsEncoderHandle encoder, Render_TextureHandle dest);
AL2O3_EXTERN_C Render_TextureHandle SynthWaveVizTests_ColourTarget(SynthWaveVizTestsHandle ctx);
