module;
#include "Core/Macros.hh"
#include "Windows.API.hh"

#include <new>
export module Vitro.Windows.AppContext;

import Vitro.App.AppContextBase;
import Vitro.App.EventSystem;
import Vitro.App.KeyCode;
import Vitro.App.MouseCode;
import Vitro.App.Window;
import Vitro.App.WindowEvent;
import Vitro.Core.Array;
import Vitro.Core.Rectangle;
import Vitro.Core.Vector;
import Vitro.Windows.Utils;

namespace vt::windows
{
	export class AppContext final : public AppContextBase
	{
	public:
		static AppContext& get()
		{
			return static_cast<AppContext&>(AppContextBase::get());
		}

		AppContext() : instance_handle(::GetModuleHandle(nullptr))
		{
			WNDCLASS const window_class {
				.style		   = CS_DBLCLKS,
				.lpfn_wnd_proc   = forward_messages,
				.h_instance	   = instance_handle,
				.lpsz_class_name = Window::WindowClassName,
			};
			ATOM registered = ::RegisterClassW(&window_class);
			vt_ensure(registered, "Failed to register window class.");

			RAWINPUTDEVICE const raw_input_device {
				.us_usage_page = 0x01, // Usage page constant for generic desktop controls
				.us_usage	 = 0x02, // Usage constant for a generic mouse
			};
			BOOL succeeded = ::RegisterRawInputDevices(&raw_input_device, 1, sizeof(RAWINPUTDEVICE));
			vt_ensure(succeeded, "Failed to register raw input device.");
		}

		void poll_events() const override
		{
			MSG message;
			while(::PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
			{
				::TranslateMessage(&message);
				::DispatchMessageW(&message);
			}
		}

		void* handle() override
		{
			return instance_handle;
		}

	private:
		HINSTANCE const instance_handle;
		unsigned		key_repeats		  = 0;
		KeyCode			last_key_code		  = {};
		Int2			last_mouse_position = {};

		static LRESULT CALLBACK forward_messages(HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param)
		{
			switch(message)
			{
				case WM_MOVE: get().on_window_move(hwnd, l_param); return 0;
				case WM_SIZE: get().on_window_size(hwnd, l_param); return 0;
				case WM_ACTIVATE: get().restore_window_cursor_state(hwnd, w_param); return 0;
				case WM_SETFOCUS: get().on_window_event<WindowFocusEvent>(hwnd); return 0;
				case WM_KILLFOCUS: get().on_window_event<WindowUnfocusEvent>(hwnd); return 0;
				case WM_PAINT: get().on_window_event<WindowPaintEvent>(hwnd); return 0;
				case WM_CLOSE: get().on_window_close(hwnd); return 0;
				case WM_SHOWWINDOW: get().on_window_show(hwnd, w_param); return 0;
				case WM_INPUT: get().on_raw_input(hwnd, l_param); break;
				case WM_KEYDOWN:
				case WM_SYSKEYDOWN: get().on_key_down(hwnd, w_param); return 0;
				case WM_KEYUP:
				case WM_SYSKEYUP: get().on_key_up(hwnd, w_param); return 0;
				case WM_CHAR:
				case WM_SYSCHAR: get().on_key_text(hwnd, w_param); return 0;
				case WM_UNICHAR: get().on_key_text(hwnd, w_param); return w_param == UNICODE_NOCHAR;
				case WM_MOUSEMOVE: get().store_last_mouse_position(l_param); return 0;
				case WM_LBUTTONDOWN: get().on_mouse_event<MouseDownEvent>(hwnd, MouseCode::Mouse1); return 0;
				case WM_RBUTTONDOWN: get().on_mouse_event<MouseDownEvent>(hwnd, MouseCode::Mouse2); return 0;
				case WM_MBUTTONDOWN: get().on_mouse_event<MouseDownEvent>(hwnd, MouseCode::Wheel); return 0;
				case WM_XBUTTONDOWN: get().on_mouse_event<MouseDownEvent>(hwnd, map_extra_mouse_button(w_param)); return true;
				case WM_LBUTTONUP: get().on_mouse_event<MouseUpEvent>(hwnd, MouseCode::Mouse1); return 0;
				case WM_RBUTTONUP: get().on_mouse_event<MouseUpEvent>(hwnd, MouseCode::Mouse2); return 0;
				case WM_MBUTTONUP: get().on_mouse_event<MouseUpEvent>(hwnd, MouseCode::Wheel); return 0;
				case WM_XBUTTONUP: get().on_mouse_event<MouseUpEvent>(hwnd, map_extra_mouse_button(w_param)); return true;
				case WM_LBUTTONDBLCLK: get().on_mouse_event<DoubleClickEvent>(hwnd, MouseCode::Mouse1); return 0;
				case WM_RBUTTONDBLCLK: get().on_mouse_event<DoubleClickEvent>(hwnd, MouseCode::Mouse2); return 0;
				case WM_MBUTTONDBLCLK: get().on_mouse_event<DoubleClickEvent>(hwnd, MouseCode::Wheel); return 0;
				case WM_XBUTTONDBLCLK: get().on_mouse_event<DoubleClickEvent>(hwnd, map_extra_mouse_button(w_param)); return true;
				case WM_MOUSEWHEEL: get().on_vertical_scroll(hwnd, w_param); return 0;
				case WM_MOUSEHWHEEL: get().on_horizontal_scroll(hwnd, w_param); return 0;
			}
			return ::DefWindowProcW(hwnd, message, w_param, l_param);
		}

		static MouseCode map_extra_mouse_button(WPARAM wp)
		{
			return static_cast<MouseCode>(HIWORD(wp) + 3);
		}

		void on_window_move(HWND hwnd, LPARAM lp)
		{
			auto window = find_window(hwnd);
			if(!window)
				return;

			Int2 position {
				.x = GET_X_LPARAM(lp),
				.y = GET_Y_LPARAM(lp),
			};
			EventSystem::notify<WindowMoveEvent>(*window, position);
		}

		void on_window_size(HWND hwnd, LPARAM lp)
		{
			auto window = find_window(hwnd);
			if(!window)
				return;

			Extent size {
				.width	= LOWORD(lp),
				.height = HIWORD(lp),
			};
			EventSystem::notify<WindowSizeEvent>(*window, size);
		}

		void restore_window_cursor_state(HWND hwnd, WPARAM wp)
		{
			auto window = find_window(hwnd);
			if(!window || window->cursor_enabled())
				return;

			if(wp & WA_ACTIVE || wp & WA_CLICKACTIVE)
				window->disable_cursor();
		}

		template<typename E> void on_window_event(HWND hwnd)
		{
			auto window = find_window(hwnd);
			if(window)
				EventSystem::notify<E>(*window);
		}

		void on_window_close(HWND hwnd)
		{
			auto window = find_window(hwnd);
			if(window)
				window->close();
		}

		void on_window_show(HWND hwnd, WPARAM wp)
		{
			if(wp)
				on_window_event<WindowOpenEvent>(hwnd);
			else
				on_window_event<WindowCloseEvent>(hwnd);
		}

		void on_raw_input(HWND hwnd, LPARAM lp)
		{
			auto window = find_window(hwnd);
			if(!window)
				return;

			UINT size;
			auto input_handle = reinterpret_cast<HRAWINPUT>(lp);

			::GetRawInputData(input_handle, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));
			Array<BYTE> bytes(size);
			::GetRawInputData(input_handle, RID_INPUT, bytes.data(), &size, sizeof(RAWINPUTHEADER));
			auto input = new(bytes.data()) RAWINPUT;

			Int2 direction {
				.x = input->data.mouse.l_last_x,
				.y = input->data.mouse.l_last_y,
			};
			EventSystem::notify<MouseMoveEvent>(*window, last_mouse_position, direction);
		}

		void on_key_down(HWND hwnd, WPARAM wp)
		{
			auto key = static_cast<KeyCode>(wp);

			if(key == last_key_code)
				++key_repeats;
			else
				key_repeats = 0;
			last_key_code = key;

			auto window = find_window(hwnd);
			if(window)
				EventSystem::notify<KeyDownEvent>(*window, key, key_repeats);
		}

		void on_key_up(HWND hwnd, WPARAM wp)
		{
			key_repeats	= 0;
			last_key_code = KeyCode::None;

			auto window = find_window(hwnd);
			if(!window)
				return;

			auto key = static_cast<KeyCode>(wp);
			EventSystem::notify<KeyUpEvent>(*window, key);
		}

		void on_key_text(HWND hwnd, WPARAM wp)
		{
			auto window = find_window(hwnd);
			if(!window)
				return;

			wchar_t const chars[] {static_cast<wchar_t>(wp), L'\0'};
			EventSystem::notify<KeyTextEvent>(*window, last_key_code, narrow_string(chars));
		}

		void store_last_mouse_position(LPARAM lp)
		{
			last_mouse_position.x = GET_X_LPARAM(lp);
			last_mouse_position.y = GET_Y_LPARAM(lp);
		}

		template<typename E> void on_mouse_event(HWND hwnd, MouseCode button)
		{
			auto window = find_window(hwnd);
			if(window)
				EventSystem::notify<E>(*window, button);
		}

		void on_vertical_scroll(HWND hwnd, WPARAM wp)
		{
			auto window = find_window(hwnd);
			if(!window)
				return;

			Float2 offset {
				.y = short(HIWORD(wp)) / float(WHEEL_DELTA),
			};
			EventSystem::notify<MouseScrollEvent>(*window, offset);
		}

		void on_horizontal_scroll(HWND hwnd, WPARAM wp)
		{
			auto window = find_window(hwnd);
			if(!window)
				return;

			Float2 offset {
				.x = short(HIWORD(wp)) / -float(WHEEL_DELTA),
			};
			EventSystem::notify<MouseScrollEvent>(*window, offset);
		}
	};
}
