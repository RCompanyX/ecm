#include "global.hpp"

#include "logger/logger.hpp"
#include "audio/audio.hpp"

#include "input.hpp"

WNDPROC o_wndproc;
bool toggle_once = false;
bool skip_once = false;
bool previous_once = false;
std::unordered_map<input::callback_type, std::vector<input::callback>> input::callbacks_;

namespace
{
	void skip_to_next_track()
	{
		if (audio::playing)
		{
			audio::stop(0);
			audio::play_next_song();
		}
		else if (!audio::paused)
		{
			audio::play_next_song();
		}
	}

	void skip_to_previous_track()
	{
		audio::play_previous_song();
	}
}

std::uint32_t input::key_from_string(const char* key_text, std::uint32_t default_key)
{
	if (!key_text || !key_text[0])
	{
		return default_key;
	}

	if ((key_text[0] == 'F' || key_text[0] == 'f') && key_text[1] != '\0')
	{
		int function_key = std::atoi(key_text + 1);
		if (function_key >= 1 && function_key <= 24)
		{
			return VK_F1 + function_key - 1;
		}
	}

	if (key_text[1] == '\0')
	{
		if (key_text[0] >= '0' && key_text[0] <= '9')
		{
			return key_text[0];
		}

		if (key_text[0] >= 'A' && key_text[0] <= 'Z')
		{
			return key_text[0];
		}

		if (key_text[0] >= 'a' && key_text[0] <= 'z')
		{
			return key_text[0] - 32;
		}
	}

	if (!_stricmp(key_text, "SPACE")) return VK_SPACE;
	if (!_stricmp(key_text, "TAB")) return VK_TAB;
	if (!_stricmp(key_text, "ENTER")) return VK_RETURN;
	if (!_stricmp(key_text, "ESC") || !_stricmp(key_text, "ESCAPE")) return VK_ESCAPE;
	if (!_stricmp(key_text, "BACKSPACE")) return VK_BACK;
	if (!_stricmp(key_text, "INSERT")) return VK_INSERT;
	if (!_stricmp(key_text, "DELETE")) return VK_DELETE;
	if (!_stricmp(key_text, "HOME")) return VK_HOME;
	if (!_stricmp(key_text, "END")) return VK_END;
	if (!_stricmp(key_text, "PAGEUP")) return VK_PRIOR;
	if (!_stricmp(key_text, "PAGEDOWN")) return VK_NEXT;
	if (!_stricmp(key_text, "UP")) return VK_UP;
	if (!_stricmp(key_text, "DOWN")) return VK_DOWN;
	if (!_stricmp(key_text, "LEFT")) return VK_LEFT;
	if (!_stricmp(key_text, "RIGHT")) return VK_RIGHT;

	return default_key;
}

LRESULT __stdcall wndproc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);

	switch (uMsg)
	{

	case WM_SYSKEYDOWN:
		if (wParam == VK_RETURN)
		{
			if ((HIWORD(lParam) & KF_ALTDOWN))
			{
				return 1;
			}
		}
		break;

	case WM_KEYDOWN:
		for (const auto callback : input::callbacks_[input::callback_type::on_key_down])
		{
			callback(wParam);
		}
		break;

	case WM_KEYUP:
		for (const auto callback : input::callbacks_[input::callback_type::on_key_up])
		{
			callback(wParam);
		}
		break;

	case WM_SYSCOMMAND:
		if ((wParam & 0xFF00) == SC_KEYMENU)
		{
			return 1;
		}
	}


	return CallWindowProcA(o_wndproc, hWnd, uMsg, wParam, lParam);
}

void input::init_overlay()
{
	o_wndproc = (WNDPROC)SetWindowLongW(global::hwnd, GWLP_WNDPROC, (LONG_PTR)wndproc);

	input::on(input::callback_type::on_key_down, [](auto key) -> input::result_type
	{
     if (key == input::toggle_overlay_key && !toggle_once)
		{
			global::hide = !global::hide;
            toggle_once = true;
		}

        if (key == input::skip_track_key && !skip_once)
		{
         skip_to_next_track();
			skip_once = true;
		}

		if (key == input::previous_track_key && !previous_once)
		{
			skip_to_previous_track();
			previous_once = true;
		}

		return input::result_type::cont;
	});

	input::on(input::callback_type::on_key_up, [](auto key) -> input::result_type
	{
      if (key == input::toggle_overlay_key && toggle_once)
		{
           toggle_once = false;
		}

     if (key == input::skip_track_key && skip_once)
		{
			skip_once = false;
		}

		if (key == input::previous_track_key && previous_once)
		{
			previous_once = false;
		}

		return input::result_type::cont;
	});
}


void input::update()
{
	const bool skip_down = (GetAsyncKeyState(input::skip_track_key) & 0x8000) != 0;
	if (skip_down && !skip_once)
	{
     skip_to_next_track();
		skip_once = true;
	}
	else if (!skip_down && skip_once)
	{
		skip_once = false;
	}

	const bool previous_down = (GetAsyncKeyState(input::previous_track_key) & 0x8000) != 0;
	if (previous_down && !previous_once)
	{
		skip_to_previous_track();
		previous_once = true;
	}
	else if (!previous_down && previous_once)
	{
		previous_once = false;
	}
}


void input::on(input::callback_type type, input::callback callback)
{
	input::callbacks_[type].emplace_back(callback);
}

bool input::is_key_down(std::uint32_t key)
{
	return ImGui::IsKeyPressed(key, false);
}

std::uint32_t input::toggle_overlay_key = VK_F11;
std::uint32_t input::skip_track_key = VK_F10;
std::uint32_t input::previous_track_key = VK_F9;
