#include "WebsocketClient.h"

#include <algorithm>

std::atomic<bool> WebsocketClient::s_is_alive{ false };

WebsocketClient* WebsocketClient::FromUserdata(lua_State* L, const int narg)
{
	return reinterpret_cast<WebsocketClient*>(luaL_checkudata(L, narg, "WebsocketClient"));
}

WebsocketClient* WebsocketClient::NewUserdata(lua_State* L)
{
	auto* pClient = reinterpret_cast<WebsocketClient*>(lua_newuserdata(L, sizeof(WebsocketClient)));
	new (pClient) WebsocketClient();

	return pClient;
}

connection_ptr_variant WebsocketClient::connect(const std::string& t_uri, websocketpp::lib::error_code& ec, bool& is_tls)
{
	is_tls = is_tls_uri(t_uri);
	if (is_tls)
	{
		tls_client::connection_ptr connection = m_tls_client.get_connection(t_uri, ec);
		if (ec) return connection_ptr_variant{};

		m_tls_client.connect(connection);
		return connection;
	}

	non_tls_client::connection_ptr connection = m_plain_client.get_connection(t_uri, ec);
	if (ec) return connection_ptr_variant{};

	m_plain_client.connect(connection);

	return connection;
}

void WebsocketClient::on_tick()
{
	if (m_plain_connection_count > 0)
	{
		m_plain_client.poll();
		m_plain_client.reset();
	}

	if (m_tls_connection_count > 0)
	{
		m_tls_client.poll();
		m_tls_client.reset();
	}
}

context_ptr WebsocketClient::on_tls_init(websocketpp::connection_hdl)
{
	context_ptr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);

	try
	{
		ctx->set_options(
			boost::asio::ssl::context::default_workarounds |
			boost::asio::ssl::context::no_sslv2 |
			boost::asio::ssl::context::no_sslv3 |
			boost::asio::ssl::context::single_dh_use);

		ctx->set_verify_mode(boost::asio::ssl::verify_none);
	}
	catch (std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

	return ctx;
}

bool WebsocketClient::is_tls_uri(const std::string& t_uri)
{
	static const std::string prefix = "wss://";
	if (t_uri.size() < prefix.size())
		return false;

	return std::equal(prefix.begin(), prefix.end(), t_uri.begin());
}

void WebsocketClient::register_connection(bool is_tls)
{
	if (is_tls)
		++m_tls_connection_count;
	else
		++m_plain_connection_count;
}

void WebsocketClient::unregister_connection(bool is_tls)
{
	if (is_tls)
	{
		if (m_tls_connection_count > 0)
			--m_tls_connection_count;
		return;
	}

	if (m_plain_connection_count > 0)
		--m_plain_connection_count;
}

bool WebsocketClient::IsAlive()
{
	return s_is_alive.load();
}
