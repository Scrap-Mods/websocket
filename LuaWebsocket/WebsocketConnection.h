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

#define STORE_STACK(L) const int __stack_idx__ = lua_gettop(L)
#define CHECK_STACK(L, args) assert(lua_gettop(L) - args == __stack_idx__, "Stack corrupted")

class WebsocketConnection
{
    lua_State* m_state;
public:
    client::connection_ptr m_connection;
    int m_refCallbacks;

    WebsocketConnection(lua_State* t_state, client::connection_ptr&& t_connection, int t_refCallbacks) :
        m_state(t_state),
        m_connection(t_connection),
        m_refCallbacks(t_refCallbacks)
    {
        m_connection->set_open_handler(std::bind(&WebsocketConnection::on_open, this, ::_1));
        m_connection->set_fail_handler(std::bind(&WebsocketConnection::on_fail, this, ::_1));
        m_connection->set_message_handler(std::bind(&WebsocketConnection::on_message, this, ::_1, ::_2));
        m_connection->set_close_handler(std::bind(&WebsocketConnection::on_close, this, ::_1));
        m_connection->set_close_handshake_timeout(1);
    }

    ~WebsocketConnection() = default;

    void on_open(websocketpp::connection_hdl hdl);

    void on_fail(websocketpp::connection_hdl hdl);

    void on_message(websocketpp::connection_hdl hdl, message_ptr msg);

    void on_close(websocketpp::connection_hdl hdl);
};

