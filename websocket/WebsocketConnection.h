#pragma once

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <luajit/lua.hpp>

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

#define STORE_STACK(L) const int __stack_idx__ = lua_gettop(L)
#define CHECK_STACK(L, args) assert(lua_gettop(L) - args == __stack_idx__ && "Stack corrupted")

struct WebsocketConnection
{
	WebsocketConnection(lua_State* t_state, client::connection_ptr&& t_connection, const int t_refCallbacks) :
		m_state(t_state),
		m_connection(std::move(t_connection)),
		m_refCallbacks(t_refCallbacks)
	{
		m_connection->set_open_handler(std::bind(&WebsocketConnection::on_open, this, ::_1));
		m_connection->set_fail_handler(std::bind(&WebsocketConnection::on_fail, this, ::_1));
		m_connection->set_message_handler(std::bind(&WebsocketConnection::on_message, this, ::_1, ::_2));
		m_connection->set_close_handler(std::bind(&WebsocketConnection::on_close, this, ::_1));
	}

	~WebsocketConnection() = default;

	static WebsocketConnection* FromUserdata(lua_State* L, const int narg);
	static WebsocketConnection* NewUserdata(lua_State* L, client::connection_ptr&& conn);

	void on_open(websocketpp::connection_hdl hdl);

	void on_fail(websocketpp::connection_hdl hdl);

	void on_message(websocketpp::connection_hdl hdl, message_ptr msg);

	void on_close(websocketpp::connection_hdl hdl);

private:
	lua_State* m_state;
public:
	client::connection_ptr m_connection;
	int m_refCallbacks;
};