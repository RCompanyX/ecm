#include "logger/logger.hpp"
#include "global.hpp"

#include "audio.hpp"
#include "bass_api.hpp"
#include "player.hpp"
#include "fs/fs.hpp"
#include "hook/hook.hpp"
#include "defs.hpp"

#include <array>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <numeric>
#include <random>

namespace
{
    constexpr int max_playback_history_entries = 50;
    constexpr float normalization_target_rms = 0.16f;
    constexpr float normalization_min_gain_db = -12.0f;
    constexpr float normalization_max_gain_db = 12.0f;
    constexpr float max_channel_volume = 4.0f;
    constexpr std::size_t analysis_buffer_float_count = 8192;
    constexpr float minimum_detectable_rms = 0.0001f;
    constexpr float minimum_gain_multiplier_delta = 0.001f;

    struct normalization_result_t
    {
        float gain_multiplier = 1.0f;
        std::string source = "Unavailable";
        bool available = false;
    };

    struct normalization_cache_entry_t
    {
        std::string signature;
        float gain_multiplier = 1.0f;
        std::string source = "Unavailable";
    };

    std::unordered_map<std::string, normalization_cache_entry_t> normalization_cache;
    bool normalization_cache_loaded = false;
    bool normalization_cache_dirty = false;

	std::int32_t clamp_volume(const std::int32_t volume)
	{
		return std::clamp(volume, 0, 100);
	}

    float clamp_channel_volume(const float volume)
    {
        return std::clamp(volume, 0.0f, max_channel_volume);
    }

    std::string trim_copy(std::string value)
    {
        const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) { return std::isspace(ch) != 0; });
        const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) { return std::isspace(ch) != 0; }).base();

        if (first >= last)
        {
            return {};
        }

        return std::string(first, last);
    }

    std::string uppercase_copy(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
            return static_cast<char>(std::toupper(ch));
        });
        return value;
    }

    std::string get_relative_playlist_path(const std::string& file)
    {
        if (file.rfind(audio::playlist_dir + "\\", 0) == 0)
        {
            return file.substr(audio::playlist_dir.size() + 1);
        }

        if (file.rfind(audio::playlist_dir + "/", 0) == 0)
        {
            return file.substr(audio::playlist_dir.size() + 1);
        }

        return file;
    }

    std::string normalization_cache_path()
    {
        return fs::get_self_path() + "ecm-r.volume-cache.txt";
    }

    std::string build_track_signature(const std::string& file)
    {
        try
        {
            const auto size = std::filesystem::file_size(std::filesystem::path(file));
            const auto last_write = std::filesystem::last_write_time(std::filesystem::path(file)).time_since_epoch().count();
            return std::to_string(size) + ":" + std::to_string(last_write);
        }
        catch (const std::exception&)
        {
            return {};
        }
    }

    void load_normalization_cache()
    {
        if (normalization_cache_loaded)
        {
            return;
        }

        normalization_cache_loaded = true;
        normalization_cache.clear();

        if (!fs::exists(normalization_cache_path()))
        {
            return;
        }

        std::istringstream input(fs::read(normalization_cache_path()));
        std::string line;
        while (std::getline(input, line))
        {
            line = trim_copy(line);
            if (line.empty())
            {
                continue;
            }

            const std::size_t first = line.find('|');
            const std::size_t second = first == std::string::npos ? std::string::npos : line.find('|', first + 1);
            const std::size_t third = second == std::string::npos ? std::string::npos : line.find('|', second + 1);
            if (first == std::string::npos || second == std::string::npos || third == std::string::npos)
            {
                continue;
            }

            const std::string relative_path = line.substr(0, first);
            const std::string signature = line.substr(first + 1, second - first - 1);
            const std::string gain_text = line.substr(second + 1, third - second - 1);
            const std::string source = line.substr(third + 1);

            try
            {
                normalization_cache[relative_path] = {
                    signature,
                    std::stof(gain_text),
                    source
                };
            }
            catch (const std::exception&)
            {
                continue;
            }
        }
    }

    void save_normalization_cache()
    {
        if (!normalization_cache_dirty)
        {
            return;
        }

        std::vector<std::string> relative_paths;
        relative_paths.reserve(normalization_cache.size());
        for (const auto& [relative_path, entry] : normalization_cache)
        {
            relative_paths.emplace_back(relative_path);
        }

        std::sort(relative_paths.begin(), relative_paths.end());

        std::ostringstream output;
        output << std::fixed << std::setprecision(6);
        for (const auto& relative_path : relative_paths)
        {
            const auto it = normalization_cache.find(relative_path);
            if (it == normalization_cache.end())
            {
                continue;
            }

            output << relative_path << '|'
                   << it->second.signature << '|'
                   << it->second.gain_multiplier << '|'
                   << it->second.source << '\n';
        }

        fs::write(normalization_cache_path(), output.str(), false);
        normalization_cache_dirty = false;
    }

    float db_to_gain_multiplier(const float gain_db)
    {
        return std::pow(10.0f, gain_db / 20.0f);
    }

    float clamp_gain_db_by_peak(const float gain_db, const float peak)
    {
        float clamped_gain_db = std::clamp(gain_db, normalization_min_gain_db, normalization_max_gain_db);
        if (peak > 0.0f)
        {
            clamped_gain_db = std::min(clamped_gain_db, -20.0f * std::log10(peak));
        }

        return std::clamp(clamped_gain_db, normalization_min_gain_db, normalization_max_gain_db);
    }

    bool try_parse_float_prefix(const std::string& value, float& parsed_value)
    {
        const std::string trimmed = trim_copy(value);
        if (trimmed.empty())
        {
            return false;
        }

        try
        {
            std::size_t processed = 0;
            parsed_value = std::stof(trimmed, &processed);
            return processed > 0;
        }
        catch (const std::exception&)
        {
            return false;
        }
    }

    bool try_extract_replaygain_value(const char* tags, float& gain_db, float& peak)
    {
        if (tags == nullptr)
        {
            return false;
        }

        std::string gain_value;
        std::string peak_value;
        std::string album_gain_value;
        std::string album_peak_value;

        for (const char* tag = tags; *tag != '\0'; tag += std::strlen(tag) + 1)
        {
            const std::string entry(tag);
            const auto separator = entry.find('=');
            if (separator == std::string::npos)
            {
                continue;
            }

            const std::string key = uppercase_copy(trim_copy(entry.substr(0, separator)));
            const std::string value = trim_copy(entry.substr(separator + 1));

            if (key == "REPLAYGAIN_TRACK_GAIN")
            {
                gain_value = value;
            }
            else if (key == "REPLAYGAIN_TRACK_PEAK")
            {
                peak_value = value;
            }
            else if (key == "REPLAYGAIN_ALBUM_GAIN")
            {
                album_gain_value = value;
            }
            else if (key == "REPLAYGAIN_ALBUM_PEAK")
            {
                album_peak_value = value;
            }
        }

        const std::string effective_gain_value = !gain_value.empty() ? gain_value : album_gain_value;
        const std::string effective_peak_value = !peak_value.empty() ? peak_value : album_peak_value;
        if (effective_gain_value.empty() || !try_parse_float_prefix(effective_gain_value, gain_db))
        {
            return false;
        }

        peak = 0.0f;
        if (!effective_peak_value.empty())
        {
            try_parse_float_prefix(effective_peak_value, peak);
        }

        return true;
    }

    std::uint32_t read_big_endian_u32(const unsigned char* data)
    {
        return
            (static_cast<std::uint32_t>(data[0]) << 24) |
            (static_cast<std::uint32_t>(data[1]) << 16) |
            (static_cast<std::uint32_t>(data[2]) << 8) |
            static_cast<std::uint32_t>(data[3]);
    }

    std::uint32_t read_synchsafe_u32(const unsigned char* data)
    {
        return
            (static_cast<std::uint32_t>(data[0] & 0x7F) << 21) |
            (static_cast<std::uint32_t>(data[1] & 0x7F) << 14) |
            (static_cast<std::uint32_t>(data[2] & 0x7F) << 7) |
            static_cast<std::uint32_t>(data[3] & 0x7F);
    }

    std::size_t find_id3_text_terminator(const unsigned char* data, const std::size_t size, const unsigned char encoding)
    {
        if (encoding == 0 || encoding == 3)
        {
            for (std::size_t i = 0; i < size; ++i)
            {
                if (data[i] == 0)
                {
                    return i;
                }
            }

            return size;
        }

        for (std::size_t i = 0; i + 1 < size; i += 2)
        {
            if (data[i] == 0 && data[i + 1] == 0)
            {
                return i;
            }
        }

        return size;
    }

    std::string decode_id3_text(const unsigned char* data, const std::size_t size, const unsigned char encoding)
    {
        if (data == nullptr || size == 0)
        {
            return {};
        }

        if (encoding == 0 || encoding == 3)
        {
            return trim_copy(std::string(reinterpret_cast<const char*>(data), size));
        }

        bool big_endian = encoding == 2;
        std::size_t offset = 0;
        if (encoding == 1 && size >= 2)
        {
            if (data[0] == 0xFE && data[1] == 0xFF)
            {
                big_endian = true;
                offset = 2;
            }
            else if (data[0] == 0xFF && data[1] == 0xFE)
            {
                big_endian = false;
                offset = 2;
            }
        }

        std::string decoded;
        decoded.reserve(size / 2);

        for (; offset + 1 < size; offset += 2)
        {
            const std::uint16_t code_unit = big_endian
                ? static_cast<std::uint16_t>((data[offset] << 8) | data[offset + 1])
                : static_cast<std::uint16_t>((data[offset + 1] << 8) | data[offset]);

            if (code_unit == 0)
            {
                break;
            }

            decoded.push_back(code_unit < 0x80 ? static_cast<char>(code_unit) : '?');
        }

        return trim_copy(decoded);
    }

    bool try_extract_replaygain_from_id3v2(const void* raw_tags, float& gain_db, float& peak)
    {
        const auto* data = static_cast<const unsigned char*>(raw_tags);
        if (data == nullptr || std::memcmp(data, "ID3", 3) != 0)
        {
            return false;
        }

        const unsigned char major_version = data[3];
        if (major_version != 3 && major_version != 4)
        {
            return false;
        }

        std::size_t total_size = 10 + read_synchsafe_u32(data + 6);
        if ((data[5] & 0x10) != 0)
        {
            total_size += 10;
        }

        std::string gain_value;
        std::string peak_value;
        std::string album_gain_value;
        std::string album_peak_value;

        for (std::size_t offset = 10; offset + 10 <= total_size; )
        {
            if (data[offset] == 0)
            {
                break;
            }

            const std::string frame_id(reinterpret_cast<const char*>(data + offset), 4);
            const std::uint32_t frame_size = major_version == 4
                ? read_synchsafe_u32(data + offset + 4)
                : read_big_endian_u32(data + offset + 4);

            if (frame_size == 0 || offset + 10 + frame_size > total_size)
            {
                break;
            }

            if (frame_id == "TXXX" && frame_size > 1)
            {
                const auto* frame_data = data + offset + 10;
                const unsigned char encoding = frame_data[0];
                const auto* text_data = frame_data + 1;
                const std::size_t text_size = frame_size - 1;
                const std::size_t description_end = find_id3_text_terminator(text_data, text_size, encoding);
                const std::string description = uppercase_copy(decode_id3_text(text_data, description_end, encoding));
                const std::size_t separator_size = (encoding == 0 || encoding == 3) ? 1 : 2;
                const std::size_t value_offset = std::min(description_end + separator_size, text_size);
                const std::string value = decode_id3_text(text_data + value_offset, text_size - value_offset, encoding);

                if (description == "REPLAYGAIN_TRACK_GAIN")
                {
                    gain_value = value;
                }
                else if (description == "REPLAYGAIN_TRACK_PEAK")
                {
                    peak_value = value;
                }
                else if (description == "REPLAYGAIN_ALBUM_GAIN")
                {
                    album_gain_value = value;
                }
                else if (description == "REPLAYGAIN_ALBUM_PEAK")
                {
                    album_peak_value = value;
                }
            }

            offset += 10 + frame_size;
        }

        const std::string effective_gain_value = !gain_value.empty() ? gain_value : album_gain_value;
        const std::string effective_peak_value = !peak_value.empty() ? peak_value : album_peak_value;
        if (effective_gain_value.empty() || !try_parse_float_prefix(effective_gain_value, gain_db))
        {
            return false;
        }

        peak = 0.0f;
        if (!effective_peak_value.empty())
        {
            try_parse_float_prefix(effective_peak_value, peak);
        }

        return true;
    }

    normalization_result_t build_metadata_normalization_result(const float gain_db, const float peak)
    {
        normalization_result_t result;
        result.gain_multiplier = db_to_gain_multiplier(clamp_gain_db_by_peak(gain_db, peak));
        result.source = "Metadata";
        result.available = true;
        return result;
    }

    normalization_result_t try_extract_metadata_normalization(const bass_api::stream_handle_t stream)
    {
        float gain_db = 0.0f;
        float peak = 0.0f;

        if (try_extract_replaygain_value(static_cast<const char*>(bass_api::channel_get_tags(stream, bass_api::tag_ogg)), gain_db, peak))
        {
            return build_metadata_normalization_result(gain_db, peak);
        }

        if (try_extract_replaygain_value(static_cast<const char*>(bass_api::channel_get_tags(stream, bass_api::tag_ape)), gain_db, peak))
        {
            return build_metadata_normalization_result(gain_db, peak);
        }

        if (try_extract_replaygain_from_id3v2(bass_api::channel_get_tags(stream, bass_api::tag_id3v2), gain_db, peak))
        {
            return build_metadata_normalization_result(gain_db, peak);
        }

        return {};
    }

    normalization_result_t analyze_stream_normalization(const bass_api::stream_handle_t stream)
    {
        normalization_result_t result;
        std::array<float, analysis_buffer_float_count> buffer{};
        double sum_of_squares = 0.0;
        float peak = 0.0f;
        std::uint64_t sample_count = 0;

        while (true)
        {
            const DWORD bytes_read = bass_api::channel_get_data(
                stream,
                buffer.data(),
                static_cast<DWORD>(buffer.size() * sizeof(float)) | bass_api::data_float);

            if (bytes_read == 0 || bytes_read == bass_api::data_error)
            {
                break;
            }

            const std::size_t floats_read = bytes_read / sizeof(float);
            for (std::size_t i = 0; i < floats_read; ++i)
            {
                const float sample = buffer[i];
                sum_of_squares += static_cast<double>(sample) * static_cast<double>(sample);
                peak = std::max(peak, std::abs(sample));
            }

            sample_count += floats_read;
        }

        if (sample_count == 0)
        {
            return result;
        }

        const float rms = static_cast<float>(std::sqrt(sum_of_squares / static_cast<double>(sample_count)));
        if (rms <= minimum_detectable_rms)
        {
            return result;
        }

        const float gain_db = clamp_gain_db_by_peak(20.0f * std::log10(normalization_target_rms / rms), peak);
        result.gain_multiplier = db_to_gain_multiplier(gain_db);
        result.source = "Analyzed";
        result.available = true;
        return result;
    }

    normalization_result_t resolve_normalization_for_track(const std::string& file)
    {
        normalization_result_t result;
        result.source = audio::volume_normalization_enabled ? "Unavailable" : "Disabled";

        if (!audio::volume_normalization_enabled)
        {
            return result;
        }

        load_normalization_cache();

        const std::string relative_path = get_relative_playlist_path(file);
        const std::string signature = build_track_signature(file);

        const auto cached = normalization_cache.find(relative_path);
        if (!signature.empty() && cached != normalization_cache.end() && cached->second.signature == signature)
        {
            result.gain_multiplier = cached->second.gain_multiplier;
            result.source = cached->second.source;
            result.available = cached->second.source != "Unavailable";
            return result;
        }

        const auto stream = bass_api::stream_create_file(file.c_str(), bass_api::sample_float | bass_api::stream_prescan | bass_api::stream_decode);
        if (stream == 0)
        {
            if (!relative_path.empty() && !signature.empty())
            {
                normalization_cache[relative_path] = { signature, 1.0f, "Unavailable" };
                normalization_cache_dirty = true;
                save_normalization_cache();
            }

            return result;
        }

        result = try_extract_metadata_normalization(stream);
        if (!result.available)
        {
            result = analyze_stream_normalization(stream);
        }

        bass_api::stream_free(stream);

        if (!relative_path.empty() && !signature.empty())
        {
            normalization_cache[relative_path] = { signature, result.gain_multiplier, result.source };
            normalization_cache_dirty = true;
            save_normalization_cache();
        }

        return result;
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
    audio::currently_playing.where = file;
  audio::playlist_ended = false;
    const auto normalization = resolve_normalization_for_track(file);
    audio::current_track_normalization_gain = normalization.gain_multiplier;
    audio::current_track_normalization_source = normalization.source;
    audio::applied_channel_volume = -1.0f;
	::play_file(file.c_str(), channel);
}

void audio::stop(int channel)
{
	audio::paused = false;
	audio::playing = false;

  bass_api::stream_free(audio::chan[channel]);
	audio::chan[channel] = 0;
	audio::applied_channel_volume = -1.0f;
    audio::current_track_normalization_gain = 1.0f;
    audio::current_track_normalization_source = audio::volume_normalization_enabled ? "Idle" : "Disabled";

	audio::currently_playing.title = "N/A";
    audio::currently_playing.where = "N/A";
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
    const float effective_volume = clamp_channel_volume((static_cast<float>(volume) / 100.0f) * audio::current_track_normalization_gain);
	if (std::abs(audio::applied_channel_volume - effective_volume) < minimum_gain_multiplier_delta)
	{
		return;
	}

	if (bass_api::set_channel_volume(audio::chan[0], effective_volume))
	{
		audio::applied_channel_volume = effective_volume;
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

void audio::refresh_current_track_normalization()
{
    audio::current_track_normalization_gain = 1.0f;
    audio::current_track_normalization_source = audio::volume_normalization_enabled ? "Idle" : "Disabled";
    audio::applied_channel_volume = -1.0f;

    if (!audio::volume_normalization_enabled || audio::chan[0] == 0)
    {
        audio::apply_current_context_volume();
        return;
    }

    if (audio::current_song_index < 0 || audio::current_song_index >= static_cast<int>(audio::playlist_order.size()))
    {
        if (audio::currently_playing.where != "N/A")
        {
            const auto normalization = resolve_normalization_for_track(audio::currently_playing.where);
            audio::current_track_normalization_gain = normalization.gain_multiplier;
            audio::current_track_normalization_source = normalization.source;
        }

        audio::apply_current_context_volume();
        return;
    }

    const int playlist_entry_index = audio::playlist_order[audio::current_song_index];
    if (playlist_entry_index < 0 || playlist_entry_index >= static_cast<int>(audio::playlist_files.size()))
    {
        audio::apply_current_context_volume();
        return;
    }

    const auto normalization = resolve_normalization_for_track(audio::playlist_files[playlist_entry_index].first);
    audio::current_track_normalization_gain = normalization.gain_multiplier;
    audio::current_track_normalization_source = normalization.source;
    audio::apply_current_context_volume();
}

void audio::set_volume(std::int32_t vol_in)
{
   const std::int32_t volume = clamp_volume(vol_in);
    const float effective_volume = clamp_channel_volume((static_cast<float>(volume) / 100.0f) * audio::current_track_normalization_gain);
	if (audio::chan[0] != 0 && bass_api::set_channel_volume(audio::chan[0], effective_volume))
	{
		audio::applied_channel_volume = effective_volume;
	}
}

bool audio::paused = false;
bool audio::playing = false;
std::int32_t audio::req;
std::int32_t audio::chan[2];
std::int32_t audio::volume = 50;
std::int32_t audio::frontend_volume = 50;
std::int32_t audio::ingame_volume = 50;
float audio::applied_channel_volume = -1.0f;
float audio::current_track_normalization_gain = 1.0f;
std::string audio::current_track_normalization_source = "Disabled";
bool audio::stop_music_on_loading_screens = true;
bool audio::volume_normalization_enabled = false;
bool audio::shuffle_enabled = true;
bool audio::repeat_enabled = true;
bool audio::playlist_ended = false;
playing_t audio::currently_playing = {"N/A", "N/A"};
std::string audio::playlist_name = "Music";
std::string audio::playlist_dir = "Music";
std::vector<std::pair<std::string, std::string>> audio::playlist_files;
std::vector<int> audio::playlist_order;
std::vector<int> audio::playback_history;
int audio::current_song_index = 0;
int audio::playback_history_index = -1;
std::int32_t audio::playlist_context = -1;
std::initializer_list<std::string> audio::supported_files { "wav", "mp1", "mp2", "mp3", "ogg", "aif"};
std::vector<const char*> audio::mute_detection;
