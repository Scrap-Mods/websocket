#include "WebsocketConnection.h"
#include "WebsocketClient.h"

WebsocketConnection* WebsocketConnection::FromUserdata(lua_State* L, const int narg)
{
	return reinterpret_cast<WebsocketConnection*>(luaL_checkudata(L, narg, "WebsocketConnection"));
}

WebsocketConnection* WebsocketConnection::NewUserdata(lua_State* L, connection_ptr_variant&& conn, WebsocketClient* owner, bool is_tls)
{
	auto* pConnection = reinterpret_cast<WebsocketConnection*>(
		lua_newuserdata(L, sizeof(WebsocketConnection)));

	lua_createtable(L, 0, 0);
	const int ref = luaL_ref(L, LUA_REGISTRYINDEX);

	new (pConnection) WebsocketConnection(L, std::move(conn), owner, is_tls, ref);

	return pConnection;
}

WebsocketConnection::~WebsocketConnection()
{
	if (m_owner)
		m_owner->unregister_connection(m_is_tls);
}

void WebsocketConnection::on_open(websocketpp::connection_hdl hdl)
{
	STORE_STACK(m_state);

	lua_rawgeti(m_state, LUA_REGISTRYINDEX, m_refCallbacks);
	if (lua_type(m_state, -1) != LUA_TTABLE)
	{
		lua_pop(m_state, 1);
		CHECK_STACK(m_state, 0);
		return;
	}

	lua_getfield(m_state, -1, "open");

	if (lua_type(m_state, -1) != LUA_TFUNCTION)
	{
		lua_pop(m_state, 2);
		CHECK_STACK(m_state, 0);
		return;
	}

	lua_call(m_state, 0, 0);
	lua_pop(m_state, 1);

	CHECK_STACK(m_state, 0);
}

void WebsocketConnection::on_fail(websocketpp::connection_hdl hdl)
{
	STORE_STACK(m_state);
	lua_rawgeti(m_state, LUA_REGISTRYINDEX, m_refCallbacks);
	if (lua_type(m_state, -1) != LUA_TTABLE)
	{
		lua_pop(m_state, 1);
		CHECK_STACK(m_state, 0);
		return;
	}
	lua_getfield(m_state, -1, "error");

	if (lua_type(m_state, -1) != LUA_TFUNCTION)
	{
		lua_pop(m_state, 2);
		CHECK_STACK(m_state, 0);
		return;
	}
	
	const auto ec = get_ec();
	lua_pushinteger(m_state, ec.value());
	lua_pushstring(m_state, ec.message().c_str());

	lua_call(m_state, 2, 0);
	lua_pop(m_state, 1);
	CHECK_STACK(m_state, 0);
}

void WebsocketConnection::on_message(websocketpp::connection_hdl hdl, message_ptr msg)
{
	STORE_STACK(m_state);
	lua_rawgeti(m_state, LUA_REGISTRYINDEX, m_refCallbacks);
	if (lua_type(m_state, -1) != LUA_TTABLE)
	{
		lua_pop(m_state, 1);
		CHECK_STACK(m_state, 0);
		return;
	}
	lua_getfield(m_state, -1, "message");

	if (lua_type(m_state, -1) != LUA_TFUNCTION)
	{
		lua_pop(m_state, 2);
		CHECK_STACK(m_state, 0);
		return;
	}

	lua_pushstring(m_state, msg->get_payload().c_str());

	lua_call(m_state, 1, 0);
	lua_pop(m_state, 1);
	CHECK_STACK(m_state, 0);
}

void WebsocketConnection::on_close(websocketpp::connection_hdl hdl)
{
	STORE_STACK(m_state);
	lua_rawgeti(m_state, LUA_REGISTRYINDEX, m_refCallbacks);
	if (lua_type(m_state, -1) != LUA_TTABLE)
	{
		lua_pop(m_state, 1);
		CHECK_STACK(m_state, 0);
		return;
	}
	lua_getfield(m_state, -1, "close");

	if (lua_type(m_state, -1) != LUA_TFUNCTION)
	{
		lua_pop(m_state, 2);
		CHECK_STACK(m_state, 0);
		return;
	}

	const auto ec = get_ec();
	lua_pushinteger(m_state, ec.value());
	lua_pushstring(m_state, ec.message().c_str());

	lua_call(m_state, 2, 0);
	lua_pop(m_state, 1);
	CHECK_STACK(m_state, 0);
}

void WebsocketConnection::append_header(const std::string& key, const std::string& value)
{
	visit_connection([&](auto& connection)
		{
			connection->append_header(key, value);
		});
}

void WebsocketConnection::send(const char* message, std::size_t message_len)
{
	visit_connection([&](auto& connection)
		{
			connection->send(message, message_len, websocketpp::frame::opcode::text);
		});
}

void WebsocketConnection::close()
{
	visit_connection([&](auto& connection)
		{
			connection->close(websocketpp::close::status::normal, "");
		});
}

void WebsocketConnection::clear_close_handler()
{
	visit_connection([&](auto& connection)
		{
			connection->set_close_handler({});
		});
}

void WebsocketConnection::terminate()
{
	visit_connection([&](auto& connection)
		{
			connection->terminate({});
		});
}

websocketpp::connection_hdl WebsocketConnection::get_hdl() const
{
	return visit_connection([](const auto& connection)
		{
			return websocketpp::connection_hdl(connection->weak_from_this());
		});
}

websocketpp::lib::error_code WebsocketConnection::get_ec() const
{
	return visit_connection([](const auto& connection)
		{
			return connection->get_ec();
		});
}

int WebsocketConnection::get_ref_callbacks() const
{
	return m_refCallbacks;
}
