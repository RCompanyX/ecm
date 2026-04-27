#pragma once

#ifdef _M_AMD64
std::int64_t ImGui_ImplWin32_WndProcHandler(::HWND, std::uint32_t, std::uint64_t, std::int64_t);
#else
long ImGui_ImplWin32_WndProcHandler(::HWND, std::uint32_t, std::uint32_t, long);
#endif

class input
{
public:
	static void init_overlay();
  static void update();
	static std::uint32_t key_from_string(const char* key_text, std::uint32_t default_key);

	enum callback_type : std::uint8_t
	{
		on_key_down,
		on_key_up,
	};

	enum result_type : std::uint8_t
	{
		cont,
		interrupt,
	};

	using callback = input::result_type(__cdecl*)(std::uint32_t);

	static void on(input::callback_type type, input::callback callback);
	static bool is_key_down(std::uint32_t key);

	static std::unordered_map<input::callback_type, std::vector<input::callback>> callbacks_;
  static std::uint32_t toggle_overlay_key;
	static std::uint32_t skip_track_key;
  static std::uint32_t previous_track_key;
};
