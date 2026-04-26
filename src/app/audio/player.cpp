#include "logger/logger.hpp"
#include "fs/fs.hpp"
#include "global.hpp"
#include "audio.hpp"
#include "bass_api.hpp"
#include "hook/hook.hpp"
#include "settings/settings.hpp"

void play_file(const char* file, int channel)
{
   audio::chan[channel] = static_cast<std::int32_t>(bass_api::stream_create_file(file));

  if (audio::chan[channel] != 0 && bass_api::channel_play(audio::chan[channel], false))
	{
		audio::playing = true;
		std::string title = "N/A";
		std::string artist = "N/A";

		std::string temp = file;
		logger::rem_path_info(temp, audio::playlist_dir);
		std::vector<std::string> info = logger::split(temp, " - ");

		if (info.size() == 1)
		{
			title = info[0];
		}
		else if (info.size() == 2)
		{
			artist = info[0];
			title = info[1].erase(0, 2);
		}

		hook::SummonChyron(title.c_str(), artist.c_str(), audio::playlist_name.c_str());
	}
}
