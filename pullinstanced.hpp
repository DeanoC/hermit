#pragma once

#include "render_basics/api.h"
#include "render_basics/view.h"
#include "al2o3_cmath/vector.hpp"
#include "al2o3_cadt/vector.hpp"

class PullInstanced {
public:
	static PullInstanced* Create(Render_RendererHandle renderer, Render_FrameBufferHandle destination);
	static void Destroy(PullInstanced* pi);

};
