
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>
#include <atomic>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>

#ifndef _WIN32_IE
#define _WIN32_IE 0x0600
#endif
#if defined(_WIN32)
#include <direct.h>

#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include "resource.h"
#endif

#include <olectl.h>
#include <ole2.h>
#include <ocidl.h>

#include <string>
#include <iostream>
#include <fstream>
#include <cstdlib>

#include <boost/scope_exit.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include <boost/asio/io_service.hpp>

#include <windows.h>

#include "boost/avloop.hpp"

class auto_handle
{
	std::shared_ptr<void> m_handle;
public:
	explicit auto_handle(HANDLE handle)
		: m_handle(handle, CloseHandle)
	{
	}

	operator HANDLE ()
	{
		return (HANDLE)m_handle.get();
	}
};


typedef boost::function<bool(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)> av_dlgproc_t;

namespace detail {
static BOOL CALLBACK internal_clusure_dlg_proc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
}


struct input_box_get_input_with_image_settings
{
	std::string imagedata;
	std::function<void(boost::system::error_code, std::string)> donecallback;
	LPPICTURE pPic;
	boost::asio::io_service * io_service;
	UINT_PTR timerid;
	int remain_time;
	~input_box_get_input_with_image_settings()
	{
		if (pPic)
			pPic->Release();
	}
};

typedef std::shared_ptr<input_box_get_input_with_image_settings> input_box_get_input_with_image_settings_ptr;

static bool input_box_get_input_with_image_dlgproc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam, input_box_get_input_with_image_settings_ptr settings)
{
	switch (message)
	{
	case WM_INITDIALOG:
		{
			HBITMAP hbitmap = CreateBitmap(32, 32, 3, 32, 0);
			HGLOBAL hglobal = GlobalAlloc(GMEM_MOVEABLE, settings->imagedata.size());

			{
				LPVOID  locked_mem = GlobalLock(hglobal);
				std::copy(settings->imagedata.begin(), settings->imagedata.end(), reinterpret_cast<char*>(locked_mem));
				GlobalUnlock(hglobal);
			}
			IStream * istream;

			CreateStreamOnHGlobal(hglobal, 1, &istream);
			OleLoadPicture(istream, 0, TRUE, IID_IPicture, (LPVOID*)&(settings->pPic));
			istream->Release();
			settings->pPic->get_Handle((OLE_HANDLE*)&hbitmap);
			// decode the jpeg img data to bitmap and call set img
			SendMessage(GetDlgItem(hwndDlg, IDC_VERCODE_DISPLAY), STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) hbitmap);
			SetTimer(hwndDlg, 2, 1000, 0);
		}
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			// 获取验证码，然后回调
		{
			std::string vcstr;
			vcstr.resize(8);
			vcstr.resize(GetDlgItemTextA(hwndDlg, IDC_INPUT_VERYCODE, &vcstr[0], vcstr.size()));
			settings->donecallback(boost::system::error_code(), vcstr);
			settings->donecallback.clear();
			avloop_gui_del_dlg(*(settings->io_service), hwndDlg);
		}
			return TRUE;
		case IDCANCEL:
			settings->donecallback(boost::asio::error::timed_out, "");
			settings->donecallback.clear();
			avloop_gui_del_dlg(*(settings->io_service), hwndDlg);
			// 退出消息循环
			return TRUE;
		}
		return FALSE;
	case WM_TIMER:
		{
			if (settings->remain_time>=0)
			{
				if (settings->remain_time < 15)
				{
					std::wstring title;
					title = (boost::wformat(L"输入验证码 （剩余 %d 秒）") % settings->remain_time).str();
					// 更新窗口标题！
					SetWindowTextW(hwndDlg, title.c_str());
					if (settings->remain_time == 5)
					{
						FlashWindow(hwndDlg, settings->remain_time & 1);
					}
				}
			}
			else
			{
				KillTimer(hwndDlg, wParam);
				SendMessage(hwndDlg, WM_COMMAND, IDCANCEL, 0);
			}
			settings->remain_time--;
		}
		return TRUE;
	}
	return FALSE;
}

std::function<void()> async_input_box_get_input_with_image(boost::asio::io_service & io_service,
	std::string imagedata, std::function<void(boost::system::error_code, std::string)> donecallback)
{
	HMODULE hIns = GetModuleHandle(NULL);

	input_box_get_input_with_image_settings setting;
	setting.imagedata = imagedata;
	setting.donecallback = donecallback;
	setting.pPic = NULL;
	setting.io_service = &io_service;
	setting.remain_time = 32;

	input_box_get_input_with_image_settings_ptr settings(new input_box_get_input_with_image_settings(setting));

	av_dlgproc_t * real_proc = new av_dlgproc_t(boost::bind(&input_box_get_input_with_image_dlgproc, _1, _2, _3, _4, settings));


	HWND dlgwnd = CreateDialogParam(hIns, MAKEINTRESOURCE(IDD_INPUT_VERCODE), GetConsoleWindow(), (DLGPROC)detail::internal_clusure_dlg_proc, (LPARAM)real_proc);
	RECT	rtWindow = { 0 };
	RECT	rtContainer = { 0 };

	GetWindowRect(dlgwnd, &rtWindow);
	rtWindow.right -= rtWindow.left;
	rtWindow.bottom -= rtWindow.top;

	if (GetParent(dlgwnd))
	{
		GetWindowRect(GetParent(dlgwnd), &rtContainer);
	}
	else
	{
		SystemParametersInfo(SPI_GETWORKAREA, 0, &rtContainer, 0);
	}
	SetWindowPos(dlgwnd, NULL, (rtContainer.right - rtWindow.right) / 2, (rtContainer.bottom - rtWindow.bottom) / 2, 0, 0, SWP_NOSIZE);
	::ShowWindow(dlgwnd, SW_SHOWNORMAL);
	SetForegroundWindow(dlgwnd);
	avloop_gui_add_dlg(io_service, dlgwnd);
	return boost::bind<void>(&DestroyWindow, dlgwnd);
}

namespace detail {

// 消息回调封装，间接调用闭包版
static BOOL CALLBACK internal_clusure_dlg_proc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	BOOL ret = FALSE;

	if (message == WM_INITDIALOG)
	{
		SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
	}
	auto cb_ptr = GetWindowLongPtr(hwndDlg, DWLP_USER);
	av_dlgproc_t * real_callback_ptr = reinterpret_cast<av_dlgproc_t*>(cb_ptr);
	if (cb_ptr)
	{
		ret = (*real_callback_ptr)(hwndDlg, message, wParam, 0);
	}
	if (message == WM_DESTROY)
	{
		SetWindowLongPtr(hwndDlg, DWLP_USER, 0);
		if (real_callback_ptr)
			boost::checked_delete(real_callback_ptr);
	}
	return ret;
}

} // namespace detail

#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
