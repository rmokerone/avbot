
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

#include <OleCtl.h>
#include <Ole2.h>
#include <OCIdl.h>

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


#include "fsconfig.ipp"

struct avbot_dlg_settings
{
	std::string& qqnumber;
	std::string& qqpwd;

	std::string& ircnick;
	std::string& ircpwd;
	std::string& ircroom;

	std::string& xmppuser;
	std::string& xmpppwd;
	std::string& xmpproom;
};

static void InitDlgSettings(HWND hwndDlg, avbot_dlg_settings & in)
{
	// 设定 窗口的文字
	SetDlgItemTextA(hwndDlg, IDC_EDIT_USER_NAME, in.qqnumber.c_str());
	SetDlgItemTextA(hwndDlg, IDC_EDIT_PWD, in.qqpwd.c_str());

	if (!in.ircnick.empty())
	{
		CheckDlgButton(hwndDlg, IDC_CHECK_IRC, 1);
		// 设定文字
		SetDlgItemTextA(hwndDlg, IDC_EDIT_IRC_CHANNEL, in.ircroom.c_str());
		SetDlgItemTextA(hwndDlg, IDC_EDIT_IRC_NICK, in.ircnick.c_str());
		SetDlgItemTextA(hwndDlg, IDC_EDIT_IRC_PWD, in.ircpwd.c_str());
	}
	else
	{
		CheckDlgButton(hwndDlg, IDC_CHECK_IRC, 0);
	}

	if (!in.xmppuser.empty())
	{
		CheckDlgButton(hwndDlg, IDC_CHECK_XMPP, 1);
		// 设定文字
		SetDlgItemTextA(hwndDlg, IDC_EDIT_XMPP_CHANNEL, in.xmpproom.c_str());
		SetDlgItemTextA(hwndDlg, IDC_EDIT_XMPP_NICK, in.xmppuser.c_str());
		SetDlgItemTextA(hwndDlg, IDC_EDIT_XMPP_PWD, in.xmpppwd.c_str());
	}
	else
	{
		CheckDlgButton(hwndDlg, IDC_CHECK_XMPP, 0);
	}

	if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_XMPP) != BST_CHECKED) {
		EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_XMPP_CHANNEL), FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_XMPP_NICK), FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_XMPP_PWD), FALSE);
	}
	if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_IRC) != BST_CHECKED) {
		EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_IRC_CHANNEL), FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_IRC_NICK), FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_IRC_PWD), FALSE);
	}
}

static void ExtractDlgSettings(HWND hDlg, avbot_dlg_settings & out)
{
	// 看起来avbot用的是多字节,std::string not std::wstring
	TCHAR temp[MAX_PATH];
	bool use_xmpp = false;
	bool use_irc = false;
	// qq setting
	GetDlgItemText(hDlg, IDC_EDIT_USER_NAME, temp, MAX_PATH);
	out.qqnumber = temp;

	GetDlgItemText(hDlg, IDC_EDIT_PWD, temp, MAX_PATH);
	out.qqpwd = temp;

	// irc setting
	if (IsDlgButtonChecked(hDlg, IDC_CHECK_IRC) == BST_CHECKED)
	{
		GetDlgItemText(hDlg, IDC_EDIT_IRC_CHANNEL, temp, MAX_PATH);
		out.ircroom = temp;

		GetDlgItemText(hDlg, IDC_EDIT_IRC_NICK, temp, MAX_PATH);
		out.ircnick = temp;

		GetDlgItemText(hDlg, IDC_EDIT_IRC_PWD, temp, MAX_PATH);
		out.ircpwd = temp;
		use_irc = true;
	}

	// xmpp setting
	if (IsDlgButtonChecked(hDlg, IDC_CHECK_XMPP) == BST_CHECKED)
	{
		GetDlgItemText(hDlg, IDC_EDIT_XMPP_CHANNEL, temp, MAX_PATH);
		out.xmpproom = temp;

		GetDlgItemText(hDlg, IDC_EDIT_XMPP_NICK, temp, MAX_PATH);
		out.xmppuser = temp;

		GetDlgItemText(hDlg, IDC_EDIT_XMPP_PWD, temp, MAX_PATH);
		out.xmpppwd = temp;

		use_xmpp = true;
	}

	// save data to config file
	try {
		fs::path config_file;
		if (exist_config_file()) {
			config_file = configfilepath();
		}
		else {
			std::cerr << "config file not exist." << std::endl;
			TCHAR file_path[MAX_PATH];
			GetModuleFileName(NULL, file_path, MAX_PATH);
			config_file = fs::path(file_path).parent_path() / "qqbotrc";
		}

		std::ofstream file(config_file.string().c_str());

		file << "# qq config" << std::endl;
		file << "qqnum=" << out.qqnumber << std::endl;
		file << "qqpwd=" << out.qqpwd << std::endl;
		file << std::endl;

		if (use_irc)
		{
			file << "# irc config" << std::endl;
			file << "ircnick=" << out.ircnick << std::endl;
			file << "ircpwd=" << out.ircpwd << std::endl;
			file << "ircrooms=" << out.ircroom << std::endl;
			file << std::endl;
		}

		if (use_xmpp)
		{
			file << "# xmpp config" << std::endl;
			file << "xmppuser=" << out.xmppuser << std::endl;
			file << "xmpppwd=" << out.xmpppwd << std::endl;
			file << "xmpprooms=" << out.xmpproom << std::endl;
			file << std::endl;
		}
	}
	catch (...) {
		MessageBox(hDlg, TEXT("error while handle config file"), TEXT("avbot faital error"), MB_OK);
		exit(1);
	}
}

// 这里是真正的消息回调函数，支持闭包！
static bool dlgproc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam, avbot_dlg_settings & settings)
{
	BOOL fError;

	switch (message)
	{
	case WM_INITDIALOG:
		InitDlgSettings(hwndDlg, settings);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			// 通知主函数写入数据并重启
		{
			ExtractDlgSettings(hwndDlg, settings);
			EndDialog(hwndDlg, (INT_PTR)&settings);
		}
			return TRUE;
		case IDCANCEL:
			EndDialog(hwndDlg, 0);
			// 退出消息循环
			return TRUE;
		case IDC_CHECK_XMPP:{
			BOOL enable = FALSE;
			if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_XMPP) == BST_CHECKED) enable = TRUE;

			EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_XMPP_CHANNEL), enable);
			EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_XMPP_NICK), enable);
			EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_XMPP_PWD), enable);
		}return TRUE;
		case IDC_CHECK_IRC:{
			BOOL enable = FALSE;
			if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_IRC) == BST_CHECKED) enable = TRUE;

			EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_IRC_CHANNEL), enable);
			EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_IRC_NICK), enable);
			EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_IRC_PWD), enable);
		}return TRUE;
		}
	}

	return FALSE;
}

typedef boost::function<bool(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)> av_dlgproc_t;

namespace detail {
static BOOL CALLBACK internal_clusure_dlg_proc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
}

void setup_dialog(std::string & qqnumber, std::string & qqpwd, std::string & ircnick,
					std::string & ircroom, std::string & ircpwd,
					std::string &xmppuser, std::string & xmppserver,std::string & xmpppwd, std::string & xmpproom, std::string &xmppnick)
{
	// windows下面弹出选项设置框框.
	HMODULE hIns = GetModuleHandle(NULL);


	// 开启个模态对话框
	avbot_dlg_settings out = { qqnumber, qqpwd, ircnick, ircpwd, ircroom, xmppuser, xmpppwd, xmpproom };
	av_dlgproc_t * real_proc = new av_dlgproc_t(boost::bind(&dlgproc, _1, _2, _3, _4, boost::ref(out)));

	INT_PTR ret = DialogBoxParam(hIns, MAKEINTRESOURCE(IDD_AVSETTINGS_DIALOG), NULL, (DLGPROC)detail::internal_clusure_dlg_proc, (LPARAM)real_proc);

	// 处理返回值，消息循环里应该调用 	EndDialog() 退出模块对话框
	if (!ret)
	{
		// 退出 avbot
		exit(0);
	}
	assert(ret == reinterpret_cast<INT_PTR>(&out));
	// 返回值其实是一个指针，指向一块牛逼的内存。存放对话框记录的东西
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
