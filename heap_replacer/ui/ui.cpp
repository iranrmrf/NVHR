#include "ui.h"

#ifdef HR_USE_GUI

decltype(ui::direct_input_8_create) ui::direct_input_8_create = nullptr;
decltype(ui::create_device) ui::create_device = nullptr;
decltype(ui::get_device_state) ui::get_device_state = nullptr;
decltype(ui::get_device_data) ui::get_device_data = nullptr;
decltype(ui::set_cooperative_level) ui::set_cooperative_level = nullptr;

decltype(ui::window_proc_main) ui::window_proc_main = nullptr;
decltype(ui::window_proc_sub) ui::window_proc_sub = nullptr;

decltype(ui::create_window) ui::create_window = nullptr;
decltype(ui::dispatch_message) ui::dispatch_message = nullptr;
decltype(ui::display_scene) ui::display_scene = nullptr;

uint color_value_getter(void* data, int index)
{
	static color_array ca(hr::get_dhm()->get_cell_count());
	static int running_size = 0;
	static col4 running_color = { };
	if (--running_size > 0) { return running_color.hex; }
	if (!(running_size = hr::get_dhm()->get_addr_size_by_index(*(size_t*)data, index) / (4u * KB))) { return col4().hex; }
	return (running_color = ca[index]).hex;
}

void plot_lines(const char* label, const char* overlay, float width, float height, graph_data* data)
{
	ImGui::PlotLines(label, data->values, data->count, data->offset, overlay, 0.0f, data->window_max, ImVec2(width, height));
}

ui::ui() : ui_settings("hr_settings.ini")
{
	ImGui::SetAllocatorFunctions([](size_t size, void* _) { return hr::hr_malloc(size); }, [](void* address, void* _) { hr::hr_free(address); });

	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();

	io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
	io.MouseDrawCursor = false;

	io.Fonts->AddFontDefault();
	io.Fonts->Build();

	ImGui::StyleColorsDark();

	this->enable_input = true;

	this->plot_height = 0.0f;

	this->refresh_time = 0.0f;

	this->ops[0] = [](char* buff) { sprintf(buff, "%.1f FPS", ImGui::GetIO().Framerate); };
	this->ops[1] = [](char* buff) { sprintf(buff, "%.2f ms", 1000.0f / ImGui::GetIO().Framerate); };
	this->ops[2] = [](char* buff) { POINT p; GetCursorPos(&p); sprintf(buff, "(%04d, %04d)", p.x, p.y); };

	// UI

	this->enable_ui = false;
	this->enable_overlay = false;
	this->enable_background_logging = false;
	this->enable_info_window = false;
	this->enable_mpm_window = false;
	this->enable_dhm_window = false;
	this->enable_shm_window = false;

	this->enable_demo_window = false;
	this->enable_style_editor_window = false;
	this->enable_user_guide_window = false;
	this->enable_metrics_window = false;
	this->enable_about_window = false;

	// OVERLAY

	this->menu_height = 0.0f;

	this->enable_overlay_fps = false;
	this->enable_overlay_frametime = false;
	this->enable_overlay_mouse_pos = false;

	this->overlay_color[R] = 0.0f;
	this->overlay_color[G] = 0.0f;
	this->overlay_color[B] = 0.0f;
	this->overlay_color[A] = 1.0f;

	// INFO

	this->enable_info_computer_stats = false;
	this->enable_info_fps_graph = false;
	this->enable_info_frametime_graph = false;

	// MEMORY POOL MANAGER

	this->enable_mpm_allocs_graph = false;
	this->enable_mpm_frees_graph = false;
	this->enable_mpm_pool_stats = false;

	// DEFAULT HEAP MANAGER

	this->enable_dhm_allocs_graph = false;
	this->enable_dhm_frees_graph = false;
	this->enable_dhm_free_blocks_graph = false;

	//this->enable_dhm_used = false;
	//this->enable_dhm_free = false;

	this->dhm_block_count = 0u;

	// GRAPH DATA

	this->gd_info_fps = new graph_data(COUNTER_INIT_VALUE);
	this->gd_info_frametime = new graph_data(COUNTER_INIT_VALUE);

	this->gd_mpm_allocs = new graph_data(COUNTER_INIT_VALUE);
	this->gd_mpm_frees = new graph_data(COUNTER_INIT_VALUE);

	this->gd_dhm_allocs = new graph_data(COUNTER_INIT_VALUE);
	this->gd_dhm_frees = new graph_data(COUNTER_INIT_VALUE);
	this->gd_dhm_free_blocks = new graph_data(COUNTER_INIT_VALUE);

	this->load_settings();
}

ui::~ui()
{
	ImGui::DestroyContext();
}

void ui::init(HWND wnd, LPDIRECT3DDEVICE9 device)
{
	ImGui_ImplWin32_Init(wnd);
	ImGui_ImplDX9_Init(device);
	this->refresh_time = (float)ImGui::GetTime();
}

void ui::handle_input(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	ImGui_ImplWin32_WndProcHandler(wnd, msg, wParam, lParam);
}

void ui::set_imgui_mouse_state(bool enabled)
{
	ImGuiIO& io = ImGui::GetIO();
	io.MouseDrawCursor = enabled;
	BIT_SET(io.ConfigFlags, ImGuiConfigFlags_NoMouse, !enabled);
}

void ui::load_settings()
{
	// UI

	this->enable_overlay = this->ui_settings.load_bool("UI", "enable_overlay");
	this->enable_background_logging = this->ui_settings.load_bool("UI", "enable_background_logging");
	this->enable_info_window = this->ui_settings.load_bool("UI", "enable_info_window");
	this->enable_mpm_window = this->ui_settings.load_bool("UI", "enable_mpm_window");
	this->enable_dhm_window = this->ui_settings.load_bool("UI", "enable_dhm_window");
	this->enable_shm_window = this->ui_settings.load_bool("UI", "enable_shm_window");

	this->enable_demo_window = this->ui_settings.load_bool("UI", "enable_demo_window");
	this->enable_style_editor_window = this->ui_settings.load_bool("UI", "enable_style_editor_window");
	this->enable_user_guide_window = this->ui_settings.load_bool("UI", "enable_user_guide_window");
	this->enable_metrics_window = this->ui_settings.load_bool("UI", "enable_metrics_window");
	this->enable_about_window = this->ui_settings.load_bool("UI", "enable_about_window");

	// OVERLAY

	this->menu_height = 0.0f;

	this->enable_overlay_fps = this->ui_settings.load_bool("OVERLAY", "enable_overlay_fps");
	this->enable_overlay_frametime = this->ui_settings.load_bool("OVERLAY", "enable_overlay_frametime");
	this->enable_overlay_mouse_pos = this->ui_settings.load_bool("OVERLAY", "enable_overlay_mouse_pos");

	this->overlay_color[R] = (float)this->ui_settings.load_double("OVERLAY", "overlay_r");
	this->overlay_color[G] = (float)this->ui_settings.load_double("OVERLAY", "overlay_g");
	this->overlay_color[B] = (float)this->ui_settings.load_double("OVERLAY", "overlay_b");
	this->overlay_color[A] = (float)this->ui_settings.load_double("OVERLAY", "overlay_a", 1.0f);

	// INFO

	this->enable_info_computer_stats = this->ui_settings.load_bool("INFO", "enable_info_computer_stats");
	this->enable_info_fps_graph = this->ui_settings.load_bool("INFO", "enable_info_fps_graph");
	this->enable_info_frametime_graph = this->ui_settings.load_bool("INFO", "enable_info_frametime_graph");

	// MEMORY POOL MANAGER

	this->enable_mpm_allocs_graph = this->ui_settings.load_bool("MEMORY POOL MANAGER", "enable_mpm_allocs_graph");
	this->enable_mpm_frees_graph = this->ui_settings.load_bool("MEMORY POOL MANAGER", "enable_mpm_frees_graph");

	this->enable_mpm_pool_stats = this->ui_settings.load_bool("MEMORY POOL MANAGER", "enable_mpm_pool_stats");

	// DEFAULT HEAP MANAGER

	this->enable_dhm_allocs_graph = this->ui_settings.load_bool("DEFAULT HEAP MANAGER", "enable_dhm_allocs_graph");
	this->enable_dhm_frees_graph = this->ui_settings.load_bool("DEFAULT HEAP MANAGER", "enable_dhm_frees_graph");

	this->enable_dhm_free_blocks_graph = this->ui_settings.load_bool("DEFAULT HEAP MANAGER", "enable_dhm_free_blocks_graph");

	this->dhm_block_count = 0u;

	// GRAPH DATA COUNTERS

	this->gd_info_fps->count = this->ui_settings.load_long("COUNTERS", "gd_info_fps_count", COUNTER_INIT_VALUE);
	this->gd_info_frametime->count = this->ui_settings.load_long("COUNTERS", "gd_info_frametime_count", COUNTER_INIT_VALUE);

	this->gd_mpm_allocs->count = this->ui_settings.load_long("COUNTERS", "gd_mpm_allocs_count", COUNTER_INIT_VALUE);
	this->gd_mpm_frees->count = this->ui_settings.load_long("COUNTERS", "gd_mpm_frees_count", COUNTER_INIT_VALUE);

	this->gd_dhm_allocs->count = this->ui_settings.load_long("COUNTERS", "gd_dhm_allocs_count", COUNTER_INIT_VALUE);
	this->gd_dhm_frees->count = this->ui_settings.load_long("COUNTERS", "gd_dhm_frees_count", COUNTER_INIT_VALUE);
	this->gd_dhm_free_blocks->count = this->ui_settings.load_long("COUNTERS", "gd_dhm_free_blocks_count", COUNTER_INIT_VALUE);
}

void ui::save_settings()
{
	// UI

	this->ui_settings.save_bool("UI", "enable_overlay", this->enable_overlay);
	this->ui_settings.save_bool("UI", "enable_background_logging", this->enable_background_logging);
	this->ui_settings.save_bool("UI", "enable_info_window", this->enable_info_window);
	this->ui_settings.save_bool("UI", "enable_mpm_window", this->enable_mpm_window);
	this->ui_settings.save_bool("UI", "enable_dhm_window", this->enable_dhm_window);
	this->ui_settings.save_bool("UI", "enable_shm_window", this->enable_shm_window);

	this->ui_settings.save_bool("UI", "enable_demo_window", this->enable_demo_window);
	this->ui_settings.save_bool("UI", "enable_style_editor_window", this->enable_style_editor_window);
	this->ui_settings.save_bool("UI", "enable_user_guide_window", this->enable_user_guide_window);
	this->ui_settings.save_bool("UI", "enable_metrics_window", this->enable_metrics_window);
	this->ui_settings.save_bool("UI", "enable_about_window", this->enable_about_window);

	// OVERLAY

	this->ui_settings.save_bool("OVERLAY", "enable_overlay_fps", this->enable_overlay_fps);
	this->ui_settings.save_bool("OVERLAY", "enable_overlay_frametime", this->enable_overlay_frametime);
	this->ui_settings.save_bool("OVERLAY", "enable_overlay_mouse_pos", this->enable_overlay_mouse_pos);

	this->ui_settings.save_double("OVERLAY", "overlay_r", this->overlay_color[R]);
	this->ui_settings.save_double("OVERLAY", "overlay_g", this->overlay_color[G]);
	this->ui_settings.save_double("OVERLAY", "overlay_b", this->overlay_color[B]);
	this->ui_settings.save_double("OVERLAY", "overlay_a", this->overlay_color[A]);

	// INFO

	this->ui_settings.save_bool("INFO", "enable_info_computer_stats", this->enable_info_computer_stats);
	this->ui_settings.save_bool("INFO", "enable_info_fps_graph", this->enable_info_fps_graph);
	this->ui_settings.save_bool("INFO", "enable_info_frametime_graph", this->enable_info_frametime_graph);

	// MEMORY POOL MANAGER

	this->ui_settings.save_bool("MEMORY POOL MANAGER", "enable_mpm_allocs_graph", this->enable_mpm_allocs_graph);
	this->ui_settings.save_bool("MEMORY POOL MANAGER", "enable_mpm_frees_graph", this->enable_mpm_frees_graph);
	this->ui_settings.save_bool("MEMORY POOL MANAGER", "enable_mpm_pool_stats", this->enable_mpm_pool_stats);

	// DEFAULT HEAP MANAGER

	this->ui_settings.save_bool("DEFAULT HEAP MANAGER", "enable_dhm_allocs_graph", this->enable_dhm_allocs_graph);
	this->ui_settings.save_bool("DEFAULT HEAP MANAGER", "enable_dhm_frees_graph", this->enable_dhm_frees_graph);
	this->ui_settings.save_bool("DEFAULT HEAP MANAGER", "enable_dhm_free_blocks_graph", this->enable_dhm_free_blocks_graph);

	// GRAPH DATA COUNTERS

	this->ui_settings.save_long("COUNTERS", "gd_info_fps_count", this->gd_info_fps->count);
	this->ui_settings.save_long("COUNTERS", "gd_info_frametime_count", this->gd_info_frametime->count);

	this->ui_settings.save_long("COUNTERS", "gd_mpm_allocs_count", this->gd_mpm_allocs->count);
	this->ui_settings.save_long("COUNTERS", "gd_mpm_frees_count", this->gd_mpm_frees->count);

	this->ui_settings.save_long("COUNTERS", "gd_dhm_allocs_count", this->gd_dhm_allocs->count);
	this->ui_settings.save_long("COUNTERS", "gd_dhm_frees_count", this->gd_dhm_frees->count);
	this->ui_settings.save_long("COUNTERS", "gd_dhm_free_blocks_count", this->gd_dhm_free_blocks->count);

	this->ui_settings.save();
}

void ui::render()
{
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	this->update_graphs();
	this->render_ui();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}

void ui::update_graphs()
{
	ImGuiIO& io = ImGui::GetIO();
	while (this->refresh_time < ImGui::GetTime())
	{
		if (this->enable_ui || this->enable_background_logging)
		{
			this->gd_info_fps->add_data(io.Framerate);
			this->gd_info_frametime->add_data(1000.0f / io.Framerate);

			this->gd_mpm_allocs->add_data((float)hr::get_mpm()->get_allocs());
			this->gd_mpm_frees->add_data((float)hr::get_mpm()->get_frees());

			this->gd_dhm_allocs->add_data((float)hr::get_dhm()->get_allocs());
			this->gd_dhm_frees->add_data((float)hr::get_dhm()->get_frees());
			this->gd_dhm_free_blocks->add_data((float)hr::get_dhm()->get_free_blocks());
		}

		this->refresh_time += 1.0f / 60.0f;
	}
}

void ui::clear_graphs()
{
	this->save_settings();

	if (this->enable_background_logging)
	{
		if (!this->enable_info_fps_graph) { this->gd_info_fps->clear(); }
		if (!this->enable_info_frametime_graph) { this->gd_info_frametime->clear(); }

		if (!this->enable_mpm_allocs_graph) { this->gd_mpm_allocs->clear(); }
		if (!this->enable_mpm_frees_graph) { this->gd_mpm_frees->clear(); }

		if (!this->enable_dhm_allocs_graph) { this->gd_dhm_allocs->clear(); }
		if (!this->enable_dhm_frees_graph) { this->gd_dhm_frees->clear(); }
		if (!this->enable_dhm_free_blocks_graph) { this->gd_dhm_free_blocks->clear(); }
	}
	else
	{
		this->gd_info_fps->clear();
		this->gd_info_frametime->clear();

		this->gd_mpm_allocs->clear();
		this->gd_mpm_frees->clear();

		this->gd_dhm_allocs->clear();
		this->gd_dhm_frees->clear();
		this->gd_dhm_free_blocks->clear();
	}
}

void ui::render_ui()
{
	if (this->enable_overlay) { this->render_overlay(); }
	if (this->enable_ui)
	{
		this->render_game_menu();
		if (this->enable_info_window) { this->render_info_window(); }
		if (this->enable_mpm_window) { this->render_memory_pool_menu(); }
		if (this->enable_dhm_window) { this->render_default_heap_menu(); }
		if (this->dhm_block_count) { this->render_default_heap_blocks(); }
	}
}

void ui::render_game_menu()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("General"))
		{
			ImGui::MenuItem("Debug Log", nullptr);
			ImGui::Separator();
			if (ImGui::BeginMenu("Overlay"))
			{
				static bool moved = false;
				static int item_index = 0;
				for (size_t i = 0; i < countof(this->names); i++)
				{
					ImGui::Checkbox(this->names[i], this->bools[i]);
					if (ImGui::IsItemActive() && !ImGui::IsItemHovered())
					{
						if (fabsf(ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).y) >= ImGui::GetItemRectSize().y)
						{
							int j = i + (ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).y < 0.0f ? -1 : 1);
							if (j >= 0 && j < (int)countof(names))
							{
								util::swap(this->names[i], this->names[j]);
								util::swap(this->bools[i], this->bools[j]);
								util::swap(this->templates[i], this->templates[j]);
								util::swap(this->ops[i], this->ops[j]);

								item_index = j;
								moved = true;

								ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
							}
						}
					}
					if (!ImGui::IsItemActive() && moved && item_index == i)
					{
						if (ImGui::IsItemHovered()) { *this->bools[i] = !*this->bools[i]; }
						moved = false;
					}
				}				
				ImGui::Separator();
				ImGui::ColorEdit4("##overlay_text_color", this->overlay_color, ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_NoOptions);
				this->enable_overlay = this->enable_overlay_fps || this->enable_overlay_frametime || this->enable_overlay_mouse_pos;
				ImGui::EndMenu();
			}
			ImGui::Separator();
			if (ImGui::BeginMenu("Info Window"))
			{
				//ImGui::Checkbox("CPU Usage", &this->enable_info_fps_graph);

				if (ImGui::BeginTable("##info", 2))
				{
					ImGui::TableNextColumn(); ImGui::Checkbox("FPS Graph", &this->enable_info_fps_graph);
					ImGui::TableNextColumn();
					ImGui::SetNextItemWidth(150.0f);
					ImGui::InputScalar("##fps_count", ImGuiDataType_U16, &this->gd_info_fps->count, &this->slider_lo, &this->slider_hi, "%u");

					ImGui::TableNextColumn(); ImGui::Checkbox("Frametime Graph", &this->enable_info_frametime_graph);
					ImGui::TableNextColumn();
					ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
					 ImGui::InputScalar("##frametime_count", ImGuiDataType_U16, &this->gd_info_frametime->count, &this->slider_lo, &this->slider_hi, "%u");

					ImGui::EndTable();
				}

				this->enable_info_window = this->enable_info_fps_graph || this->enable_info_frametime_graph;

				ImGui::EndMenu();
			}

			ImGui::Checkbox("Background Logging", &this->enable_background_logging);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Heap Statistics"))
		{
			if (ImGui::BeginMenu("Memory Pool Manager"))
			{
				if (ImGui::BeginTable("##info", 2))
				{
					ImGui::TableNextColumn(); ImGui::Checkbox("Allocations per Frame Graph", &this->enable_mpm_allocs_graph);
					ImGui::TableNextColumn();
					ImGui::SetNextItemWidth(150.0f);
					ImGui::InputScalar("##allocs_count", ImGuiDataType_U16, &this->gd_mpm_allocs->count, &this->slider_lo, &this->slider_hi, "%u");

					ImGui::TableNextColumn(); ImGui::Checkbox("Frees per Frame Graph", &this->enable_mpm_frees_graph);
					ImGui::TableNextColumn();
					ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
					ImGui::InputScalar("##frees_count", ImGuiDataType_U16, &this->gd_mpm_frees->count, &this->slider_lo, &this->slider_hi, "%u");

					ImGui::EndTable();
				}

				ImGui::Checkbox("Pool Statistics", &this->enable_mpm_pool_stats);

				this->enable_mpm_window = this->enable_mpm_allocs_graph || this->enable_mpm_frees_graph || this->enable_mpm_pool_stats;

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Default Heap Manager"))
			{
				if (ImGui::BeginTable("##info", 2))
				{
					ImGui::TableNextColumn(); ImGui::Checkbox("Allocations per Frame Graph", &this->enable_dhm_allocs_graph);
					ImGui::TableNextColumn();
					ImGui::SetNextItemWidth(150.0f);
					ImGui::InputScalar("##allocs_count", ImGuiDataType_U16, &this->gd_dhm_allocs->count, &this->slider_lo, &this->slider_hi, "%u");

					ImGui::TableNextColumn(); ImGui::Checkbox("Frees per Frame Graph", &this->enable_dhm_frees_graph);
					ImGui::TableNextColumn();
					ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
					ImGui::InputScalar("##frees_count", ImGuiDataType_U16, &this->gd_dhm_frees->count, &this->slider_lo, &this->slider_hi, "%u");

					ImGui::EndTable();
				}

				float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
				ImGui::PushButtonRepeat(true);
				if (ImGui::ArrowButton("##left", ImGuiDir_Left)) { if (this->dhm_block_count > 0) { this->dhm_block_count--; } }
				ImGui::SameLine(0.0f, spacing);
				if (ImGui::ArrowButton("##right", ImGuiDir_Right)) { if (this->dhm_block_count < hr::get_dhm()->get_block_count() + 1) { this->dhm_block_count++; } }
				ImGui::PopButtonRepeat();
				ImGui::SameLine();
				ImGui::Text("Block Count : %d", this->dhm_block_count);

				this->enable_dhm_window = this->enable_dhm_allocs_graph || this->enable_dhm_frees_graph;

				ImGui::EndMenu();
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("ImGui"))
		{
			ImGui::Checkbox("Demo Window", &this->enable_demo_window);
			ImGui::Checkbox("Style Editor", &this->enable_style_editor_window);
			ImGui::Checkbox("User Guide", &this->enable_user_guide_window);

			ImGui::Separator();

			ImGui::Checkbox("Metrics Window", &this->enable_metrics_window);

			ImGui::Separator();

			if (ImGui::MenuItem("Use Dark Theme")) { ImGui::StyleColorsDark(); }
			if (ImGui::MenuItem("Use Light Theme")) { ImGui::StyleColorsLight(); }
			if (ImGui::MenuItem("Use Classic Theme")) { ImGui::StyleColorsClassic(); }

			ImGui::Separator();

			//if (ImGui::MenuItem("Use Custom1 Theme")) { ImGui::StyleColorsCustom1(); }
			//if (ImGui::MenuItem("Use Custom2 Theme")) { ImGui::StyleColorsCustom2(); }
			//if (ImGui::MenuItem("Use Custom3 Theme")) { ImGui::StyleColorsCustom3(); }
			//if (ImGui::MenuItem("Use Custom4 Theme")) { ImGui::StyleColorsCustom4(); }
			//if (ImGui::MenuItem("Use Custom5 Theme")) { ImGui::StyleColorsCustom5(); }

			ImGui::Separator();

			ImGui::Checkbox("About", &this->enable_about_window);

			ImGui::Separator();

			if (ImGui::MenuItem("Terminate Process")) { this->save_settings(); TerminateProcess(GetCurrentProcess(), 0u); }

			ImGui::EndMenu();
		}

		this->menu_height = ImGui::GetWindowSize().y;

		ImGui::EndMainMenuBar();
	}

	if (this->enable_demo_window) { ImGui::ShowDemoWindow(&this->enable_demo_window); }
	if (this->enable_style_editor_window) { ImGui::ShowStyleEditor(); }
	if (this->enable_metrics_window) { ImGui::ShowMetricsWindow(&this->enable_metrics_window); }
	if (this->enable_user_guide_window) { ImGui::ShowUserGuide(); }
	if (this->enable_about_window) { ImGui::ShowAboutWindow(&this->enable_about_window); }

	// log
}

void ui::render_overlay()
{
	ImGuiIO& io = ImGui::GetIO();
	char buff[32];

	float height = this->enable_ui ? this->menu_height : 0.0f;
	ImGui::SetNextWindowPos(ImVec2(0.0f, height));
	ImVec2 window_size = ImVec2(io.DisplaySize);
	window_size.y -= height;
	ImGui::SetNextWindowSize(window_size);
	if (ImGui::Begin("##overlay", nullptr,
		ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs | 
		ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoFocusOnAppearing))
	{
		float total_len = 0.0f;
		for (size_t i = 0; i < CHECKBOX_COUNT; i++)
		{
			if (*this->bools[i]) { total_len += ImGui::CalcTextSize(this->templates[i]).x; }
		}
		ImGui::PushStyleColor(ImGuiCol_Text, *(ImVec4*)this->overlay_color);
		for (size_t i = 0; i < CHECKBOX_COUNT; i++)
		{
			if (*this->bools[i])
			{
				ImGui::SameLine();
				ImGui::SetCursorPosX(ImGui::GetWindowContentRegionWidth() - total_len);
				total_len -= ImGui::CalcTextSize(this->templates[i]).x;
				this->ops[i](buff);
				ImGui::Text(buff);
			}
		}
		ImGui::PopStyleColor();
	}
	ImGui::End();
}

void ui::render_info_window()
{
	if (ImGui::Begin("Game Info", nullptr, ImGuiWindowFlags_NoFocusOnAppearing))
	{
		char buff[32];
		// ADD COMPUTER HARDWARE
		if (this->enable_info_fps_graph)
		{
			sprintf(buff, "%.0f\n\n%.0f\n\n0", this->gd_info_fps->window_max, this->gd_info_fps->get_back());
			plot_lines(buff, "fps", 0.0f, this->plot_height, this->gd_info_fps);
		}

		if (this->enable_info_frametime_graph)
		{
			sprintf(buff, "%.0f\n\n%.0f\n\n0", this->gd_info_frametime->window_max, this->gd_info_frametime->get_back());
			plot_lines(buff, "frametime", 0.0f, this->plot_height, this->gd_info_frametime);
		}
	}
	ImGui::End();
}

void ui::render_memory_pool_menu()
{
	if (ImGui::Begin("Memory Pool Manager", nullptr, ImGuiWindowFlags_NoFocusOnAppearing))
	{
		char buff[32];

		if (this->enable_mpm_allocs_graph)
		{
			sprintf(buff, "%.0f\n\n%.0f\n\n0", this->gd_mpm_allocs->window_max, this->gd_mpm_allocs->get_back());
			plot_lines(buff, "mpm allocs", 0.0f, this->plot_height, this->gd_mpm_allocs);
			ImGui::NewLine();
		}

		if (this->enable_mpm_frees_graph)
		{
			sprintf(buff, "%.0f\n\n%.0f\n\n0", this->gd_mpm_frees->window_max, this->gd_mpm_frees->get_back());
			plot_lines(buff, "mpm frees", 0.0f, this->plot_height, this->gd_mpm_frees);
			ImGui::NewLine();
		}

		if (this->enable_mpm_pool_stats)
		{
			ImVec2 s = ImGui::GetItemRectSize();

			for (size_t index = 0u; index < hr::get_mpm()->get_pool_count(); index++)
			{
				memory_pool* pool = hr::get_mpm()->get_pool(index);
				size_t cell_count = pool->get_cell_count();
				size_t max_cell_count = pool->get_max_cell_count();
				float perc = (float)cell_count / max_cell_count;
				float used_size = perc * ((float)pool->get_max_size() / MB);
				ImGui::Text("Pool: %*d\n", 4u, pool->get_item_size());
				sprintf(buff, "%d / %d (%.2f MB)", cell_count, max_cell_count, used_size);
				ImGui::SameLine(); ImGui::ProgressBar(perc, ImVec2(s.x, 0.0f), buff);
			}
		}
	}
	ImGui::End();
}

void ui::render_default_heap_menu()
{
	if (ImGui::Begin("Default Heap Manager", nullptr, ImGuiWindowFlags_NoFocusOnAppearing))
	{
		char buff[32];

		if (this->enable_dhm_allocs_graph)
		{
			sprintf(buff, "%.0f\n\n%.0f\n\n0", this->gd_dhm_allocs->window_max, this->gd_dhm_allocs->get_back());
			plot_lines(buff, "dhm allocs", 0.0f, this->plot_height, this->gd_dhm_allocs);
			ImGui::NewLine();
		}

		if (this->enable_dhm_frees_graph)
		{
			sprintf(buff, "%.0f\n\n%.0f\n\n0", this->gd_dhm_frees->window_max, this->gd_dhm_frees->get_back());
			plot_lines(buff, "dhm frees", 0.0f, this->plot_height, this->gd_dhm_frees);
			ImGui::NewLine();
		}

		if (this->enable_dhm_free_blocks_graph)
		{
			sprintf(buff, "%.0f\n\n%.0f\n\n0", this->gd_dhm_free_blocks->window_max, this->gd_dhm_free_blocks->get_back());
			plot_lines(buff, "dhm free blocks", 0.0f, this->plot_height, this->gd_dhm_free_blocks);
			ImGui::NewLine();
		}

		sprintf(buff, "used: %.2f / %.2f MB", (float)hr::get_dhm()->get_used_size() / MB, (float)hr::get_dhm()->get_curr_size() / MB);
		ImGui::ProgressBar((float)hr::get_dhm()->get_used_size() / hr::get_dhm()->get_curr_size(), ImVec2(-1.0f, 32.0f), buff);
		
		sprintf(buff, "free: %.2f / %.2f MB", (float)hr::get_dhm()->get_free_size() / MB, (float)hr::get_dhm()->get_curr_size() / MB);
		ImGui::ProgressBar((float)hr::get_dhm()->get_free_size() / hr::get_dhm()->get_curr_size(), ImVec2(-1.0f, 32.0f), buff);

	}
	ImGui::End();
}

void ui::render_default_heap_blocks()
{
	char buff[32];
	for (size_t i = 0u; i < this->dhm_block_count; i++)
	{
		sprintf(buff, "Block : %d", i + 1);
		if (ImGui::Begin(buff))
		{
			char buff2[32];
			sprintf(buff2, "##%d", i + 1);
			ImGui::PlotColorGrid(buff2, color_value_getter, &i, hr::get_dhm()->get_cell_count(), 0, ImVec2(0.0f, 0.0f));
		}
		ImGui::End();
	}
}

HRESULT WINAPI ui::direct_input_8_create_hook(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, IUnknown* punkOuter)
{
	HRESULT hr = ui::direct_input_8_create(hinst, dwVersion, riidltf, ppvOut, punkOuter);
	if (SUCCEEDED(hr))
	{
		MH_Initialize();
		hr::get_uim()->create_device_addr = ((IDirectInput8A*)*ppvOut)->lpVtbl->CreateDevice;
		MH_CreateHook(hr::get_uim()->create_device_addr, &ui::create_device_hook, (void**)&ui::create_device);
		MH_EnableHook(hr::get_uim()->create_device_addr);
	}
	return hr;
}

HRESULT WINAPI ui::create_device_hook(IDirectInput8A* self, REFGUID rguid, IDirectInputDevice8A** lplpDirectInputDevice, IUnknown* pUnkOuter)
{
	HRESULT hr = ui::create_device(self, rguid, lplpDirectInputDevice, pUnkOuter);
	if (SUCCEEDED(hr))
	{		
		void* get_device_state_addr = (*lplpDirectInputDevice)->lpVtbl->GetDeviceState;
		MH_CreateHook(get_device_state_addr, &ui::get_device_state_hook, (void**)&ui::get_device_state);
		MH_EnableHook(get_device_state_addr);
		void* get_device_data_addr = (*lplpDirectInputDevice)->lpVtbl->GetDeviceData;
		MH_CreateHook(get_device_data_addr, &ui::get_device_data_hook, (void**)&ui::get_device_data);
		MH_EnableHook(get_device_data_addr);
		hr::get_uim()->set_cooperative_level_addr = (*lplpDirectInputDevice)->lpVtbl->SetCooperativeLevel;
		MH_CreateHook(hr::get_uim()->set_cooperative_level_addr, &ui::set_cooperative_level_hook, (void**)&ui::set_cooperative_level);
		MH_EnableHook(hr::get_uim()->set_cooperative_level_addr);
	}
	return hr;
}

HRESULT APIENTRY ui::get_device_state_hook(IDirectInputDevice8A* self, DWORD cbData, LPVOID lpvData)
{
	HRESULT hr = ui::get_device_state(self, cbData, lpvData);
	if (lpvData && !hr::get_uim()->enable_input) { util::cmemset8(lpvData, 0u, cbData); }
	return hr;
}

HRESULT APIENTRY ui::get_device_data_hook(IDirectInputDevice8A* self, DWORD cbObjectData, LPDIDEVICEOBJECTDATA rgdod, LPDWORD pdwInOut, DWORD dwFlags)
{
	HRESULT hr = ui::get_device_data(self, cbObjectData, rgdod, pdwInOut, dwFlags);
	if (rgdod && !hr::get_uim()->enable_input) { util::cmemset8(rgdod, 0u, cbObjectData * *pdwInOut); }
	return hr;
}

HRESULT APIENTRY ui::set_cooperative_level_hook(IDirectInputDevice8A* self, HWND hwnd, DWORD dwFlags)
{
	return ui::set_cooperative_level(self, hwnd, DISCL_FOREGROUND | DISCL_EXCLUSIVE);
}

LRESULT CALLBACK ui::window_proc_hook(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ui* uim = hr::get_uim();
	uim->handle_input(hwnd, uMsg, wParam, lParam);
	if (uMsg == WM_KEYDOWN)
	{
		if (wParam == VK_INSERT)
		{
			if (lParam & (1 << 30)) // held
			{
				uim->set_imgui_mouse_state(false);
				uim->enable_input = true;
			}
			else // tapped
			{
 				uim->enable_ui = !uim->enable_ui;
				uim->set_imgui_mouse_state(uim->enable_ui);
				uim->enable_input = !uim->enable_ui;
				if (!uim->enable_ui) { uim->clear_graphs(); }
				else { uim->load_settings(); } // FOR TESTING
			}
		}
	}
	else if (uMsg == WM_ACTIVATEAPP || uMsg == WM_ACTIVATE || uMsg == WM_SETFOCUS)
	{
		if ((uMsg == WM_ACTIVATEAPP && wParam == TRUE) || (uMsg == WM_ACTIVATE && wParam != WA_INACTIVE) || (uMsg == WM_SETFOCUS))
		{
			while (ShowCursor(FALSE) >= 0);
			uim->enable_input = true;
		}
		if ((uMsg == WM_ACTIVATEAPP && wParam == FALSE) || (uMsg == WM_ACTIVATE && wParam == WA_INACTIVE))
		{
			while (ShowCursor(TRUE) < 0);
			uim->enable_input = false;
		}
		return 0;
	}
	return CallWindowProc((hwnd == HR_MAIN_WINDOW) ? ui::window_proc_main : ui::window_proc_sub, hwnd, uMsg, wParam, lParam);
}

HWND WINAPI ui::create_window_hook(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
	if (lpWindowName && !_stricmp(lpWindowName, HR_WINDOW_NAME)) { dwStyle = WS_POPUP; dwExStyle = WS_EX_APPWINDOW; }
	HWND hwnd = ui::create_window(dwExStyle, lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
	if (hwnd && !_stricmp(lpClassName, HR_WINDOW_NAME))
	{
		(lpWindowName ? ui::window_proc_main : ui::window_proc_sub) = (WNDPROC)SetWindowLongPtr(hwnd, GWL_WNDPROC, (LONG_PTR)&ui::window_proc_hook);
	}
	return hwnd;
}

LRESULT WINAPI ui::dispatch_message_hook(const MSG* msg)
{
	LRESULT result = ui::dispatch_message(msg);
	HWND foreground = GetForegroundWindow();
	if ((msg->message == WM_MOUSEMOVE) &&
		(msg->hwnd == HR_MAIN_WINDOW || msg->hwnd == HR_SUB_WINDOW) &&
		(foreground == HR_MAIN_WINDOW || foreground == HR_SUB_WINDOW))
	{
		if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && !(ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_NoMouse))
		{
			ClipCursor(nullptr);
		}
		else
		{
			RECT rcClip;
			GetWindowRect(msg->hwnd, &rcClip);
			rcClip.left += 1; rcClip.top += 1; rcClip.right -= 1; rcClip.bottom -= 1;
			ClipCursor(&rcClip);
		}
	}
	return result;
}

HRESULT ui::display_scene_hook(IDirect3DDevice9* self)
{
	static bool init = [self]() { hr::get_uim()->init(HR_MAIN_WINDOW, *((LPDIRECT3DDEVICE9*)self + 0xA2)); return true; }();
	hr::get_uim()->render();
	return ui::display_scene(self);
}

#endif

// TODO
// save theme
// save order
// fix progress bar sizes
// fix crashing on insert press (lock?)
