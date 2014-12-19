
#include "avchannel.hpp"

avchannel::avchannel(std::string name)
	: m_name(std::move(name))
{
}

bool avchannel::can_handle(channel_identifier channel_id) const
{
    // 看是否属于自己频道
    auto found = std::find(std::begin(m_rooms), std::end(m_rooms), channel_id);
    return found != std::end(m_rooms);
}

void avchannel::handle_message(channel_identifier channel_id, avbotmsg msg, send_avbot_message_t send_avbot_message, boost::asio::yield_context yield_context)
{
    // 开始处理本频道消息
    for (auto room : m_rooms)
    {
        // 避免重复发送给自己
        if (room == channel_id)
            continue;

        send_avbot_message(room, msg, yield_context);
    }

    auto send_avchannel_message = [this, send_avbot_message](avbotmsg msg, boost::asio::yield_context yield_context)
	{
		broadcast_message(msg, send_avbot_message, yield_context);
	};

    handle_extra_message(channel_id, msg, send_avchannel_message, yield_context);
}

void avchannel::broadcast_message(avbotmsg msg, send_avbot_message_t send_avbot_message, boost::asio::yield_context yield_context)
{
    // 开始处理消息
    for (auto room : m_rooms)
    {
        send_avbot_message(room, msg, yield_context);
    }
}
