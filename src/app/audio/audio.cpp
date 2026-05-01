#include "logger/logger.hpp"
#include "global.hpp"

#include "audio.hpp"
#include "bass_api.hpp"
#include "player.hpp"
#include "fs/fs.hpp"
#include "hook/hook.hpp"
#include "defs.hpp"

#include <algorithm>
#include <cctype>
#include <numeric>
#include <random>

namespace
{
	constexpr int max_playback_history_entries = 50;

	std::string to_lower_copy(std::string value)
	{
		std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
			return static_cast<char>(std::tolower(ch));
		});

		return value;
	}

	std::int32_t clamp_volume(const std::int32_t volume)
	{
		return std::clamp(volume, 0, 100);
	}

	enum class playlist_context_t : std::int32_t
	{
		all,
		frontend,
		ingame
	};

	playlist_context_t get_playlist_context()
	{
        global::state = game_state;

		switch (global::state)
		{
       case GameFlowState::LoadingFrontend:
		case GameFlowState::InFrontend:
			return playlist_context_t::frontend;

      case GameFlowState::LoadingTrack:
		case GameFlowState::LoadingRegion:
		case GameFlowState::Racing:
      case GameFlowState::UnloadingFrontend:
			return playlist_context_t::ingame;

		default:
			return playlist_context_t::all;
		}
	}

	bool is_track_valid_for_context(const std::string& track_context, const playlist_context_t playlist_context)
	{
		if (track_context == "ALL" || track_context == "N/A")
		{
			return true;
		}

		switch (playlist_context)
		{
		case playlist_context_t::frontend:
			return track_context != "IG";

		case playlist_context_t::ingame:
			return track_context != "FE";

		default:
			return true;
		}
	}

	void clear_playback_history()
	{
		audio::playback_history.clear();
		audio::playback_history_index = -1;
	}

	void record_playback_history(const int playlist_entry_index)
	{
		if (audio::playback_history_index + 1 < static_cast<int>(audio::playback_history.size()))
		{
			audio::playback_history.erase(audio::playback_history.begin() + audio::playback_history_index + 1, audio::playback_history.end());
		}

		if (static_cast<int>(audio::playback_history.size()) >= max_playback_history_entries)
		{
			audio::playback_history.erase(audio::playback_history.begin());
			if (audio::playback_history_index > 0)
			{
				--audio::playback_history_index;
			}
		}

		audio::playback_history.emplace_back(playlist_entry_index);
		audio::playback_history_index = static_cast<int>(audio::playback_history.size()) - 1;
	}

	void sync_current_song_index_from_playlist_entry(const int playlist_entry_index)
	{
		const auto it = std::find(audio::playlist_order.begin(), audio::playlist_order.end(), playlist_entry_index);
		if (it != audio::playlist_order.end())
		{
			audio::current_song_index = static_cast<int>(std::distance(audio::playlist_order.begin(), it));
		}
	}

	void play_song_from_playlist_entry(const int playlist_entry_index, const bool record_history)
	{
		if (playlist_entry_index < 0 || playlist_entry_index >= static_cast<int>(audio::playlist_files.size()))
		{
			return;
		}

     if (record_history && audio::shuffle_enabled)
		{
			record_playback_history(playlist_entry_index);
		}

		switch (global::state)
		{
		case GameFlowState::LoadingFrontend:
		case GameFlowState::InFrontend:
		case GameFlowState::LoadingTrack:
		case GameFlowState::LoadingRegion:
		case GameFlowState::Racing:
		default:
			audio::play_file(audio::playlist_files[playlist_entry_index].first, 0);
			break;
		}
	}

    void play_song_from_playlist_order(const int song_index, const bool record_history = true)
	{
		if (song_index < 0 || song_index >= static_cast<int>(audio::playlist_order.size()))
		{
			return;
		}

		int playlist_entry_index = audio::playlist_order[song_index];

		if (playlist_entry_index > static_cast<int>(audio::playlist_files.size()) - 1)
		{
			playlist_entry_index = static_cast<int>(audio::playlist_files.size()) - 1;
		}

		play_song_from_playlist_entry(playlist_entry_index, record_history);
	}

	bool ensure_playlist_order_for_current_context(const int reset_index)
	{
		const auto playlist_context = static_cast<std::int32_t>(get_playlist_context());
		if (audio::playlist_order.empty() || audio::playlist_context != playlist_context)
		{
         if (audio::playlist_context != playlist_context)
			{
				clear_playback_history();
			}

			audio::create_playlist_order();
			audio::current_song_index = reset_index;
		}

		return !audio::playlist_order.empty();
	}

	void move_current_song_index(const int delta)
	{
		const int last_song_index = static_cast<int>(audio::playlist_order.size()) - 1;
		const int next_song_index = audio::current_song_index + delta;

		if (next_song_index > last_song_index)
		{
			if (!audio::repeat_enabled)
			{
				audio::playlist_ended = true;
				audio::current_song_index = static_cast<int>(audio::playlist_order.size());
				return;
			}

			audio::create_playlist_order();
			audio::current_song_index = 0;
			return;
		}

		if (next_song_index < 0)
		{
			audio::current_song_index = audio::repeat_enabled ? last_song_index : 0;
			return;
		}

		audio::current_song_index = next_song_index;
	}

	bool try_play_song_from_history(const int history_index)
	{
		if (history_index < 0 || history_index >= static_cast<int>(audio::playback_history.size()))
		{
			return false;
		}

		const int playlist_entry_index = audio::playback_history[history_index];
		sync_current_song_index_from_playlist_entry(playlist_entry_index);
		play_song_from_playlist_entry(playlist_entry_index, false);
		return true;
	}

	void play_relative_song(const int delta)
	{
		const int reset_index = delta > 0 ? -1 : static_cast<int>(audio::playlist_order.size());
		if (!ensure_playlist_order_for_current_context(reset_index))
		{
			return;
		}

		if (audio::chan[0] != 0)
		{
			audio::stop(0);
		}

		if (audio::shuffle_enabled)
		{
			if (delta < 0 && audio::playback_history_index > 0)
			{
				--audio::playback_history_index;
				if (try_play_song_from_history(audio::playback_history_index))
				{
					return;
				}
			}
			else if (delta < 0 && audio::playback_history_index == 0)
			{
				if (try_play_song_from_history(audio::playback_history_index))
				{
					return;
				}
			}

			if (delta > 0 && audio::playback_history_index >= 0 && audio::playback_history_index < static_cast<int>(audio::playback_history.size()) - 1)
			{
				++audio::playback_history_index;
				if (try_play_song_from_history(audio::playback_history_index))
				{
					return;
				}
			}
		}

		move_current_song_index(delta);

		if (audio::current_song_index < 0 || audio::current_song_index >= static_cast<int>(audio::playlist_order.size()))
		{
			return;
		}

		play_song_from_playlist_order(audio::current_song_index);
	}
}

void audio::init()
{
	switch (global::game)
	{
	case game_t::NFSU2:
		audio::mute_detection.emplace_back("ig_playmovie.fng");
		audio::mute_detection.emplace_back("ls_psamovie.fng");
		audio::mute_detection.emplace_back("ls_thxmovie.fng");
		audio::mute_detection.emplace_back("ls_ealogo.fng");
		audio::mute_detection.emplace_back("ls_blankmovie.fng");
		audio::mute_detection.emplace_back("ug_ls_introfmv.fng");
		audio::mute_detection.emplace_back("ui_magazineback.fng");
		audio::mute_detection.emplace_back("ui_magazinereward.fng");
		audio::mute_detection.emplace_back("ui_magazineselect.fng");
		audio::mute_detection.emplace_back("ui_magazineview.fng");
		audio::mute_detection.emplace_back("ui_rewardssponsor.fng");
		audio::mute_detection.emplace_back("ui_status_dvd.fng");
		audio::mute_detection_keywords.emplace_back("comic");
		audio::mute_detection_keywords.emplace_back("fmv");
		audio::mute_detection_keywords.emplace_back("magazine");
		audio::mute_detection_keywords.emplace_back("movie");
		audio::mute_detection_keywords.emplace_back("zone_unlock");
		audio::mute_detection_keywords.emplace_back("new_zone_unlocked");
		audio::mute_detection_keywords.emplace_back("reward");
		audio::mute_detection_keywords.emplace_back("sponsor");
		break;
	}

   if (!bass_api::load())
	{
        const std::string error_message = logger::va("bass.dll could not be loaded!\n%s\nNo audio will play for this session!", bass_api::last_error().c_str());
		logger::log_error(logger::va("Failed to load bass.dll: %s", bass_api::last_error().c_str()));
		global::msg_box("ECM-R BASS", error_message.c_str());
		global::shutdown = true;
		return;
	}

	if (HIWORD(bass_api::get_version()) != bass_api::version)
	{
      global::msg_box("ECM-R BASS", "An incorrect version of BASS.DLL was loaded!");
		global::shutdown = true;
       bass_api::unload();
		return;
	}

  if (!bass_api::init_device(global::hwnd))
	{
      global::msg_box("ECM-R BASS", "Can't initialize device!\nNo audio will play for this session!");
       bass_api::unload();
		global::shutdown = true;
		return;
	}

  audio::create_playlist_order();
	audio::pause();
	audio::update();
}

void audio::update()
{
	global::state = game_state;

	const bool is_loading_state =
		global::state == GameFlowState::LoadingFrontend ||
		global::state == GameFlowState::LoadingRegion ||
		global::state == GameFlowState::LoadingTrack;

    if (audio::stop_music_on_loading_screens && is_loading_state)
	{
		if (audio::playing)
		{
			audio::stop(0);
		}

		return;
	}

	audio::apply_current_context_volume();

 std::uint32_t state = bass_api::channel_is_active(audio::chan[0]);

	switch (state)
	{
   case bass_api::active_stopped:
		audio::playing = false;
		break;
	}

	if (!audio::paused && !audio::playing)
	{
		audio::play_next_song();
	}

	if (audio::playlist_order.empty() || audio::current_song_index < 0 || audio::current_song_index > audio::playlist_order.size() - 1)
	{
		return;
	}

}

void audio::play_file(const std::string& file, int channel)
{
	std::string title = file;
	logger::rem_path_info(title, audio::playlist_dir);
	audio::currently_playing.title = title;
 audio::playlist_ended = false;
	::play_file(file.c_str(), channel);
}

void audio::stop(int channel)
{
	audio::paused = false;
	audio::playing = false;

  bass_api::stream_free(audio::chan[channel]);
	audio::chan[channel] = 0;
	audio::applied_volume = -1;

	audio::currently_playing.title = "N/A";
}

void audio::shuffle()
{
	audio::create_playlist_order();
}

const char* audio::current_playlist_context()
{
	switch (get_playlist_context())
	{
	case playlist_context_t::frontend:
		return "Frontend";

	case playlist_context_t::ingame:
		return "In-game";

	default:
		return "All";
	}
}

int audio::current_playlist_track_count()
{
	const auto playlist_context = get_playlist_context();
	int track_count = 0;

	for (const auto& track : audio::playlist_files)
	{
		if (is_track_valid_for_context(track.second, playlist_context))
		{
			++track_count;
		}
	}

	return track_count;
}

std::int32_t audio::current_context_volume()
{
	switch (get_playlist_context())
	{
	case playlist_context_t::frontend:
		return clamp_volume(audio::frontend_volume);

	case playlist_context_t::ingame:
		return clamp_volume(audio::ingame_volume);

	default:
		return clamp_volume(audio::volume);
	}
}

void audio::apply_current_context_volume()
{
	if (audio::chan[0] == 0)
	{
		return;
	}

	const std::int32_t volume = audio::current_context_volume();
	if (audio::applied_volume == volume)
	{
		return;
	}

	if (bass_api::set_channel_volume(audio::chan[0], static_cast<float>(volume) / 100.0f))
	{
		audio::applied_volume = volume;
	}
}

void audio::create_playlist_order()
{
	audio::playlist_order.clear();
	audio::playlist_ended = false;

	if (!audio::shuffle_enabled)
	{
		clear_playback_history();
	}

	if (audio::playlist_files.empty())
	{
		return;
	}

	const auto playlist_context = get_playlist_context();
	audio::playlist_context = static_cast<std::int32_t>(playlist_context);

	for (int i = 0; i < audio::playlist_files.size(); ++i)
	{
		if (is_track_valid_for_context(audio::playlist_files[i].second, playlist_context))
		{
			audio::playlist_order.emplace_back(i);
		}
	}

	if (audio::playlist_order.empty())
	{
		return;
	}

	if (audio::shuffle_enabled)
	{
        static std::random_device rd;
		static std::mt19937 mt(rd());
		std::shuffle(audio::playlist_order.begin(), audio::playlist_order.end(), mt);
	}
}

void audio::play()
{
	audio::paused = false;
   bass_api::start();
}

void audio::pause()
{
	audio::paused = true;
   bass_api::pause();
}

bool audio::should_pause_for_package(const char* package_name)
{
	if (!package_name || package_name[0] == '\0')
	{
		return false;
	}

	const std::string normalized_package_name = to_lower_copy(package_name);

	for (const auto& package : audio::mute_detection)
	{
		if (package == normalized_package_name)
		{
			return true;
		}
	}

	for (const auto& keyword : audio::mute_detection_keywords)
	{
		if (normalized_package_name.find(keyword) != std::string::npos)
		{
			return true;
		}
	}

	return false;
}

void audio::enumerate_playlist()
{
	std::vector<std::string> files = fs::get_all_files(audio::playlist_dir, audio::supported_files);
	for (std::string file : files)
	{
		audio::playlist_files.emplace_back(file, "N/A");
	}
}

void audio::play_next_song()
{
    play_relative_song(1);
}

void audio::play_previous_song()
{
    play_relative_song(-1);
}

void audio::set_volume(std::int32_t vol_in)
{
   const std::int32_t volume = clamp_volume(vol_in);
	if (audio::chan[0] != 0 && bass_api::set_channel_volume(audio::chan[0], static_cast<float>(volume) / 100.0f))
	{
		audio::applied_volume = volume;
	}
}

bool audio::paused = false;
bool audio::playing = false;
std::int32_t audio::req;
std::int32_t audio::chan[2];
std::int32_t audio::volume = 50;
std::int32_t audio::frontend_volume = 50;
std::int32_t audio::ingame_volume = 50;
std::int32_t audio::applied_volume = -1;
bool audio::stop_music_on_loading_screens = true;
bool audio::shuffle_enabled = true;
bool audio::repeat_enabled = true;
bool audio::playlist_ended = false;
playing_t audio::currently_playing = {"N/A"};
std::string audio::playlist_name = "Music";
std::string audio::playlist_dir = "Music";
std::vector<std::pair<std::string, std::string>> audio::playlist_files;
std::vector<int> audio::playlist_order;
std::vector<int> audio::playback_history;
int audio::current_song_index = 0;
int audio::playback_history_index = -1;
std::int32_t audio::playlist_context = -1;
std::initializer_list<std::string> audio::supported_files { "wav", "mp1", "mp2", "mp3", "ogg", "aif"};
std::vector<std::string> audio::mute_detection;
std::vector<std::string> audio::mute_detection_keywords;
