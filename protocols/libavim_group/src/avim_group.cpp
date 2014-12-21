
#include "avim_group.hpp"
#include "avim_group_impl.hpp"

avim::avim(boost::asio::io_service& io, std::string key, std::string cert, std::string groupdeffile)
	: m_io_service(io)
{
	m_impl.reset(new avim_group_impl(io, key, cert, groupdeffile));
	m_impl->start();
}

avim::~avim()
{
	m_impl->m_quitting = true;
}

void avim::on_message(boost::function<void(std::string reciver, std::string sender, std::vector<avim_msg>)> cb)
{
	m_impl->on_message.connect(cb);
}
