
#include <boost/function.hpp>
#include <boost/asio/io_service.hpp>

#ifdef _WIN32
#include <windows.h>
#endif

/*
 * 创建一个对话框并以 imagedata 指示的数据显示验证码
 * 返回一个用来撤销他的闭包。
 */
std::function<void()> async_input_box_get_input_with_image(boost::asio::io_service & io_service, std::string imagedata, std::function<void(boost::system::error_code, std::string)> donecallback);

/*
 * 开启一个模态对话框，然后让用户配置
 */
void setup_dialog(std::string & qqnumber, std::string & qqpwd,
	std::string & ircnick, std::string & ircroom, std::string & ircpwd,
		std::string &xmppuser, std::string & xmppserver,std::string & xmpppwd, std::string & xmpproom, std::string &xmppnick);
