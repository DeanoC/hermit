#include <cstdio>
#include "al2o3_platform/platform.h"
#include "al2o3_platform/visualdebug.h"
#include "al2o3_memory/memory.h"
#include "al2o3_cmath/scalar.hpp"
#include "utils_gameappshell/gameappshell.h"
#include "utils_simple_logmanager/logmanager.h"

#include "render_basics/theforge/api.h"
#include "render_basics/api.h"
#include "render_basics/view.h"
#include "render_basics/framebuffer.h"

#include "al2o3_enki/TaskScheduler_c.h"
#include "gfx_theforge/theforge.h"
#include "input_basic/input.h"
#include "gfx_imgui/imgui.h"

#include "al2o3_os/filesystem.h"

#include "synthwaveviztests.h"
#include "meshmodrendertests.hpp"
#include "alife/alifetests.hpp"

extern void VisualDebugTests();

SimpleLogManager_Handle g_logger;
int g_returnCode;

bool bDoVisualDebugTests = false;

bool bDoSynthWaveVizTests = false;
SynthWaveVizTestsHandle synthWaveVizTests;

bool bDoMeshModRenderTests = false;
MeshModRender_RenderStyle meshModRenderStyle;
MeshModRenderTests* meshModRenderTests;

bool bDoALifeTests = false;
ALifeTests* alifeTests = nullptr;

Render_RendererHandle renderer;
Render_FrameBufferHandle frameBuffer;

InputBasic_ContextHandle input;
InputBasic_KeyboardHandle keyboard;
InputBasic_MouseHandle mouse;

enkiTaskSchedulerHandle taskScheduler;

enum AppKey {
	AppKey_Quit,
	AppKey_GPUCapture,
	AppKey_SlideLeft,
	AppKey_SlideRight,
	AppKey_Forward,
	AppKey_Back,
	AppKey_XAxis,
	AppKey_YAxis,
	AppKey_PrimaryButton
};

enum class GpuCaptureState {
	NotCapturing,
	StartCapturing,
	Capturing
};

GpuCaptureState gpuCaptureState = GpuCaptureState::NotCapturing;

static void *EnkiAlloc(void *userData, size_t size) {
	return MEMORY_ALLOCATOR_MALLOC((Memory_Allocator *) userData, size);
}
static void EnkiFree(void *userData, void *ptr) {
	MEMORY_ALLOCATOR_FREE((Memory_Allocator *) userData, ptr);
}

static void ShowFileMenu() {
	ImGui::Separator();
	if (ImGui::MenuItem("Quit", "Alt+F4")) {
		GameAppShell_Quit();
	}
}

static void ShowTests() {
	ImGui::Separator();
	ImGui::Checkbox("Visual Debug Tests", &bDoVisualDebugTests);
	ImGui::Checkbox("SynthWave viz tests", &bDoSynthWaveVizTests);
	ImGui::Checkbox("MeshMod render tests", &bDoMeshModRenderTests);
	ImGui::Checkbox("ALife tests", &bDoALifeTests);
}

static void ShowMeshModRenderStyles() {
	ImGui::Separator();
	auto backup = meshModRenderStyle;
	ImGui::RadioButton("Face Colours", (int*)&meshModRenderStyle, (int)MMR_RS_FACE_COLOURS);
	ImGui::RadioButton("Tri Colours", (int*)&meshModRenderStyle, (int)MMR_RS_TRIANGLE_COLOURS);
	ImGui::RadioButton("Normal as Colour", (int*)&meshModRenderStyle, (int)MMR_RS_NORMAL);
	ImGui::RadioButton("Dot Colour", (int*)&meshModRenderStyle, (int)MMR_RS_DOT);
	if(backup != meshModRenderStyle) {
		meshModRenderTests->setStyle(meshModRenderStyle);
	}
}

static void ShowAppMainMenuBar() {
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			ShowFileMenu();
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Tests")) {
			ShowTests();
			ImGui::EndMenu();
		}
		if(bDoMeshModRenderTests && ImGui::BeginMenu("Styles")) {
			ShowMeshModRenderStyles();
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}

static Math::Vec3F viewPosition = {0, 0, -10};
static Math::Vec3F viewLookAt = {0, 0, -9};
static Math::Vec3F viewWorldUp = { 0, 1, 0 };

static void CameraInfoWindow() {
	ImGui::Begin("Camera Info");
	ImGui::LabelText("Position", "%f, %f, %f", viewPosition.x, viewPosition.y, viewPosition.z);
	ImGui::LabelText("Look At", "%f, %f, %f", viewLookAt.x, viewLookAt.y, viewLookAt.z);
	ImGui::End();
}

static bool Init() {

#if AL2O3_PLATFORM == AL2O3_PLATFORM_APPLE_MAC
	//	Os_SetCurrentDir("../.."); // for xcode, no idea...
	Os_SetCurrentDir("..");
	char currentDir[2048];
	Os_GetCurrentDir(currentDir, 2048);
	LOGINFO(currentDir);
#endif

	MeshMod_StartUp();

	// setup basic input and map quit key
	input = InputBasic_Create();
	uint32_t userIdBlk = InputBasic_AllocateUserIdBlock(input); // 1st 1000 id are the apps
	ASSERT(userIdBlk == 0);

	renderer = Render_RendererCreate(input);
	if (!renderer) {
		LOGERROR("Render_RendererCreate failed");
		return false;
	}

	taskScheduler = enkiNewTaskScheduler(&EnkiAlloc, &EnkiFree, &Memory_GlobalAllocator);

	GameAppShell_WindowDesc windowDesc;
	GameAppShell_WindowGetCurrentDesc(&windowDesc);

	if (InputBasic_GetKeyboardCount(input) > 0) {
		keyboard = InputBasic_KeyboardCreate(input, 0);
	}
	if (InputBasic_GetMouseCount(input) > 0) {
		mouse = InputBasic_MouseCreate(input, 0);
	}

	if (keyboard) {
		InputBasic_MapToKey(input, AppKey_Quit, keyboard, InputBasic_Key_Escape);
		InputBasic_MapToKey(input, AppKey_GPUCapture, keyboard, InputBasic_Key_Tab);
		InputBasic_MapToKey(input, AppKey_SlideLeft, keyboard, InputBasic_Key_Left);
		InputBasic_MapToKey(input, AppKey_SlideRight, keyboard, InputBasic_Key_Right);
		InputBasic_MapToKey(input, AppKey_Forward, keyboard, InputBasic_Key_Up);
		InputBasic_MapToKey(input, AppKey_Back, keyboard, InputBasic_Key_Down);
	}
	if(mouse) {
		InputBasic_MapToMouseAxis(input, AppKey_XAxis, mouse, InputBasis_Axis_X);
		InputBasic_MapToMouseAxis(input, AppKey_YAxis, mouse, InputBasis_Axis_Y);
		InputBasic_MapToMouseButton(input, AppKey_PrimaryButton, mouse, InputBasic_MouseButton_Left);
	}

	InputBasic_SetWindowSize(input, windowDesc.width, windowDesc.height);

	Render_FrameBufferDesc fbDesc{};
	fbDesc.platformHandle = GameAppShell_GetPlatformWindowPtr();
	fbDesc.queue = Render_RendererGetPrimaryQueue(renderer, Render_QT_GRAPHICS);
	fbDesc.frameBufferWidth = windowDesc.width;
	fbDesc.frameBufferHeight = windowDesc.height;
	fbDesc.colourFormat = TinyImageFormat_UNDEFINED;
	fbDesc.embeddedImgui = true;
	fbDesc.visualDebugTarget = true;
	frameBuffer = Render_FrameBufferCreate(renderer, &fbDesc);

	return true;
}

static void Resize() {
	if(frameBuffer.handle.handle == 0)
		return;

	if(!Render_FrameBufferHandleIsValid(frameBuffer))
		return;

	GameAppShell_WindowDesc windowDesc;
	GameAppShell_WindowGetCurrentDesc(&windowDesc);

	// if we have a 0 sized windows, we should halt but for now just make ickle
	// 8x8 render targets
	if(windowDesc.width == 0) {
		windowDesc.width = 8;
	}
	if(windowDesc.height == 0) {
		windowDesc.height = 8;
	}

	Render_FrameBufferResize(frameBuffer, windowDesc.width, windowDesc.height);
	InputBasic_SetWindowSize(input, windowDesc.width, windowDesc.height);
	if(synthWaveVizTests) {
		SynthWaveVizTests_Resize(synthWaveVizTests, windowDesc.width, windowDesc.height);
	}
}

static void Update(double deltaMS) {
	GameAppShell_WindowDesc windowDesc;
	GameAppShell_WindowGetCurrentDesc(&windowDesc);

	InputBasic_Update(input, deltaMS);
	Render_FrameBufferUpdate(frameBuffer,
													 windowDesc.width, windowDesc.height,
													 deltaMS);

	if (InputBasic_GetAsBool(input, AppKey_Quit)) {
		GameAppShell_Quit();
	}
	if (InputBasic_GetAsBool(input, AppKey_GPUCapture)) {
		gpuCaptureState = GpuCaptureState::StartCapturing;
	}

	using namespace Math;

	auto viewMatrix = Math_LookAtMat4F(viewPosition, viewLookAt, viewWorldUp);
	Vec3F forwardVec = Vec3F::From(viewMatrix.col[2].v);
	Vec3F slideVec = Vec3F::From(viewMatrix.col[0].v);

	float const forwardRate = 0.002f * (float)deltaMS;
	float const backRate = 0.001f * (float)deltaMS;
	float const slideRate = 0.001f * (float)deltaMS;
	float const turnRate = 0.001f * PiF() * (float)deltaMS;

	Vec3F const forwardVel = forwardVec * forwardRate;
	Vec3F const backVel = forwardVec * -backRate;
	Vec3F const slideVel = slideVec * slideRate;


	Render_View view{
			viewPosition,
			viewLookAt,
			viewWorldUp,

			Math_DegreesToRadiansF(45.0f),
			(float) windowDesc.width / (float) windowDesc.height,
			1, 10000
	};
	Render_SetFrameBufferDebugView(frameBuffer, &view);


	if(bDoVisualDebugTests) {
		VisualDebugTests();
	}

	if(bDoSynthWaveVizTests) {
		if(!synthWaveVizTests) {
			synthWaveVizTests = SynthWaveVizTests_Create(renderer, windowDesc.width, windowDesc.height);
			if(!synthWaveVizTests) {
				LOGERROR("SynthWaveVizTests_Create failed");
				bDoSynthWaveVizTests = false;
			}
		}

		if(synthWaveVizTests) {
			SynthWaveVizTests_Update(synthWaveVizTests, deltaMS);
		}
	} else {
		if(synthWaveVizTests) {
			SynthWaveVizTests_Destroy(synthWaveVizTests);
			synthWaveVizTests = nullptr;
		}
	}

	if(bDoMeshModRenderTests) {
		if(!meshModRenderTests) {
			Render_ROPLayout ropLayout;
			Render_FrameBufferDescribeROPLayout(frameBuffer, &ropLayout);
			meshModRenderTests = MeshModRenderTests::Create(renderer, &ropLayout);
			if(!meshModRenderTests) {
				LOGERROR("MeshModRenderTests::Create failed");
				bDoMeshModRenderTests = false;
			}
			meshModRenderTests->setStyle(meshModRenderStyle);
		}

		if(meshModRenderTests) {
			meshModRenderTests->update(deltaMS, view);
		}
	} else {
		if(meshModRenderTests) {
			MeshModRenderTests::Destroy(meshModRenderTests);
			meshModRenderTests = nullptr;
		}
	}

	if(bDoALifeTests) {
		if(!alifeTests) {
			Render_ROPLayout ropLayout;
			Render_FrameBufferDescribeROPLayout(frameBuffer, &ropLayout);
			alifeTests = ALifeTests::Create(renderer, &ropLayout);
			if(!alifeTests) {
				LOGERROR("ALifeTest::Create failed");
				bDoALifeTests = false;
			}
		}

		if(alifeTests) {
			alifeTests->update(deltaMS, view);
		}
	} else {
		if(alifeTests) {
			ALifeTests::Destroy(alifeTests);
			alifeTests = nullptr;
		}
	}

	ImGui::NewFrame();
	auto const imguiIO = ImGui::GetIO();
	if(!imguiIO.WantCaptureKeyboard) {
		if (InputBasic_GetAsBool(input, AppKey_Forward)) {

			viewPosition = viewPosition + forwardVel;
			viewLookAt = viewLookAt + forwardVel;
		}
		if (InputBasic_GetAsBool(input, AppKey_Back)) {
			viewPosition = viewPosition + backVel;
			viewLookAt = viewLookAt + backVel;
		}
		if (InputBasic_GetAsBool(input, AppKey_SlideRight)) {
			viewPosition = viewPosition + slideVel;
			viewLookAt = viewLookAt + slideVel;
		}
		if (InputBasic_GetAsBool(input, AppKey_SlideLeft)) {
			viewPosition = viewPosition - slideVel;
			viewLookAt = viewLookAt - slideVel;
		}
	}
	if(!imguiIO.WantCaptureMouse && InputBasic_GetAsBool(input, AppKey_PrimaryButton)) {
		float const rawx = InputBasic_GetAsFloat(input, AppKey_XAxis);
		float const rawy = InputBasic_GetAsFloat(input, AppKey_YAxis);
		float const curx = (rawx - 0.5f) * 2.0f;
		float const cury = (rawy - 0.5f) * 2.0f;

		Math_Mat3F rot;
		rot.col[0] = slideVec;
		rot.col[1] = viewWorldUp;
		rot.col[2] = forwardVec;

		if(Abs(curx) > 1e-3f) {
			Math_Mat3F roty = Math_RotateYAxisMat3F(curx * turnRate);
			rot = Math_MultiplyMat3F(roty, rot);
		}
		if(Abs(cury) > 1e-3f) {
			Math_Mat3F rotx = Math_RotateXAxisMat3F(cury * turnRate);
			rot = Math_MultiplyMat3F(rot, rotx);
		}
		float const viewLength = Length(viewLookAt - viewPosition);
		Vec3F const uv { Vec3F::From(rot.col[1]) };
		Vec3F const fv { Vec3F::From(rot.col[2]) };
		viewLookAt = fv * viewLength + viewPosition;
		viewWorldUp = uv;
	}

	ShowAppMainMenuBar();
	CameraInfoWindow();

	ImGui::Render();
}

static void Draw(double deltaMS) {
	if(gpuCaptureState == GpuCaptureState::StartCapturing) {
		char curpath[2048];
		Os_GetCurrentDir(curpath, 2048);
		char path[2048];
		sprintf(path,"%s/%s", curpath, "capture.gputrace");
		Render_RendererStartGpuCapture(renderer, path);
		gpuCaptureState = GpuCaptureState::Capturing;
	}

	Render_FrameBufferNewFrame(frameBuffer);

	auto graphicsEncoder = Render_FrameBufferGraphicsEncoder(frameBuffer);

	if(bDoSynthWaveVizTests && synthWaveVizTests) {
		SynthWaveVizTests_Render(synthWaveVizTests, graphicsEncoder);
		SynthWaveVizTests_Composite(synthWaveVizTests, graphicsEncoder, Render_FrameBufferColourTarget(frameBuffer));
	}

	if(bDoMeshModRenderTests && meshModRenderTests) {
		meshModRenderTests->render(graphicsEncoder);
	}

	if(bDoALifeTests && alifeTests) {
		alifeTests->render(graphicsEncoder);
	}

	Render_FrameBufferPresent(frameBuffer);

	if(gpuCaptureState == GpuCaptureState::Capturing) {
		Render_RendererEndGpuCapture(renderer);
		gpuCaptureState = GpuCaptureState::NotCapturing;
	}

}

static void Exit() {
	LOGINFO("Exiting");

	// framebuffer destroy will stall to all pipes are empty
	Render_FrameBufferDestroy(renderer, frameBuffer);

	if(meshModRenderTests) {
		MeshModRenderTests::Destroy(meshModRenderTests);
		meshModRenderTests = nullptr;
	}

	if(synthWaveVizTests) {
		SynthWaveVizTests_Destroy(synthWaveVizTests);
		synthWaveVizTests = nullptr;
	}

	if(alifeTests) {
		ALifeTests::Destroy(alifeTests);
		alifeTests = nullptr;
	}

	InputBasic_MouseDestroy(mouse);
	InputBasic_KeyboardDestroy(keyboard);
	InputBasic_Destroy(input);

	enkiDeleteTaskScheduler(taskScheduler);
	Render_RendererDestroy(renderer);

	MeshMod_Shutdown();

	SimpleLogManager_Free(g_logger);

	Memory_TrackerDestroyAndLogLeaks();

}

static void Abort() {
	LOGINFO("ABORT ABORT ABORT");
	abort();
}

static void ProcessMsg(void *msg) {
	if (input) {
		InputBasic_PlatformProcessMsg(input, msg);
	}
}

int main(int argc, char const *argv[]) {
	g_logger = SimpleLogManager_Alloc();

//	Memory_TrackerBreakOnAllocNumber = 2141;

	GameAppShell_Shell *shell = GameAppShell_Init();
	shell->onInitCallback = &Init;
	shell->onDisplayResizeCallback = &Resize;
	shell->onQuitCallback = &Exit;
	shell->onAbortCallback = &Abort;
	shell->perFrameUpdateCallback = &Update;
	shell->perFrameDrawCallback = &Draw;
	shell->onMsgCallback = &ProcessMsg;

	shell->initialWindowDesc.name = "Hermit";
	shell->initialWindowDesc.width = -1;
	shell->initialWindowDesc.height = -1;
	shell->initialWindowDesc.windowsFlags = 0;
	shell->initialWindowDesc.visible = true;

	GameAppShell_MainLoop(argc, argv);

	return g_returnCode;

}
