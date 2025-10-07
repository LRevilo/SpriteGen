#include <InputManager.h>
#include "AppLayer.h"
#include <thread>
#include "imgui/imgui_internal.h"  // for ImGuiDockNodeFlags_NoTabBar

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool AppLayer::HandleInput(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
}
void AppLayer::OnAttach() {
    DXE_INFO("Attached AppLayer Layer: ", name);

	IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplWin32_Init(DXE::Renderer::Hwnd());
    ImGui_ImplDX11_Init(DXE::Renderer::Device(), DXE::Renderer::Context());


	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	io.ConfigWindowsMoveFromTitleBarOnly = true;


	RegisterGenerators();



}
void AppLayer::OnDetach() { DXE_INFO("Dettached AppLayer Layer: ", name); }

void AppLayer::Update(float dt) {


}
void AppLayer::Render(float dt) {

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();


	ImGuiViewport* main_viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(main_viewport->WorkPos);
	ImGui::SetNextWindowSize(main_viewport->WorkSize);
	ImGui::SetNextWindowViewport(main_viewport->ID);

	ImGuiIO& io = ImGui::GetIO();

	// Create a dockspace for the main window
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus;

	ImGui::Begin("MainWindow", nullptr, window_flags);
	ImGuiID dockspace_id = ImGui::GetID("DockSpace");
	ImGuiWindowFlags dock_flags = ImGuiDockNodeFlags_PassthruCentralNode;
	if (!io.KeyAlt) { dock_flags |= ImGuiDockNodeFlags_NoTabBar; }
	ImGui::DockSpace(dockspace_id, ImVec2(0, 0), dock_flags);



	ImGui::Begin("TestWindow");
	ImGui::Text("Editor");
	if (ImGui::InputInt("Texture Size", &Size)) {
		ResizeFrames(Size);
	}

	if (ImGui::InputInt("Frame Count", &FrameCount)) {}


	auto play_button_text = Playing ? "Pause" : "Play";
	if (ImGui::Button(play_button_text)) { Playing = !Playing; }

	ImGui::Text("Frames: %d", TextureFrames.size());

	DrawGeneratorUI();

	DrawFrameTimeline(TextureFrames, SelectedFrame);


	// inside your ImGui frame loop:
	static float elapsedTime = 0.f;
	float frameDuration = 1.f / FrameCount;
	if (Playing && TextureFrames.size()) {
		elapsedTime += dt;
		if (elapsedTime >= frameDuration) {
			elapsedTime -= frameDuration;
			SelectedFrame = (SelectedFrame + 1) % TextureFrames.size();
		}
		if (elapsedTime > 1.f) { elapsedTime = 0.f; }
	}
	else { elapsedTime = 0.f; }

	DrawMainFramePreview(TextureFrames, SelectedFrame);










	ImGui::End();





	ImGui::End();
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

}



void AppLayer::GenerateFramesMultiThreaded() {
	if (IsGenerating.load()) {
		DXE_LOG("Already generating, skipping...");
		return;
	}
	
	if (!ActiveGenerator) {
		DXE_LOG("No active generator selected!");
		return;
	}

	IsGenerating = true;
	bool was_playing = Playing;
	Playing = false;

	for (auto* tex : TextureFrames) { delete tex; }
	TextureFrames.clear();

	for (int i = 0; i < FrameCount; i++) {
		TextureFrames.push_back(new DXE::Texture(Size, Size, 4));
	}

	int total = FrameCount;
	int num_threads = std::thread::hardware_concurrency();
	if (num_threads == 0) num_threads = 4;
	DXE_LOG("Async threads: ", num_threads);

	std::thread mainThread([this, total, num_threads]() {
		std::vector<std::thread> workers;

		for (int t = 0; t < num_threads; ++t) {
			workers.emplace_back([this, t, num_threads, total]() {
				for (int i = t; i < total; i += num_threads) {

					int loop = (int)!ActiveGenerator->IsLooping();
					double time = (double)i / (FrameCount - loop);
					ActiveGenerator->Generate(TextureFrames[i], time);
				}
				});
		}
		for (auto& t : workers) t.join();
		});
	mainThread.join();
	for (auto* tex : TextureFrames) { tex->UpdateTexture(); }
	Playing = was_playing;
	IsGenerating = false;

}


