#pragma once
#include <boost/asio/io_service.hpp>
#include <boost/function.hpp>

class avim_impl;

struct avim_msg{
	std::string text;
	std::string image;
	std::string url;
};

class avim
{
public:
	avim(boost::asio::io_service& io, std::string key, std::string cert);
	~avim();

	void on_message(boost::function<void(std::string reciver, std::string sender, std::vector<avim_msg>)>);

private:
	boost::asio::io_service& m_io_service;
	std::shared_ptr<avim_impl> m_impl;
};
