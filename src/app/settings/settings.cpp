
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

	std::string build_version_line()
	{
		return std::string("version = \"") + VERSION + "\"";
	}

    std::string build_shuffle_enabled_line(const bool shuffle_enabled)
	{
		return std::string("shuffle_enabled = ") + (shuffle_enabled ? "true" : "false");
	}

	std::string build_repeat_enabled_line(const bool repeat_enabled)
	{
		return std::string("repeat_enabled = ") + (repeat_enabled ? "true" : "false");
	}

  std::string build_stop_music_on_loading_screens_line(const bool stop_music_on_loading_screens)
	{
		return std::string("stop_music_on_loading_screens = ") + (stop_music_on_loading_screens ? "true" : "false");
	}

	void update_config_version_only(const std::string& path)
	{
		const std::string contents = fs::read(path);
		if (contents.empty())
		{
			return;
		}

		const std::string newline = contents.find("\r\n") != std::string::npos ? "\r\n" : "\n";
		const bool has_trailing_newline = !contents.empty() && (contents.back() == '\n' || contents.back() == '\r');

		std::vector<std::string> lines;
		for (std::size_t start = 0; start < contents.size();)
		{
			const auto end = contents.find_first_of("\r\n", start);
			if (end == std::string::npos)
			{
				lines.emplace_back(contents.substr(start));
				break;
			}

			lines.emplace_back(contents.substr(start, end - start));
			start = end + 1;
			if (contents[end] == '\r' && start < contents.size() && contents[start] == '\n')
			{
				++start;
			}
		}

		std::size_t insert_index = lines.size();
		std::size_t version_index = lines.size();
		bool in_core = false;
		bool found_core = false;

		for (std::size_t i = 0; i < lines.size(); ++i)
		{
			const std::string trimmed = trim_copy(lines[i]);
			const bool is_section = trimmed.size() >= 2 && trimmed.front() == '[' && trimmed.back() == ']';

			if (is_section)
			{
				if (in_core)
				{
					insert_index = i;
					break;
				}

				in_core = trimmed == "[core]";
				found_core = found_core || in_core;
				continue;
			}

			if (!in_core)
			{
				continue;
			}

			const auto equals = lines[i].find('=');
			if (equals == std::string::npos)
			{
				continue;
			}

			if (trim_copy(lines[i].substr(0, equals)) == "version")
			{
				version_index = i;
				break;
			}
		}

		if (!found_core)
		{
			return;
		}

		const std::string version_line = build_version_line();
		if (version_index < lines.size())
		{
			lines[version_index] = version_line;
		}
		else
		{
			lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(insert_index), version_line);
		}

		std::ostringstream output;
		for (std::size_t i = 0; i < lines.size(); ++i)
		{
			if (i > 0)
			{
				output << newline;
			}
			output << lines[i];
		}

		if (has_trailing_newline)
		{
			output << newline;
		}

		if (output.str() != contents)
		{
			fs::write(path, output.str(), false);
		}
	}

    std::string build_config_text(const std::string& playlist, const int volume, const std::string& toggle_overlay, const std::string& skip_track, const std::string& previous_track, const bool stop_music_on_loading_screens, const bool shuffle_enabled, const bool repeat_enabled)
	{
		std::ostringstream output;
		output << "[core]\n";
		output << "playlist = \"" << playlist << "\"\n";
		output << "volume = \"" << volume << "\"\n";
		output << "version = \"" << VERSION << "\"\n\n";
      output << "[config]\n";
		output << build_shuffle_enabled_line(shuffle_enabled) << "\n";
		output << build_repeat_enabled_line(repeat_enabled) << "\n";
		output << build_stop_music_on_loading_screens_line(stop_music_on_loading_screens) << "\n\n";
		output << "[keys]\n";
		output << "toggle_overlay = " << toggle_overlay << "\n";
      output << "previous_track = " << previous_track << "\n";
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

     const bool version_changed = std::strcmp(safe_ini_get(config, "core", "version", ""), VERSION) != 0;
		const std::string toggle_overlay = safe_ini_get(config, "keys", "toggle_overlay", "F11");
        const std::string skip_track = safe_ini_get(config, "keys", "skip_track", "F10");
       const std::string previous_track = safe_ini_get(config, "keys", "previous_track", "F9");
		const bool missing_stop_music_on_loading_screens = ini_get(config, "config", "stop_music_on_loading_screens") == nullptr;
		const bool missing_shuffle_enabled = ini_get(config, "config", "shuffle_enabled") == nullptr;
		const bool missing_repeat_enabled = ini_get(config, "config", "repeat_enabled") == nullptr;
		const bool missing_previous_track = ini_get(config, "keys", "previous_track") == nullptr;

		audio::volume = std::stoi(safe_ini_get(config, "core", "volume", "100"));
     audio::shuffle_enabled = settings::get_boolean(safe_ini_get(config, "config", "shuffle_enabled", "true"));
		audio::repeat_enabled = settings::get_boolean(safe_ini_get(config, "config", "repeat_enabled", "true"));
		audio::stop_music_on_loading_screens = settings::get_boolean(safe_ini_get(config, "config", "stop_music_on_loading_screens", "true"));
		input::toggle_overlay_key = input::key_from_string(toggle_overlay.c_str(), VK_F11);
		input::skip_track_key = input::key_from_string(skip_track.c_str(), VK_F10);
		input::previous_track_key = input::key_from_string(previous_track.c_str(), VK_F9);

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

     if (version_changed || missing_stop_music_on_loading_screens || missing_shuffle_enabled || missing_repeat_enabled || missing_previous_track)
		{
          fs::write(settings::config_file, build_config_text(audio::playlist_name, audio::volume, toggle_overlay, skip_track, previous_track, audio::stop_music_on_loading_screens, audio::shuffle_enabled, audio::repeat_enabled), false);
		}
	}
	else if (!fs::exists(settings::config_file))
	{
		input::toggle_overlay_key = VK_F11;
		input::skip_track_key = VK_F10;
		input::previous_track_key = VK_F9;

		audio::playlist_name = "Music";
		audio::playlist_dir = audio::playlist_name;
		audio::playlist_dir = fs::get_self_path() + audio::playlist_dir;
		audio::enumerate_playlist();

		for (int i = 0; i < audio::playlist_files.size(); ++i)
		{
          audio::playlist_files[i].second = "ALL";
		}

		audio::volume = 100;
        audio::shuffle_enabled = true;
		audio::repeat_enabled = true;
		audio::stop_music_on_loading_screens = true;
     fs::write(settings::config_file, build_config_text(audio::playlist_name, audio::volume, "F11", "F10", "F9", audio::stop_music_on_loading_screens, audio::shuffle_enabled, audio::repeat_enabled), false);
		return;
	}

	ini_free(config);
}

bool settings::get_boolean(const char* bool_text)
{
   if (!bool_text)
	{
		return false;
	}

	std::string value = trim_copy(bool_text);
	if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
	{
		value = value.substr(1, value.size() - 2);
		value = trim_copy(value);
	}

	std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
		return static_cast<char>(std::tolower(ch));
	});

	return value == "true" || value == "1" || value == "yes" || value == "on";
}

//Hardcoded until x64 becomes useable
std::string settings::config_file = "ecm-r.x86.ini";
