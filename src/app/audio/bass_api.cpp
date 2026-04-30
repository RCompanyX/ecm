#include "bass_api.hpp"

#include "logger/logger.hpp"

namespace bass_api
{
    namespace
    {
        using get_version_fn = DWORD(WINAPI*)();
        using init_fn = BOOL(WINAPI*)(int, DWORD, DWORD, HWND, void*);
        using channel_is_active_fn = DWORD(WINAPI*)(DWORD);
        using channel_set_attribute_fn = BOOL(WINAPI*)(DWORD, DWORD, float);
        using stream_free_fn = BOOL(WINAPI*)(DWORD);
        using start_fn = BOOL(WINAPI*)();
        using pause_fn = BOOL(WINAPI*)();
        using set_config_fn = BOOL(WINAPI*)(DWORD, DWORD);
        using channel_get_tags_fn = const void*(WINAPI*)(DWORD, DWORD);
        using channel_get_data_fn = DWORD(WINAPI*)(DWORD, void*, DWORD);
        using stream_create_file_fn = DWORD(WINAPI*)(BOOL, const void*, unsigned long long, unsigned long long, DWORD);
        using channel_play_fn = BOOL(WINAPI*)(DWORD, BOOL);

        HMODULE module_handle = nullptr;
        get_version_fn get_version_ptr = nullptr;
        init_fn init_ptr = nullptr;
        channel_is_active_fn channel_is_active_ptr = nullptr;
        channel_set_attribute_fn channel_set_attribute_ptr = nullptr;
        stream_free_fn stream_free_ptr = nullptr;
        start_fn start_ptr = nullptr;
        pause_fn pause_ptr = nullptr;
        set_config_fn set_config_ptr = nullptr;
        channel_get_tags_fn channel_get_tags_ptr = nullptr;
        channel_get_data_fn channel_get_data_ptr = nullptr;
        stream_create_file_fn stream_create_file_ptr = nullptr;
        channel_play_fn channel_play_ptr = nullptr;
        std::string last_error_message;

        std::string format_system_error(DWORD error)
        {
            LPSTR buffer = nullptr;
            const DWORD length = FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr,
                error,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                reinterpret_cast<LPSTR>(&buffer),
                0,
                nullptr);

            if (length == 0 || buffer == nullptr)
            {
                return logger::va("Windows error %lu", error);
            }

            std::string message(buffer, length);
            LocalFree(buffer);

            while (!message.empty() && (message.back() == '\r' || message.back() == '\n'))
            {
                message.pop_back();
            }

            return logger::va("Windows error %lu: %s", error, message.c_str());
        }

        std::string get_module_directory()
        {
            char module_path[MAX_PATH]{};
            const DWORD length = GetModuleFileNameA(global::self, module_path, MAX_PATH);
            if (length == 0 || length >= MAX_PATH)
            {
                return std::string();
            }

            std::string directory(module_path, length);
            const auto separator = directory.find_last_of("\\/");
            if (separator == std::string::npos)
            {
                return std::string();
            }

            directory.erase(separator);
            return directory;
        }

        void reset()
        {
            get_version_ptr = nullptr;
            init_ptr = nullptr;
            channel_is_active_ptr = nullptr;
            channel_set_attribute_ptr = nullptr;
            stream_free_ptr = nullptr;
            start_ptr = nullptr;
            pause_ptr = nullptr;
            set_config_ptr = nullptr;
            channel_get_tags_ptr = nullptr;
            channel_get_data_ptr = nullptr;
            stream_create_file_ptr = nullptr;
            channel_play_ptr = nullptr;
        }

        template <typename T>
        bool resolve(T& target, const char* name)
        {
            target = reinterpret_cast<T>(GetProcAddress(module_handle, name));
           if (target == nullptr)
            {
                last_error_message = logger::va("Missing BASS export '%s'", name);
            }
            return target != nullptr;
        }
    }

    bool load()
    {
        if (module_handle != nullptr)
        {
            return true;
        }

     const std::string module_directory = get_module_directory();
        const std::string bass_path = module_directory.empty()
            ? std::string("bass.dll")
            : module_directory + "\\bass.dll";

        module_handle = LoadLibraryA(bass_path.c_str());
        if (module_handle == nullptr)
        {
            last_error_message = logger::va("%s (path: %s)", format_system_error(GetLastError()).c_str(), bass_path.c_str());
            reset();
            last_error_message = logger::va("%s (path: %s)", format_system_error(GetLastError()).c_str(), bass_path.c_str());
            return false;
        }

        if (!resolve(get_version_ptr, "BASS_GetVersion") ||
            !resolve(init_ptr, "BASS_Init") ||
            !resolve(channel_is_active_ptr, "BASS_ChannelIsActive") ||
            !resolve(channel_set_attribute_ptr, "BASS_ChannelSetAttribute") ||
            !resolve(stream_free_ptr, "BASS_StreamFree") ||
            !resolve(start_ptr, "BASS_Start") ||
            !resolve(pause_ptr, "BASS_Pause") ||
            !resolve(set_config_ptr, "BASS_SetConfig") ||
            !resolve(channel_get_tags_ptr, "BASS_ChannelGetTags") ||
            !resolve(channel_get_data_ptr, "BASS_ChannelGetData") ||
            !resolve(stream_create_file_ptr, "BASS_StreamCreateFile") ||
            !resolve(channel_play_ptr, "BASS_ChannelPlay"))
        {
            unload();
            return false;
        }

        return true;
    }

    void unload()
    {
        reset();
        if (module_handle != nullptr)
        {
            FreeLibrary(module_handle);
            module_handle = nullptr;
        }
    }

    bool is_available()
    {
        return module_handle != nullptr;
    }

    const std::string& last_error()
    {
        return last_error_message;
    }

    DWORD get_version()
    {
        if (get_version_ptr == nullptr)
        {
            return 0;
        }

        return get_version_ptr();
    }

    bool init_device(HWND hwnd)
    {
        return init_ptr != nullptr && init_ptr(-1, 44100, 0, hwnd, nullptr) != FALSE;
    }

    DWORD channel_is_active(DWORD channel)
    {
        if (channel_is_active_ptr == nullptr)
        {
            return active_stopped;
        }

        return channel_is_active_ptr(channel);
    }

    bool channel_set_attribute(DWORD channel, DWORD attribute, float value)
    {
        return channel_set_attribute_ptr != nullptr && channel_set_attribute_ptr(channel, attribute, value) != FALSE;
    }

    bool stream_free(DWORD channel)
    {
        return stream_free_ptr != nullptr && stream_free_ptr(channel) != FALSE;
    }

    bool start()
    {
        return start_ptr != nullptr && start_ptr() != FALSE;
    }

    bool pause()
    {
        return pause_ptr != nullptr && pause_ptr() != FALSE;
    }

    bool set_config(DWORD option, DWORD value)
    {
        return set_config_ptr != nullptr && set_config_ptr(option, value) != FALSE;
    }

    bool set_channel_volume(DWORD channel, float volume)
    {
        return channel_set_attribute(channel, attrib_vol, volume);
    }

    bool set_stream_volume_config(std::int32_t volume)
    {
        return set_config(config_gvol_stream, static_cast<DWORD>(volume * 100));
    }

    const void* channel_get_tags(DWORD channel, DWORD tags)
    {
        if (channel_get_tags_ptr == nullptr)
        {
            return nullptr;
        }

        return channel_get_tags_ptr(channel, tags);
    }

    DWORD channel_get_data(DWORD channel, void* buffer, DWORD length)
    {
        if (channel_get_data_ptr == nullptr)
        {
            return data_error;
        }

        return channel_get_data_ptr(channel, buffer, length);
    }

    stream_handle_t stream_create_file(const char* file, DWORD flags)
    {
        if (stream_create_file_ptr == nullptr)
        {
            return 0;
        }

        return stream_create_file_ptr(FALSE, file, 0, 0, flags);
    }

    bool channel_play(DWORD channel, bool restart)
    {
        return channel_play_ptr != nullptr && channel_play_ptr(channel, restart ? TRUE : FALSE) != FALSE;
    }
}
