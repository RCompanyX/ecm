
#include "logger/logger.hpp"
#include "fs/fs.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

#include "settings.hpp"
#include "menus/menus.hpp"
#include "hook/hook.hpp"
#include "audio/audio.hpp"
#include "input/input.hpp"

namespace
{
	std::string trim_copy(std::string value)
	{
		auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) { return std::isspace(ch) != 0; });
		auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) { return std::isspace(ch) != 0; }).base();

		if (first >= last)
		{
			return {};
		}

		return std::string(first, last);
	}

	std::string normalize_trax_value(const char* raw_value)
	{
		if (!raw_value)
		{
			return "ALL";
		}

		std::string value = trim_copy(raw_value);

		if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
		{
			value = value.substr(1, value.size() - 2);
			value = trim_copy(value);
		}

		std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
			return static_cast<char>(std::toupper(ch));
		});

		if (value == "FE" || value == "IG" || value == "ALL")
		{
			return value;
		}

		return "ALL";
	}

	const char* safe_ini_get(ini_t* config, const char* section, const char* key, const char* fallback)
	{
		const char* value = ini_get(config, section, key);
		return value ? value : fallback;
	}

	std::string build_config_text(const std::string& playlist, const int volume, const std::string& toggle_overlay, const std::string& skip_track)
	{
		std::ostringstream output;
		output << "[core]\n";
		output << "playlist = \"" << playlist << "\"\n";
		output << "volume = \"" << volume << "\"\n";
		output << "version = \"" << VERSION << "\"\n\n";
		output << "[keys]\n";
		output << "toggle_overlay = " << toggle_overlay << "\n";
		output << "skip_track = " << skip_track << "\n\n";
		output << "[trax]\n";

		for (const auto& song : audio::playlist_files)
		{
			std::string file = song.first;
			file.erase(0, audio::playlist_dir.size() + 1);
			output << file << " = " << normalize_trax_value(song.second.c_str()) << "\n";
		}

		return output.str();
	}
}

void settings::init()
{
	settings::config_file = fs::get_self_path() + settings::config_file;

	settings::update();
}

void settings::update()
{
	ini_t* config;

	if (fs::exists(settings::config_file))
	{
		config = ini_load(&settings::config_file[0]);

		if (!config)
		{
			fs::del(settings::config_file);
			settings::update();
			return;
		}

        if (strcmp(safe_ini_get(config, "core", "version", ""), VERSION))
		{
			fs::del(settings::config_file);
			settings::update();
			return;
		}

		const std::string toggle_overlay = safe_ini_get(config, "keys", "toggle_overlay", "F11");
		const std::string skip_track = safe_ini_get(config, "keys", "skip_track", "F10");

		audio::volume = std::stoi(safe_ini_get(config, "core", "volume", "100"));
		input::toggle_overlay_key = input::key_from_string(toggle_overlay.c_str(), VK_F11);
		input::skip_track_key = input::key_from_string(skip_track.c_str(), VK_F10);

		audio::playlist_name = safe_ini_get(config, "core", "playlist", "Music");
		audio::playlist_dir = audio::playlist_name;
		audio::playlist_dir = fs::get_self_path() + audio::playlist_dir;
		audio::enumerate_playlist();

		for (int i = 0; i < audio::playlist_files.size(); ++i)
		{
			std::string song = audio::playlist_files[i].first;
			song.erase(0, audio::playlist_dir.size() + 1);
          const char* res = ini_get(config, "trax", song.c_str());
			audio::playlist_files[i].second = normalize_trax_value(res);
		}

		fs::write(settings::config_file, build_config_text(audio::playlist_name, audio::volume, toggle_overlay, skip_track), false);
	}
	else if (!fs::exists(settings::config_file))
	{
		input::toggle_overlay_key = VK_F11;
		input::skip_track_key = VK_F10;

		audio::playlist_name = "Music";
		audio::playlist_dir = audio::playlist_name;
		audio::playlist_dir = fs::get_self_path() + audio::playlist_dir;
		audio::enumerate_playlist();

		for (int i = 0; i < audio::playlist_files.size(); ++i)
		{
          audio::playlist_files[i].second = "ALL";
		}

		audio::volume = 100;
		fs::write(settings::config_file, build_config_text(audio::playlist_name, audio::volume, "F11", "F10"), false);
		return;
	}

	ini_free(config);
}

bool settings::get_boolean(const char* bool_text)
{
	if (!std::strcmp(bool_text, "true")) return true;
	else return false;
}

//Hardcoded until x64 becomes useable
std::string settings::config_file = "ecm-r.x86.ini";
