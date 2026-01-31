#pragma once

#include "WebsocketTypes.h"
#include <luajit/lua.hpp>
#include <atomic>
#include <string>

struct WebsocketClient
{
	WebsocketClient() :
		m_plain_client(),
		m_tls_client()
	{
		s_is_alive.store(true);
		m_plain_client.init_asio();
		m_tls_client.init_asio();
		m_tls_client.set_tls_init_handler(std::bind(&WebsocketClient::on_tls_init, this, ::_1));
	};

	~WebsocketClient()
	{
		s_is_alive.store(false);
	}

	static WebsocketClient* FromUserdata(lua_State* L, const int narg);
	static WebsocketClient* NewUserdata(lua_State* L);

	connection_ptr_variant connect(const std::string& t_uri, websocketpp::lib::error_code& ec, bool& is_tls);
	void on_tick();
	context_ptr on_tls_init(websocketpp::connection_hdl);
	void register_connection(bool is_tls);
	void unregister_connection(bool is_tls);
	static bool IsAlive();

	static bool is_tls_uri(const std::string& t_uri);

	non_tls_client m_plain_client;
	tls_client m_tls_client;
	std::size_t m_plain_connection_count = 0;
	std::size_t m_tls_connection_count = 0;
	static std::atomic<bool> s_is_alive;
};
