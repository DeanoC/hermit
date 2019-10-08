#include "al2o3_platform/platform.h"
#include "al2o3_platform/visualdebug.h"
#include "al2o3_memory/memory.h"
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

#if AL2O3_PLATFORM == AL2O3_PLATFORM_APPLE_MAC
#include "al2o3_os/filesystem.h"
#endif

static SimpleLogManager_Handle g_logger;
static int g_returnCode;

Render_RendererHandle renderer;
Render_FrameBufferHandle frameBuffer;

InputBasic_ContextHandle input;
InputBasic_KeyboardHandle keyboard;
InputBasic_MouseHandle mouse;

enkiTaskSchedulerHandle taskScheduler;

enum AppKey {
	AppKey_Quit
};

static void *EnkiAlloc(void *userData, size_t size) {
	return MEMORY_ALLOCATOR_MALLOC((Memory_Allocator *) userData, size);
}
static void EnkiFree(void *userData, void *ptr) {
	MEMORY_ALLOCATOR_FREE((Memory_Allocator *) userData, ptr);
}

// Note that shortcuts are currently provided for display only (future version will add flags to BeginMenu to process shortcuts)
static void ShowMenuFile() {
	ImGui::Separator();
	if (ImGui::MenuItem("Quit", "Alt+F4")) {
		GameAppShell_Quit();
	}
}

static void ShowAppMainMenuBar() {
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			ShowMenuFile();
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}

static bool Init() {

#if AL2O3_PLATFORM == AL2O3_PLATFORM_APPLE_MAC
	//	Os_SetCurrentDir("../.."); // for xcode, no idea...
	Os_SetCurrentDir("..");
	char currentDir[2048];
	Os_GetCurrentDir(currentDir, 2048);
	LOGINFO(currentDir);
#endif

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
	}

	Render_FrameBufferDesc fbDesc{};
	fbDesc.platformHandle = GameAppShell_GetPlatformWindowPtr();
	fbDesc.queue = Render_RendererGetPrimaryQueue(renderer, Render_QT_GRAPHICS);
	fbDesc.commandPool = Render_RendererGetPrimaryCommandPool(renderer, Render_QT_GRAPHICS);
	fbDesc.frameBufferWidth = windowDesc.width;
	fbDesc.frameBufferHeight = windowDesc.height;
	fbDesc.colourFormat = TinyImageFormat_UNDEFINED;
	fbDesc.depthFormat = TinyImageFormat_UNDEFINED;
	fbDesc.embeddedImgui = true;
	fbDesc.visualDebugTarget = true;
	frameBuffer = Render_FrameBufferCreate(renderer, &fbDesc);

	return true;
}

static bool Load() {

	return true;
}

static void Update(double deltaMS) {
	GameAppShell_WindowDesc windowDesc;
	GameAppShell_WindowGetCurrentDesc(&windowDesc);

	InputBasic_SetWindowSize(input, windowDesc.width, windowDesc.height);
	InputBasic_Update(input, deltaMS);
	if (InputBasic_GetAsBool(input, AppKey_Quit)) {
		GameAppShell_Quit();
	}
	/*
		VISDEBUG_LINE(0, 0, 1.0, -1, -1, 1.0, VISDEBUG_PACKCOLOUR(255, 0, 0, 255));
		VISDEBUG_SOLID_TRI(-1, -1, 1.01,
											 -1, 1, 1.01,
											 1, 1, 1.01,
											 VISDEBUG_PACKCOLOUR(128, 128, 0, 128));
		VISDEBUG_SOLID_TRI(1, 1, 1.01,
											 1, -1, 1.01,
											 -1, -1, 1.01,
											 VISDEBUG_PACKCOLOUR(255, 0, 255, 255));

		float lineStripVerts[] = {
				-0.5, -0.5, 1.0,
				 0.5, -0.5, 1.0,
				 0.5,  0.5, 1.0,
				-0.5, -0.5, 1.0,
		};
		VISDEBUG_LINESTRIP(3, lineStripVerts, VISDEBUG_PACKCOLOUR(0, 255, 0, 255));
		float lineVerts[] = {
				-0.6, -0.55, 1.0,
				0.6, -0.55, 1.0,
				0.6, -0.55, 1.0,
				0.6,  0.5, 1.0,
				0.6,  0.5, 1.0,
				-0.6, -0.55, 1.0,
		};
		VISDEBUG_LINES(3, lineVerts, VISDEBUG_PACKCOLOUR(200, 0, 255, 255));
	*/

	/*
		float triVerts[] = {
				-0.0, -0.5, 1.0,
				-0.5, -0.5, 1.0,
				-0.0,  0.0, 1.0,

				 0.0,  0.5, 1.0,
				 0.5,  0.5, 1.0,
				 0.0,  0.0, 1.0,
		};
		VISDEBUG_SOLID_TRIS(2, triVerts, VISDEBUG_PACKCOLOUR(128, 128, 128, 128)); */
/*	float quadVerts[] = {
			-0.0, -0.5, 3.0,
			-0.5, -0.5, 3.0,
			-0.5, 0.0, 3.0,
			-0.0, -0.0, 3.0,

			0.0, 0.5, 10.0,
			0.5, 0.5, 10.0,
			0.5, 0.0, 10.0,
			0.0, 0.0, 10.0,
	};
	VISDEBUG_SOLID_QUADS(2, quadVerts, 0);*/
	Math_Vec3F pos = { 0,0, 0};
	Math_Vec3F rot = {0,0,0};
	Math_Vec3F scale = {1,1,1};
	VISDEBUG_TETRAHEDRON(pos.v, rot.v, scale.v, 0);

	Render_FrameBufferUpdate(frameBuffer,
													 windowDesc.width, windowDesc.height,
													 windowDesc.dpiBackingScale[0],
													 windowDesc.dpiBackingScale[1],
													 deltaMS);
	Render_View view{
			{0, 0, -9},
			{0, 0, 0},
			{0, 1, 0},

			Math_DegreesToRadiansF(70.0f),
			(float) windowDesc.width / (float) windowDesc.height,
			1, 10000
	};
	Render_SetFrameBufferDebugView(frameBuffer, &view);

	ImGui::NewFrame();

	ShowAppMainMenuBar();

	ImGui::EndFrame();
	ImGui::Render();
}

static void Draw(double deltaMS) {
	Render_FrameBufferNewFrame(frameBuffer);

	Render_FrameBufferPresent(frameBuffer);
}

static void Unload() {
	LOGINFO("Unloading");

	// TODO	TheForge_WaitQueueIdle(graphicsQueue);
}

static void Exit() {
	LOGINFO("Exiting");

	Render_FrameBufferDestroy(renderer, frameBuffer);

	InputBasic_MouseDestroy(mouse);
	InputBasic_KeyboardDestroy(keyboard);
	InputBasic_Destroy(input);

	enkiDeleteTaskScheduler(taskScheduler);
	Render_RendererDestroy(renderer);

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

	Memory_TrackerBreakOnAllocNumber = 0;

	GameAppShell_Shell *shell = GameAppShell_Init();
	shell->onInitCallback = &Init;
	shell->onDisplayLoadCallback = &Load;
	shell->onDisplayUnloadCallback = &Unload;
	shell->onQuitCallback = &Exit;
	shell->onAbortCallback = &Abort;
	shell->perFrameUpdateCallback = &Update;
	shell->perFrameDrawCallback = &Draw;
	shell->onMsgCallback = &ProcessMsg;

	shell->initialWindowDesc.name = "Devon";
	shell->initialWindowDesc.width = -1;
	shell->initialWindowDesc.height = -1;
	shell->initialWindowDesc.windowsFlags = 0;
	shell->initialWindowDesc.visible = true;

	GameAppShell_MainLoop(argc, argv);

	return g_returnCode;

}
