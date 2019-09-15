#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"
#include "utils_gameappshell/gameappshell.h"
#include "utils_simple_logmanager/logmanager.h"

#include "render_basics/theforge/api.h"
#include "render_basics/api.h"
#include "render_basics/cmd.h"
#include "render_basics/view.h"
#include "render_basics/framebuffer.h"

#include "al2o3_enki/TaskScheduler_c.h"
#include "gfx_theforge/theforge.h"
#include "gfx_imageio/io.h"
#include "gfx_image/utils.h"
#include "gfx_shadercompiler/compiler.h"
#include "al2o3_vfile/vfile.h"
#include "al2o3_os/filesystem.h"
#include "input_basic/input.h"
#include "gfx_imgui_al2o3_theforge_bindings/bindings.h"
#include "gfx_imgui/imgui.h"
#include "utils_nativefiledialogs/dialogs.h"

const uint32_t FRAMES_AHEAD = 3;

Render_RendererHandle renderer;
Render_QueueHandle graphicsQueue;
Render_CmdPoolHandle cmdPool;
Render_FrameBufferHandle frameBuffer;

InputBasic_ContextHandle input;
InputBasic_KeyboardHandle keyboard;
InputBasic_MouseHandle mouse;

enkiTaskSchedulerHandle taskScheduler;

enum AppKey {
	AppKey_Quit
};

ImguiBindings_ContextHandle imguiBindings;

ImguiBindings_Texture textureToView;

static void* EnkiAlloc(void* userData, size_t size) {
	return MEMORY_ALLOCATOR_MALLOC( (Memory_Allocator*)userData, size );
}
static void EnkiFree(void* userData, void* ptr) {
	MEMORY_ALLOCATOR_FREE( (Memory_Allocator*)userData, ptr );
}

// Note that shortcuts are currently provided for display only (future version will add flags to BeginMenu to process shortcuts)
static void ShowMenuFile()
{
	if (ImGui::MenuItem("Open", "Ctrl+O")) {
	}
	ImGui::Separator();
	if (ImGui::MenuItem("Quit", "Alt+F4")) {
		GameAppShell_Quit();
	}
}

static void ShowAppMainMenuBar()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
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

	renderer = Render_RendererCreate();
	if (!renderer) {
		LOGERROR("Render_RendererCreate failed");
		return false;
	}

	taskScheduler = enkiNewTaskScheduler(&EnkiAlloc, &EnkiFree, &Memory_GlobalAllocator);

	GameAppShell_WindowDesc windowDesc;
	GameAppShell_WindowGetCurrentDesc(&windowDesc);

	Render_FrameBufferDesc fbDesc {};
	fbDesc.platformHandle = GameAppShell_GetPlatformWindowPtr();
	fbDesc.queue = Render_RendererGetPrimaryQueue(renderer, Render_GQT_GRAPHICS);
	fbDesc.commandPool = Render_RendererGetPrimaryCommandPool(renderer, Render_GQT_GRAPHICS);
	fbDesc.frameBufferWidth = windowDesc.width;
	fbDesc.frameBufferHeight = windowDesc.height;
	fbDesc.frameBufferCount = FRAMES_AHEAD;
	fbDesc.colourFormat = TinyImageFormat_UNDEFINED;
	fbDesc.depthFormat = TinyImageFormat_UNDEFINED;
	frameBuffer = Render_FrameBufferCreate(renderer, &fbDesc);

	// setup basic input and map quit key
	input = InputBasic_Create();
	uint32_t userIdBlk = InputBasic_AllocateUserIdBlock(input); // 1st 1000 id are the apps
	ASSERT(userIdBlk == 0);

	if (InputBasic_GetKeyboardCount(input) > 0) {
		keyboard = InputBasic_KeyboardCreate(input, 0);
	}
	if (InputBasic_GetMouseCount(input) > 0) {
		mouse = InputBasic_MouseCreate(input, 0);
	}
	if (keyboard)
		InputBasic_MapToKey(input, AppKey_Quit, keyboard, InputBasic_Key_Escape);

	imguiBindings = ImguiBindings_Create(renderer->renderer, renderer->shaderCompiler, input,
																			 20,
																			 FRAMES_AHEAD,
																			 Render_FrameBufferColourFormat(frameBuffer),
																			 Render_FrameBufferDepthFormat(frameBuffer),
																			 TheForge_SC_1,
																			 0);
	if (!imguiBindings) {
		LOGERROR("ImguiBindings_Create failed");
		return false;
	}

	return true;
}

static bool Load() {

	return true;
}

static void Update(double deltaMS) {
	GameAppShell_WindowDesc windowDesc;
	GameAppShell_WindowGetCurrentDesc(&windowDesc);


	InputBasic_SetWindowSize(input, windowDesc.width, windowDesc.height);
	ImguiBindings_SetWindowSize(imguiBindings,
															windowDesc.width,
															windowDesc.height,
															windowDesc.dpiBackingScale[0],
															windowDesc.dpiBackingScale[1]);

	InputBasic_Update(input, deltaMS);
	if (InputBasic_GetAsBool(input, AppKey_Quit)) {
		GameAppShell_Quit();
	}

	// Imgui start
	ImguiBindings_UpdateInput(imguiBindings, deltaMS);
	ImGui::NewFrame();

	ShowAppMainMenuBar();

	ImGui::EndFrame();
	ImGui::Render();

}

static void Draw(double deltaMS) {

	Render_RenderTargetHandle renderTargets[2] = { nullptr, nullptr };

	Render_CmdHandle cmd = Render_FrameBufferNewFrame(frameBuffer, renderTargets + 0, renderTargets + 1);

	Render_CmdBindRenderTargets(cmd, renderTargets[1] ? 2 : 1, renderTargets, true, true, true);

	ImguiBindings_Render(imguiBindings, (TheForge_CmdHandle) cmd);

	Render_FrameBufferPresent(frameBuffer);
}

static void Unload() {
	LOGINFO("Unloading");

// TODO	TheForge_WaitQueueIdle(graphicsQueue);
}

static void Exit() {
	LOGINFO("Exiting");

	ImguiBindings_Destroy(imguiBindings);

	InputBasic_MouseDestroy(mouse);
	InputBasic_KeyboardDestroy(keyboard);
	InputBasic_Destroy(input);

	Render_FrameBufferDestroy(renderer, frameBuffer);

	enkiDeleteTaskScheduler(taskScheduler);
	Render_RendererDestroy(renderer);
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
	auto logger = SimpleLogManager_Alloc();

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

	auto ret = GameAppShell_MainLoop(argc, argv);

	SimpleLogManager_Free(logger);
	return ret;

}
