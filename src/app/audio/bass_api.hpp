#pragma once

#include <Windows.h>
#include <cstdint>
#include <string>

namespace bass_api
{
    using stream_handle_t = DWORD;

    inline constexpr DWORD version = 0x204;
    inline constexpr DWORD active_stopped = 0;
    inline constexpr DWORD attrib_vol = 2;
    inline constexpr DWORD sample_float = 0x100;
    inline constexpr DWORD stream_prescan = 0x20000;
    inline constexpr DWORD stream_decode = 0x200000;
    inline constexpr DWORD config_gvol_stream = 5;
    inline constexpr DWORD data_float = 0x40000000;
    inline constexpr DWORD data_error = 0xFFFFFFFF;
    inline constexpr DWORD tag_ogg = 2;
    inline constexpr DWORD tag_ape = 6;
    inline constexpr DWORD tag_id3v2 = 13;

    bool load();
    void unload();
    bool is_available();
    const std::string& last_error();
    DWORD get_version();
    bool init_device(HWND hwnd);
    DWORD channel_is_active(DWORD channel);
    bool channel_set_attribute(DWORD channel, DWORD attribute, float value);
    bool stream_free(DWORD channel);
    bool start();
    bool pause();
    bool set_config(DWORD option, DWORD value);
    bool set_channel_volume(DWORD channel, float volume);
    bool set_stream_volume_config(std::int32_t volume);
    const void* channel_get_tags(DWORD channel, DWORD tags);
    DWORD channel_get_data(DWORD channel, void* buffer, DWORD length);
    stream_handle_t stream_create_file(const char* file, DWORD flags = sample_float | stream_prescan);
    bool channel_play(DWORD channel, bool restart);
}
