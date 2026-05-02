#include "global.hpp"
#include "logger/logger.hpp"
#include "fs/fs.hpp"
#include "menus.hpp"
#include "audio/audio.hpp"
#include "hook/hook.hpp"
#include "settings/settings.hpp"
#include "audio/player.hpp"

#include <shellapi.h>

namespace
{
	constexpr auto kRepositoryUrl = "https://github.com/RCompanyX/ecm-R";
	constexpr auto kIssuesUrl = "https://github.com/RCompanyX/ecm-R/issues";

	void open_external_link(const char* url)
	{
		const auto result = reinterpret_cast<INT_PTR>(ShellExecuteA(nullptr, "open", url, nullptr, nullptr, SW_SHOWNORMAL));
		if (result <= 32)
		{
			global::msg_box("ECM-R", std::string("Failed to open link:\n") + url);
		}
	}
}

void menus::init()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui::GetIO().IniFilename = nullptr;

	menus::build_font(ImGui::GetIO());
}

void menus::prepare()
{
	switch (global::renderer)
	{
	case kiero::RenderType::D3D9:
		ImGui_ImplDX9_NewFrame();
		break;

	case kiero::RenderType::D3D10:
		ImGui_ImplDX10_NewFrame();
		break;

	case kiero::RenderType::D3D11:
		ImGui_ImplDX11_NewFrame();
		break;

	case kiero::RenderType::OpenGL:
		ImGui_ImplOpenGL3_NewFrame();
		break;
	}

	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void menus::present()
{
	ImGui::EndFrame();
	ImGui::Render();

	switch (global::renderer)
	{
	case kiero::RenderType::D3D9:
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		break;

	case kiero::RenderType::D3D10:
		ImGui_ImplDX10_RenderDrawData(ImGui::GetDrawData());
		break;

	case kiero::RenderType::D3D11:
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		break;

	case kiero::RenderType::OpenGL:
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		break;
	}
}

void menus::update()
{
	ImGui::GetIO().MouseDrawCursor = !global::hide;

	if (!global::hide)
	{
		ImGui::SetNextWindowPos({ 0, 0 });
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground;
		if (ImGui::Begin("ECM", nullptr, flags))
		{
			menus::main_menu_bar();
			ImGui::End();
		}
	}
}

void menus::main_menu_bar()
{
	if (ImGui::BeginMainMenuBar())
	{
		menus::actions();
		menus::playlist();
		menus::about();

		ImGui::Text(logger::va("Listening: %s on %s", audio::currently_playing.title.c_str(), audio::playlist_name.c_str()).c_str());

		ImGui::EndMainMenuBar();
	}
}

void menus::actions()
{
	if (ImGui::BeginMenu("Actions"))
	{
		ImGui::Text("Audio Controls");
        ImGui::PushItemWidth(120.0f);

		auto save_volume_setting = [](const char* key, const int value)
		{
			if (ini_t* config = ini_load(settings::config_file.c_str()))
			{
				ini_set(config, "core", key, std::to_string(value).c_str());
				ini_save(config, settings::config_file.c_str());
				ini_free(config);
			}
		};

		auto draw_volume_slider = [&](const char* label, std::int32_t& value, const char* config_key)
		{
			if (ImGui::SliderInt(label, &value, 0, 100))
			{
				audio::apply_current_context_volume();
				save_volume_setting(config_key, value);
			}
		};

		const std::string current_context = audio::current_playlist_context();
		const bool is_frontend_context = current_context == "Frontend";
		const bool is_ingame_context = current_context == "In-game";

       if (is_ingame_context)
		{
			draw_volume_slider("Current Volume (In-game)", audio::ingame_volume, "ingame_volume");
			draw_volume_slider("Frontend Volume", audio::frontend_volume, "frontend_volume");
		}
		else
		{
			const char* current_label = is_frontend_context ? "Current Volume (Frontend)" : "Frontend Volume";
			draw_volume_slider(current_label, audio::frontend_volume, "frontend_volume");
			draw_volume_slider("In-game Volume", audio::ingame_volume, "ingame_volume");
		}

        if (ImGui::Button("Previous"))
		{
			audio::play_previous_song();
		}

		ImGui::SameLine();

		if (ImGui::Button("Skip"))
		{
         if (audio::playing)
			{
				audio::stop(0);
				audio::play_next_song();
			}
			else if (!audio::paused)
			{
				audio::play_next_song();
			}
		}

		bool shuffle_enabled = audio::shuffle_enabled;
		if (ImGui::Checkbox("Shuffle", &shuffle_enabled))
		{
			audio::shuffle_enabled = shuffle_enabled;
			audio::create_playlist_order();
			audio::current_song_index = -1;

			if (ini_t* config = ini_load(settings::config_file.c_str()))
			{
				ini_set(config, "config", "shuffle_enabled", audio::shuffle_enabled ? "true" : "false");
				ini_save(config, settings::config_file.c_str());
				ini_free(config);
			}
		}

		bool repeat_enabled = audio::repeat_enabled;
		if (ImGui::Checkbox("Repeat", &repeat_enabled))
		{
			audio::repeat_enabled = repeat_enabled;

			if (ini_t* config = ini_load(settings::config_file.c_str()))
			{
				ini_set(config, "config", "repeat_enabled", audio::repeat_enabled ? "true" : "false");
				ini_save(config, settings::config_file.c_str());
				ini_free(config);
			}
		}

		ImGui::Text("Mode: %s", audio::shuffle_enabled ? "Random" : "Sequential");
     ImGui::Text("Repeat: %s", audio::repeat_enabled ? "All" : "Off");
		ImGui::Text("Context: %s", audio::current_playlist_context());
		ImGui::Text("Active Volume: %d", audio::current_context_volume());
		ImGui::Text("Tracks: %d", audio::current_playlist_track_count());

		ImGui::EndMenu();
	}
}

void menus::about()
{
	if (ImGui::BeginMenu("About"))
	{
		ImGui::Text("ECM-R");
		ImGui::Separator();
		ImGui::TextWrapped("Fork of the original ECM (External Custom Music) project.");
		ImGui::BulletText("Original author: BttrDrgn");
		ImGui::BulletText("Current fork maintainer: RCompanyX");
		ImGui::Spacing();
		ImGui::TextWrapped("Report bugs, request features, or share ideas through GitHub Issues.");

		if (ImGui::Button("Repository"))
		{
			open_external_link(kRepositoryUrl);
		}

		ImGui::SameLine();

		if (ImGui::Button("Issues"))
		{
			open_external_link(kIssuesUrl);
		}

		ImGui::EndMenu();
	}
}

void menus::playlist()
{
	if (ImGui::BeginMenu("Playlist"))
	{
		for (int i = 0; i < audio::playlist_files.size(); ++i)
		{
			std::string song = audio::playlist_files[i].first;
			logger::rem_path_info(song, audio::playlist_dir);
			ImGui::Text("%s", song.c_str());
		}

		ImGui::EndMenu();
	}
}

void menus::build_font(ImGuiIO& io)
{
	std::string font = "ecm/fonts/NotoSans-Regular.ttf";
	std::string font_jp = "ecm/fonts/NotoSansJP-Regular.ttf";
	std::string emoji = "ecm/fonts/NotoEmoji-Regular.ttf";

	if (fs::exists(font))
	{
		io.Fonts->AddFontFromFileTTF(&font[0], 18.0f);

		static ImFontConfig cfg;
		static ImWchar emoji_ranges[] = { 0x1, 0x1FFFF, 0 };

		if (fs::exists(emoji))
		{
			cfg.MergeMode = true;
			cfg.OversampleH = cfg.OversampleV = 1;
			//cfg.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_LoadColor;	//Noto doesnt have color
			io.Fonts->AddFontFromFileTTF(&emoji[0], 12.0f, &cfg, emoji_ranges);
		}

		if (fs::exists(font_jp))
		{
			ImFontConfig cfg;
			cfg.OversampleH = cfg.OversampleV = 1;
			cfg.MergeMode = true;
			io.Fonts->AddFontFromFileTTF(&font_jp[0], 18.0f, &cfg, io.Fonts->GetGlyphRangesJapanese());
		}
	}
}
