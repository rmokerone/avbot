
#include <iostream>
#include "avim_group_impl.hpp"
#include <avproto/easyssl.hpp>

avim_group_impl::avim_group_impl(boost::asio::io_service& io, std::string key, std::string cert)
	: m_io_service(io)
	, m_core(io)
{
	m_quitting = false;

	m_key = load_RSA_from_file(key);
	m_cert = load_X509_from_file(cert);
}

avim_group_impl::~avim_group_impl()
{
	m_quitting = true;
}

void avim_group_impl::start()
{
	boost::asio::spawn(m_io_service, std::bind(&avim_group_impl::internal_loop_coroutine, shared_from_this(), std::placeholders::_1));

	start_login();
}

void avim_group_impl::start_login()
{
	if (!m_quitting)
		boost::asio::spawn(m_io_service, std::bind(&avim_group_impl::internal_login_coroutine, shared_from_this(), std::placeholders::_1));
}

void avim_group_impl::internal_login_coroutine(boost::asio::yield_context yield_context)
{
	m_con.reset(new avjackif(m_io_service));
	m_con->set_pki(m_key, m_cert);

	try
	{
		if (!m_con->async_connect("avim.avplayer.org", "24950", yield_context))
			return start_login();

		if (!m_con->async_handshake(yield_context))
		{
			// stop loop
			std::cerr << "avim login failed!" << std::endl;
			return;
		}

		m_con->signal_notify_remove.connect([this]()
		{
			start_login();
		});

		m_core.add_interface(m_con);

		m_me_addr = av_address_to_string(*m_con->if_address());

		// 添加路由表, metric越大，优先级越低
		m_core.add_route(".+@.+", m_me_addr, m_con->get_ifname(), 100);

	}catch(const std::exception&)
	{
		start_login();
	}
}

void avim_group_impl::internal_loop_coroutine(boost::asio::yield_context yield_context)
{

	for(;!m_quitting;)
	{
		std::string sender, data;

		m_core.async_recvfrom(sender, data, yield_context);

		// 解码 group 消息

		if (!is_control_message(data))
		{
			if (!is_encrypted_message(data))
			{
				auto im =  decode_im_message(data);

				std::vector<avim_msg> avmsg;

				for (const message::avim_message& im_item : im.impkt.avim())
				{
					avim_msg item;
 					if (im_item.has_item_text())
					{
						item.text = im_item.item_text().text();
					}
					if (im_item.has_item_image())
					{
						item.image = im_item.item_image().image();
					}
					avmsg.push_back(item);
				}
				on_message(m_me_addr, im.sender, avmsg);
			}
		}
	}
}
