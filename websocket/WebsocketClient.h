#pragma once
#include <luajit/lua.hpp>

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

class WebsocketClient
{
public:
    client m_client;

    WebsocketClient() :
        m_client()
    {
        m_client.init_asio();

        m_client.set_tls_init_handler(std::bind(&WebsocketClient::on_tls_init, this, ::_1));
        
    };

    ~WebsocketClient() = default;

    client::connection_ptr connect(const std::string& t_uri, websocketpp::lib::error_code& ec);
    void on_tick();
    context_ptr on_tls_init(websocketpp::connection_hdl);
};
