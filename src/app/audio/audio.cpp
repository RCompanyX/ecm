#include "logger/logger.hpp"
#include "global.hpp"

#include "audio.hpp"
#include "bass_api.hpp"
#include "player.hpp"
#include "fs/fs.hpp"
#include "hook/hook.hpp"
#include "defs.hpp"

#include <algorithm>
#include <numeric>
#include <random>

namespace
{
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
}

void audio::init()
{
	switch (global::game)
	{
	case game_t::NFSU2:
		//WIP filter detection
		audio::mute_detection.emplace_back("LS_PSAMovie.fng");
		audio::mute_detection.emplace_back("LS_THXMovie.fng");
		audio::mute_detection.emplace_back("LS_EAlogo.fng");
		audio::mute_detection.emplace_back("LS_BlankMovie.fng");
		audio::mute_detection.emplace_back("UG_LS_IntroFMV.fng");
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

	audio::set_volume(audio::volume);
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

void audio::create_playlist_order()
{
	audio::playlist_order.clear();
	audio::playlist_ended = false;

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
    const auto playlist_context = static_cast<std::int32_t>(get_playlist_context());
	if (audio::playlist_order.empty() || audio::playlist_context != playlist_context)
	{
		audio::create_playlist_order();
		audio::current_song_index = -1;
	}

	audio::current_song_index++;

 if (audio::playlist_order.empty())
	{
		return;
	}

	//If our playlist has ended
	if (audio::current_song_index > audio::playlist_order.size() - 1)
	{
       if (!audio::repeat_enabled)
		{
			audio::playlist_ended = true;
			audio::current_song_index = static_cast<int>(audio::playlist_order.size());
			return;
		}

		audio::create_playlist_order();
		audio::current_song_index = 0;
	}

	if (audio::playlist_order.size() > 0)
	{
		int next = audio::playlist_order[audio::current_song_index];

		if (next > audio::playlist_files.size() - 1)
		{
			next = audio::playlist_files.size() - 1;
		}

        switch (global::state)
		{
      case GameFlowState::LoadingFrontend:
		case GameFlowState::InFrontend:
		case GameFlowState::LoadingTrack:
		case GameFlowState::LoadingRegion:
		case GameFlowState::Racing:
		default:
			audio::play_file(audio::playlist_files[next].first, 0);
			break;
		}
	}
}

void audio::set_volume(std::int32_t vol_in)
{
  bass_api::set_stream_volume_config(vol_in);
}

bool audio::paused = false;
bool audio::playing = false;
std::int32_t audio::req;
std::int32_t audio::chan[2];
std::int32_t audio::volume = 50;
bool audio::stop_music_on_loading_screens = true;
bool audio::shuffle_enabled = true;
bool audio::repeat_enabled = true;
bool audio::playlist_ended = false;
playing_t audio::currently_playing = {"N/A"};
std::string audio::playlist_name = "Music";
std::string audio::playlist_dir = "Music";
std::vector<std::pair<std::string, std::string>> audio::playlist_files;
std::vector<int> audio::playlist_order;
int audio::current_song_index = 0;
std::int32_t audio::playlist_context = -1;
std::initializer_list<std::string> audio::supported_files { "wav", "mp1", "mp2", "mp3", "ogg", "aif"};
std::vector<const char*> audio::mute_detection;
