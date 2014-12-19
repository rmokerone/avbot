﻿/**
 * botcmd.cpp 实现 .qqbot 命令控制
 */

#include <string>
#include <algorithm>
#include <vector>
#include <memory>
#include <iostream>

#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/locale.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include <locale.h>
#include <fstream>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <wchar.h>
#if defined(_MSC_VER)
#include <direct.h>
#endif

#include "boost/stringencodings.hpp"

#include "libavlog/avlog.hpp"
#include "libavbot/avbot.hpp"
#include "libavbot/avchannel.hpp"

#include "libwebqq/webqq.hpp"

#include "auto_welcome.hpp"
#include "botctl.hpp"

extern "C" const char * avbot_version();
extern "C" const char * avbot_version_build_time();

extern avlog logfile;			// 用于记录日志文件.

static void iopost_msg(boost::asio::io_service & io_service,
	std::function<void(std::string)> msg_sender,
	std::string msg, std::string groupid)
{
	io_service.post(std::bind(msg_sender, msg));
	logfile.add_log(groupid, msg, 0);
}

static void write_vcode(const std::string & vc_img_data)
{
	std::ofstream vcode("vercode.jpeg", std::ofstream::out);
	vcode.write(vc_img_data.data(), vc_img_data.size());
	vcode.close();
}

static std::function<void (std::string)> do_vc_code;

static void handle_join_group(webqq::qqGroup_ptr group, bool needvc,
	const std::string & vc_img_data, std::shared_ptr<webqq::webqq> qqclient,
	std::function<void(std::string)> msg_sender)
{
	if (needvc && group)
	{
		// 写入验证码.
		write_vcode(vc_img_data);
		std::string msg = boost::str(
			boost::format(
				"哎呀，加入群%s的过程中要输入验证码，请使用 .qqbot vc XXXX 输入。文件为 qqlog 目录下的 vercode.jpeg"
			)
			% group->qqnum
		);

		std::cout << utf8_to_local_encode(msg) <<  std::endl;
		msg_sender(msg);

		webqq::webqq::join_group_handler  join_group_handler;
		join_group_handler  = std::bind(
			handle_join_group, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, qqclient, msg_sender
		);

		do_vc_code = std::bind(
			&webqq::webqq::join_group, qqclient,
			group, std::placeholders::_1, join_group_handler
		);
	}
	else if (group && !needvc)
	{
		msg_sender("哎呦妈呀，群加入了呢～等待对方管理员通过\n记得修改 qqbotrc 将群添加到频道组哦~");
	}
	else
	{
		msg_sender("哎呦妈呀，群加不了!");
	}
}

static void handle_search_group(std::string groupqqnum, webqq::qqGroup_ptr group,
	bool needvc, const std::string & vc_img_data, std::shared_ptr<webqq::webqq> qqclient,
	std::function<void(std::string)> msg_sender)
{
	static webqq::qqGroup_ptr _group;

	if (needvc)
	{
		// 写入验证码.
		write_vcode(vc_img_data);
		// 向大家吵闹输入验证码.
		std::string msg = boost::str(
			boost::format(
				"哎呀，查找群%s的过程中要输入验证码，请使用 .qqbot vc XXXX 输入。文件为 qqlog 目录下的 vercode.jpeg"
			)
			% groupqqnum
		);

		std::cout <<  utf8_to_local_encode(msg) <<  std::endl;
		msg_sender(msg);

		webqq::webqq::search_group_handler search_group_handler;
		search_group_handler = std::bind(
			handle_search_group, groupqqnum, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, qqclient, msg_sender
		);

		do_vc_code = std::bind(
			&webqq::webqq::search_group, qqclient,
			groupqqnum,
			std::placeholders::_1,
			search_group_handler
		);
	}
	else if (group)
	{
		// 很好，加入群吧！
		msg_sender("哈呀，验证码正确了个去的，申请加入ing");
		qqclient->join_group(
			group,
			"",
			std::bind(handle_join_group, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, qqclient, msg_sender)
		);
	}
	else
	{
		msg_sender("没找到没找到!");
	}
}

struct mail_recoder
{
	typedef void result_type;

	std::string channel;
	std::shared_ptr<InternetMailFormat> pimf;
	avbot * mybot;
	std::function<void(std::string)> sendmsg;

	// 由 avbot 的 on_message 调用.
	//channel_identifier, avbotmsg, send_avbot_message_t, boost::asio::yield_context
	void operator()(channel_identifier cid, avbotmsg message, send_avchannel_message_t, boost::asio::yield_context, const boost::signals2::connection & con)
	{
		static boost::regex ex(".qqbot mail subject \"?(.*)\"?");
		boost::cmatch what;

		try
		{
			std::string tmsg = mybot->format_message_for_textIM(message);

			if (tmsg != ".qqbot end mail" && tmsg != ".qqbot mail end")
			{
				if (boost::regex_match(tmsg.c_str(), what, ex))
				{
					pimf->header["subject"] = what[1];
				}
				else
				{
					boost::get<std::string>(pimf->body) += mybot->format_message_for_textIM(message);
				}
			}
			else
			{
				mybot->get_mx()->async_send_mail(*pimf, *this);
				con.disconnect();
			}
		}
		catch (...)
		{
			boost::get<std::string>(pimf->body) += mybot->format_message_for_textIM(message);
		}
	}

	void operator()(const boost::system::error_code & ec)
	{
		if (ec)
			sendmsg("邮件发送失败！以上!");
		else
			sendmsg("邮件发送成功！以上!");
	}
};


//-------------

// 命令控制, 所有的协议都能享受的命令控制在这里实现.
// msg_sender 是一个函数, on_command 用/*它发送消息.
void on_bot_command(channel_identifier cid, avbotmsg avmessage, send_avchannel_message_t sendmsg, boost::asio::yield_context yield_context, avbot& mybot, avchannel& channel)
{
	boost::regex ex;
	boost::smatch what;
	webqq::qqGroup_ptr  group;

	std::string channelname = channel.name();

	auto msg_sender = boost::bind(&avbot::send_broadcast_message, &mybot,
		channelname, _1
	);

	std::string message = mybot.format_message_for_textIM(avmessage);

	if (message == ".qqbot help")
	{
		sendmsg(
			"可用的命令\n"
			"\t.qqbot help\n"
			"\t.qqbot version\n"
			"\t.qqbot ping\n"
			"\t.qqbot joke off 关闭笑话\n"
			"\t.qqbot joke on 开启笑话\n"
			"\t.qqbot joke interval 80 设定笑话间隔到80s\n"
			"\t.qqbot 给大爷讲个笑话  讲个笑话\n"
			"\t.qqbot mail to \"emailaddress\"\n"
			"\t 将命令中间的聊天内容发送到邮件 emailaddress,  注意引号\n"
			"\t 使用 .qqbot mail subject 设置主题\n"
			"\t.qqbot mail end\n"
			"\t.qqbot welcome newbie 欢迎新人\n"
			"== 以下命令需要管理员才能使用==\n"
			"\t.qqbot relogin 强制重新登录qq\n\t.qqbot reload 重新加载群成员列表\n"
			"\t.qqbot begin class XXX\t开课\n\t.qqbot end class 下课\n"
			"以上!", yield_context
		);
	}

	if (message == ".qqbot ping")
	{
		sendmsg("我还活着!", yield_context);
		return;
	}

	if (message == ".qqbot version")
	{
		sendmsg(boost::str(boost::format("我的版本是 %s (%s %s)")
			% avbot_version() % __DATE__ % __TIME__), yield_context
		);
		return;
	}

	ex.set_expression(".qqbot mail to \"?(.*)\"?");

	if (boost::regex_match(message, what, ex) && mybot.get_mx())
	{
		//if (mybot.get_mx())
		// 进入邮件记录模式.
		mail_recoder mrecoder;
		mrecoder.pimf.reset(new InternetMailFormat);
		mrecoder.pimf->header["from"] = mybot.get_mx()->mailaddres();
		mrecoder.pimf->header["to"] = what[1];
		mrecoder.pimf->header["subject"] = "send by avbot";
		mrecoder.pimf->body = std::string("");
		mrecoder.channel = channelname;
		mrecoder.mybot = & mybot;
		mrecoder.sendmsg = msg_sender;

		avchannel::handle_extra_message_type::extended_slot_type mrecoder_slot(mrecoder, _2, _3, _4, _5, _1);

		channel.handle_extra_message.connect_extended(mrecoder_slot);
		return;
	}

	ex.set_expression(".vc (.*)");

	if (boost::regex_match(message, what, ex))
	{
		mybot.feed_login_verify_code(what[1]);
	}

	ex.set_expression(".qqbot vc (.*)");

	if (boost::regex_match(message, what, ex))
	{
		if (do_vc_code)
			do_vc_code(what[1]);
		else
		{
			sendmsg("哈? 输入验证码干嘛?", yield_context);
		}
	}

	// 向新人问候.

	ex.set_expression(".qqbot (newbee|newbie|welcome) (.*)");

	if (boost::regex_match(message, what, ex))
	{
		std::string nick = what[2];

		if (nick.empty())
			return;

		auto_welcome::value_qq_list list;
		list.push_back(nick);

		auto_welcome question(
			channelname + "/welcome.txt"
		);

		question.add_to_list(list);
		question.on_handle_message(msg_sender);
	}

	ex.set_expression("^\\.qqbot qqnickmap +([\\d]+) +([^ ]+)");

	if (boost::regex_search(message, what, ex))
	{
		// map what[1] to what[2];

		std::string uin =  what[1];
		std::string nick = what[2];

		webqq::qqGroup_ptr groupptr ; // TODO 9=  mybot.get_qq()->get_Group_by_qq(channelname);

		if (groupptr)
		{
			webqq::qqBuddy_ptr budyptr = groupptr->get_Buddy_by_uin(uin);

			if (!budyptr)
			{
				groupptr->add_new_buddy(uin, "", nick);
			}
		}

	}

	if (!avmessage.sender.is_op)
		return;

	// 开始讲座记录.
	ex.set_expression(".qqbot begin class ?\"(.*)?\"");

	if (boost::regex_match(message, what, ex))
	{
		std::string title = what[1];

		if (title.empty()) return ;

		if (!logfile.begin_lecture(channelname, title))
		{
			sendmsg("lecture failed!\n", yield_context);
		}

		return;
	}

	// 停止讲座记录.
	if (message == ".qqbot end class")
	{
		logfile.end_lecture();
		return;
	}

	if (message == ".qqbot exit")
	{
		exit(0);
	}

	// 重新加载群成员列表.
	if (message == ".qqbot reload")
	{
// 		group = mybot.get_qq()->get_Group_by_qq(jsonmessage.get<std::string>("channel"));
//
// 		if (group)
// 			mybot.get_io_service().post(
// 				std::bind(&webqq::webqq::update_group_member, mybot.get_qq() , group)
// 			);

		sendmsg("群成员列表重加载.", yield_context);

		return;
	}
}

void set_do_vc(std::function<void(std::string)> f)
{
	do_vc_code = f;
}

void set_do_vc()
{
	do_vc_code = std::function<void(std::string)>();
}
