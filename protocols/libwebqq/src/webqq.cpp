/*
 * Copyright (C) 2012 - 2013  微蔡 <microcai@fedoraproject.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <boost/make_shared.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include <avhttp.hpp>
#include <avhttp/async_read_body.hpp>
#include "boost/urlencode.hpp"
#include "boost/timedcall.hpp"

#include "libwebqq/webqq.hpp"
#include "libwebqq/error_code.hpp"

#include "impl/webqq_impl.hpp"

namespace webqq{

webqq::webqq( boost::asio::io_service& asio_service, std::string qqnum, std::string passwd, bool no_persistent_db)
{
	impl = std::make_shared<qqimpl::WebQQ>(boost::ref(asio_service), qqnum, passwd, no_persistent_db);
	impl->start();
}

webqq::~webqq()
{
	impl->stop();
}

void webqq::async_login(webqq::webqq_handler_t handler)
{
	impl->async_login(handler);
}

void webqq::async_login(boost::asio::yield_context handler)
{
	// turn yield_context to handler

	using namespace boost::asio;

	boost::asio::detail::async_result_init<
		boost::asio::yield_context, void(boost::system::error_code)> init((boost::asio::yield_context&&)handler);

	async_login(init.handler);

	return init.result.get();
}

void webqq::on_verify_code( std::function< void ( std::string ) >  cb )
{
	impl->m_signeedvc.connect( cb );
}

void webqq::on_logined(std::function< void() > cb)
{
	impl->siglogined.connect(cb);
}

void webqq::on_group_msg(std::function<void(const std::string, const std::string, const std::vector<qqMsg>&)> cb)
{
	this->impl->siggroupmessage.connect( cb );
}

void webqq::on_group_found(std::function<void(qqGroup_ptr)> cb)
{
	impl->siggroupnumber.connect(cb);
}

void webqq::on_group_newbee(std::function<void(qqGroup_ptr, qqBuddy_ptr)> cb)
{
	impl->signewbuddy.connect(cb);
}

static void dummy(boost::system::error_code){}

void webqq::update_group_member(std::shared_ptr<qqGroup> group )
{
	impl->update_group_member( group , dummy);
}

qqGroup_ptr webqq::get_Group_by_gid( std::string gid )
{
	return impl->get_Group_by_gid( gid );
}

qqGroup_ptr webqq::get_Group_by_qq( std::string qq )
{
	return impl->get_Group_by_qq( qq );
}

void webqq::feed_vc( std::string vccode, std::function<void()> bad_vcreporter)
{
	impl->m_vc_queue.push(vccode);
	impl->m_sigbadvc = bad_vcreporter;
}

void webqq::send_group_message( std::string group, std::string msg, webqq_handler_t donecb )
{
	impl->send_group_message( group, msg, donecb );
}

void webqq::send_group_message( qqGroup& group, std::string msg, webqq_handler_t donecb )
{
	impl->send_group_message( group, msg, donecb );
}

void webqq::send_offline_file(qqBuddy& buddy, std::string filename, webqq_handler_t donecb)
{
	impl->get_ioservice().post(
		std::bind<void>(&qqimpl::WebQQ::send_offline_file, impl, buddy.uin, filename, donecb)
	);
}

void webqq::send_offline_file(std::string uin, std::string filename, webqq::webqq_handler_t donecb)
{
	impl->get_ioservice().post(
		std::bind<void>(&qqimpl::WebQQ::send_offline_file, impl, uin, filename, donecb)
	);
}

void webqq::disable_group_fetch()
{
	impl->m_fetch_groups = false;
}

boost::asio::io_service& webqq::get_ioservice()
{
	return impl->get_ioservice();
}

bool webqq::is_online()
{
	return impl->m_status == LWQQ_STATUS_ONLINE;
}

static void async_fetch_cface_cb(const boost::system::error_code& ec, read_streamptr stream,
	std::function<void(boost::system::error_code ec)> callback)
{
	// store result to cface_data
	callback(ec);
}

void webqq::async_fetch_cface_std_saver( boost::system::error_code ec, boost::asio::streambuf& buf, std::string cface, boost::filesystem::path parent_path)
{
	if (!fs::exists(parent_path))
	{
		fs::create_directories(parent_path);
	}

	if (!ec)
	{
		std::string imgfilename = (parent_path / cface).string();
		std::ofstream cfaceimg(imgfilename.c_str(), std::ofstream::binary|std::ofstream::out);
		cfaceimg.write(boost::asio::buffer_cast<const char*>(buf.data()), boost::asio::buffer_size(buf.data()));
	}
}

void webqq::async_fetch_cface(boost::asio::io_service& io_service, const qqMsgCface& cface, boost::asio::streambuf& buf, std::function<void(boost::system::error_code ec)> callback)
{
	std::string url = boost::str(
						boost::format( "http://web.qq.com/cgi-bin/get_group_pic?gid=%s&uin=%s&fid=%s&pic=%s&vfwebqq=%s" )
						% cface.gid
						% cface.uin
						% cface.file_id
						% avhttp::detail::escape_string(cface.name)
						% cface.vfwebqq
					);

	read_streamptr stream;
	stream.reset(new avhttp::http_stream(io_service));
	stream->request_options(
		avhttp::request_opts()
			(avhttp::http_options::cookie, cface.cookie)
	);
	avhttp::async_read_body(*stream, url, buf, std::bind(async_fetch_cface_cb, std::placeholders::_1, stream, callback));
}

void webqq::async_fetch_cface(boost::asio::io_service& io, const qqMsgCface& cface, boost::asio::streambuf& buf, boost::asio::yield_context yield_context)
{
	using namespace boost::asio;

	boost::asio::detail::async_result_init<boost::asio::yield_context, void(boost::system::error_code)>
		init((boost::asio::yield_context&&)yield_context);

	async_fetch_cface(io, cface, buf, init.handler);

	return init.result.get();
}


static void async_cface_url_final_cb(const boost::system::error_code& ec,
								read_streamptr stream, std::function<void(boost::system::error_code ec, std::string)> callback)
{
	callback(ec, stream->location());
}

void webqq::async_cface_url_final(boost::asio::io_service & io_service, const qqMsgCface & cface, std::function<void(boost::system::error_code ec, std::string)> callback)
{
	std::string url = boost::str(
						boost::format( "http://web.qq.com/cgi-bin/get_group_pic?gid=%s&uin=%s&fid=%s&pic=%s&vfwebqq=%s" )
						% cface.gid
						% cface.uin
						% cface.file_id
						% avhttp::detail::escape_string(cface.name)
						% cface.vfwebqq
					);

	read_streamptr stream;
	stream.reset( new avhttp::http_stream( io_service ) );
	stream->request_options(
		avhttp::request_opts()
			(avhttp::http_options::cookie, cface.cookie)
	);

	stream->max_redirects(0);
	stream->async_open(url, std::bind(async_cface_url_final_cb, std::placeholders::_1, stream, callback));
}

void webqq::search_group( std::string groupqqnum, std::string vfcode, webqq::search_group_handler handler )
{
	impl->search_group(groupqqnum, vfcode, handler);
}

void webqq::join_group( qqGroup_ptr group, std::string vfcode, webqq::join_group_handler handler )
{
	impl->join_group(group, vfcode, handler);
}

std::vector<qqBuddy_ptr> webqq::get_buddies()
{
	return impl->get_buddies();
}


} // namespace webqq
