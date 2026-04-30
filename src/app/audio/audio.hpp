#pragma once

#define BASS_SYNC_HLS_SEGMENT	0x10300
#define BASS_TAG_HLS_EXTINF		0x14000

using namespace std::literals;

struct playing_t
{
	std::string title, where;
};

class audio
{
public:
	static void init();
	static void play_file(const std::string& file, int channel);
	static void stop(int channel);
	static void set_volume(std::int32_t vol_in);
	static void enumerate_playlist();
	static void update();
  static void create_playlist_order();
	static void shuffle();
   static const char* current_playlist_context();
	static int current_playlist_track_count();
	static std::int32_t current_context_volume();
	static void apply_current_context_volume();
	static void play_next_song();
    static void play_previous_song();
    static void refresh_current_track_normalization();
	static void pause();
	static void play();

	static std::string playlist_name;
	static std::string playlist_dir;
	static std::vector<std::pair<std::string, std::string>> playlist_files;
	static std::vector<int> playlist_order;
  static std::vector<int> playback_history;
	static int current_song_index;
   static int playback_history_index;
	static std::int32_t playlist_context;

	static std::vector<const char*> mute_detection;

	static std::initializer_list<std::string> supported_files;

	static bool paused;
	static bool playing;
	static std::int32_t req;
	static std::int32_t chan[2];

	static std::int32_t volume;
  static std::int32_t frontend_volume;
	static std::int32_t ingame_volume;
	static float applied_channel_volume;
    static float current_track_normalization_gain;
    static std::string current_track_normalization_source;
	static bool stop_music_on_loading_screens;
    static bool volume_normalization_enabled;
	static bool shuffle_enabled;
	static bool repeat_enabled;
	static bool playlist_ended;

	static playing_t currently_playing;
};
