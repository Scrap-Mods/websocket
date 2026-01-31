#pragma once

#include "WebsocketTypes.h"
#include "WebsocketClient.h"
#include <luajit/lua.hpp>
#include <string>

#define STORE_STACK(L) const int __stack_idx__ = lua_gettop(L)
#define CHECK_STACK(L, args) assert(lua_gettop(L) - args == __stack_idx__ && "Stack corrupted")

struct WebsocketConnection
{
	WebsocketConnection(lua_State* t_state, connection_ptr_variant&& t_connection, WebsocketClient* t_owner, bool t_is_tls, const int t_refCallbacks) :
		m_state(t_state),
		m_connection(std::move(t_connection)),
		m_owner(t_owner),
		m_is_tls(t_is_tls),
		m_refCallbacks(t_refCallbacks)
	{
		if (m_owner)
			m_owner->register_connection(m_is_tls);

		visit_connection([this](auto& connection)
			{
				connection->set_open_handler(std::bind(&WebsocketConnection::on_open, this, ::_1));
				connection->set_fail_handler(std::bind(&WebsocketConnection::on_fail, this, ::_1));
				connection->set_message_handler(std::bind(&WebsocketConnection::on_message, this, ::_1, ::_2));
				connection->set_close_handler(std::bind(&WebsocketConnection::on_close, this, ::_1));
			});
	}

	~WebsocketConnection();

	static WebsocketConnection* FromUserdata(lua_State* L, const int narg);
	static WebsocketConnection* NewUserdata(lua_State* L, connection_ptr_variant&& conn, WebsocketClient* owner, bool is_tls);

	void on_open(websocketpp::connection_hdl hdl);

	void on_fail(websocketpp::connection_hdl hdl);

	void on_message(websocketpp::connection_hdl hdl, message_ptr msg);

	void on_close(websocketpp::connection_hdl hdl);

	void append_header(const std::string& key, const std::string& value);
	void send(const char* message, std::size_t message_len);
	void close();
	void clear_handlers();
	void terminate();
	websocketpp::connection_hdl get_hdl() const;
	websocketpp::lib::error_code get_ec() const;
	int get_ref_callbacks() const;

	template <typename Func>
	decltype(auto) visit_connection(Func&& func)
	{
		return std::visit([&](auto& connection) -> decltype(auto)
			{
				return func(connection);
			}, m_connection);
	}

	template <typename Func>
	decltype(auto) visit_connection(Func&& func) const
	{
		return std::visit([&](const auto& connection) -> decltype(auto)
			{
				return func(connection);
			}, m_connection);
	}


private:
	lua_State* m_state;
	connection_ptr_variant m_connection;
	WebsocketClient* m_owner;
	bool m_is_tls;
	int m_refCallbacks;
};