
#pragma once
#include <boost/asio/io_service.hpp>
#include <boost/signals2.hpp>
#include <boost/atomic.hpp>

#include <avproto.hpp>
#include <avproto/avjackif.hpp>

#include "libavim.hpp"

class avim_impl : public std::enable_shared_from_this<avim_impl>
{
public:
	avim_impl(boost::asio::io_service& io, std::string key, std::string cert);
	~avim_impl();

public:
	void start();

	void start_login();
	// callback when there is a message
	boost::signals2::signal<void(std::string reciver, std::string sender, std::vector<avim_msg>)> on_message;

private:
	void internal_login_coroutine(boost::asio::yield_context);
	void internal_loop_coroutine(boost::asio::yield_context);

private:
	boost::asio::io_service& m_io_service;

	std::shared_ptr<RSA> m_key;
	std::shared_ptr<X509> m_cert;

	avkernel m_core;

	std::shared_ptr<avjackif> m_con;
	std::string m_me_addr;

	boost::atomic<bool> m_quitting;
};
