#include "WebsocketClient.h"
#include "WebsocketConnection.h"

#include <string>
#include <vector>
#include <memory>
#include <format>

static void lua_checkargs(lua_State* L, const int argn)
{
	const int top = lua_gettop(L);
	if (top != argn)
		luaL_error(L, "Expected %d arguments, got %d", argn, top);
}

static void lua_checkargs(lua_State* L, const int argn, const int argm)
{
	const int top = lua_gettop(L);
	if (top < argn || top > argm)
		luaL_error(L, "Expected %d to %d arguments, got %d", argn, argm, top);
}

static std::string lua_stdcheckstring(lua_State* L, const int idx)
{
	std::size_t size = 0;
	const char* str = luaL_checklstring(L, idx, &size);
	return std::string(str, size);
}

static std::string lua_tostdstring(lua_State* L, const int idx)
{
	std::size_t size = 0;
	const char* str = lua_tolstring(L, idx, &size);
	return std::string(str, size);
}

// open, message, close, error
static int websocket_connection_on(lua_State* L)
{
	STORE_STACK(L);
	lua_checkargs(L, 3);

	auto* ws_conn = WebsocketConnection::FromUserdata(L, 1);
	const char* szCallback = luaL_checkstring(L, 2);
	luaL_checktype(L, 3, LUA_TFUNCTION);

	// Ensure callback table is valid
	lua_rawgeti(L, LUA_REGISTRYINDEX, ws_conn->get_ref_callbacks());

	if (lua_type(L, -1) != LUA_TTABLE)
		luaL_error(L, "Invalid callbacks table");

	lua_pushvalue(L, -2);
	lua_setfield(L, -2, szCallback);

	lua_pop(L, 1);

	lua_pushnil(L);
	CHECK_STACK(L, 1);
	return 1;
}

static int websocket_connection_send(lua_State* L)
{
	STORE_STACK(L);
	lua_checkargs(L, 2);

	auto* ws_conn = WebsocketConnection::FromUserdata(L, 1);

	std::size_t msg_len;
	const char* msg_ptr = luaL_checklstring(L, 2, &msg_len);

	ws_conn->send(msg_ptr, msg_len);

	lua_pushnil(L);
	CHECK_STACK(L, 1);
	return 1;
}

static int websocket_connection_close(lua_State* L)
{
	STORE_STACK(L);
	lua_checkargs(L, 1);

	auto* ws_conn = WebsocketConnection::FromUserdata(L, 1);

	try
	{
		ws_conn->close();
	}
	catch (...) {}

	lua_pushnil(L);
	CHECK_STACK(L, 1);
	return 1;
}

static int websocket_connection_gc(lua_State* L)
{
	STORE_STACK(L);
	lua_checkargs(L, 1);

	auto* ws_conn = WebsocketConnection::FromUserdata(L, 1);
	ws_conn->clear_close_handler();

	try
	{
		ws_conn->on_close(ws_conn->get_hdl());
		ws_conn->terminate();
	}
	catch (...) {}

	luaL_unref(L, LUA_REGISTRYINDEX, ws_conn->get_ref_callbacks());

	ws_conn->~WebsocketConnection();

	CHECK_STACK(L, 0);
	return 0;
}

inline const static luaL_Reg websocket_connection__index[] =
{
	{ "on"   , websocket_connection_on    },
	{ "send" , websocket_connection_send  },
	{ "close", websocket_connection_close },

	{ NULL, NULL }
};

static int websocket_client_tick(lua_State* L)
{
	STORE_STACK(L);
	lua_checkargs(L, 1);

	WebsocketClient::FromUserdata(L, 1)->on_tick();

	lua_pushnil(L);
	CHECK_STACK(L, 1);
	return 1;
}

static std::vector<std::pair<std::string, std::string>> websocket_parse_table(lua_State* L, const int idx)
{
	luaL_checktype(L, idx, LUA_TTABLE);

	std::vector<std::pair<std::string, std::string>> table;

	// If invalid key/value is encountered, exit
	lua_pushnil(L);

	while (lua_next(L, idx) != 0)
	{
		const int typeKey = lua_type(L, -2);
		if (typeKey != LUA_TSTRING)
			luaL_error(L, "Invalid key type, expected 'string' got: '%s'", lua_typename(L, typeKey));

		const int typeValue = lua_type(L, -1);
		if (typeValue != LUA_TSTRING)
			luaL_error(L, "Invalid value type, expected 'string' got: '%s'", lua_typename(L, typeValue));

		table.emplace_back(lua_tostdstring(L, -2), lua_tostdstring(L, -1));

		lua_pop(L, 1);
	}

	return table;
}

static std::vector<std::string> websocket_parse_cookies(lua_State* L, const int idx)
{
	luaL_checktype(L, idx, LUA_TTABLE);

	std::vector<std::string> table;

	// If invalid key/value is encountered, exit
	lua_pushnil(L);

	while (lua_next(L, idx) != 0)
	{
		const int typeKey = lua_type(L, -2);
		if (typeKey != LUA_TSTRING)
			luaL_error(L, "Invalid key type, expected 'string' got: '%s'", lua_typename(L, typeKey));

		const int typeValue = lua_type(L, -1);
		if (typeValue != LUA_TSTRING)
			luaL_error(L, "Invalid value type, expected 'string' got: '%s'", lua_typename(L, typeValue));

		table.emplace_back(std::format("{}={}", lua_tostdstring(L, -2), lua_tostdstring(L, -1)));

		lua_pop(L, 1);
	}

	return table;
}

static int websocket_client_connect(lua_State* L)
{
	STORE_STACK(L);
	lua_checkargs(L, 2, 4);
	
	auto* ws_client = WebsocketClient::FromUserdata(L, 1);
	const std::string uri = lua_stdcheckstring(L, 2);
	
	std::vector<std::pair<std::string, std::string>> headers;
	if (__stack_idx__ >= 3) headers = std::move(websocket_parse_table(L, 3));

	std::vector<std::string> cookies;
	if (__stack_idx__ >= 4) cookies = std::move(websocket_parse_cookies(L, 4));

	std::string cookies_str;
	if (cookies.size())
	{
		for (int i = 0; i < cookies.size(); i++)
		{
			cookies_str.append(cookies[i]);
			cookies_str.append("; ");
		}
		cookies_str.resize(cookies_str.size() - 2);
	}

	websocketpp::lib::error_code ec;
	bool is_tls = false;
	connection_ptr_variant c = ws_client->connect(uri, ec, is_tls);
	if (ec) luaL_error(L, "connection failed: %s", ec.message().c_str());

	auto* pConnection = WebsocketConnection::NewUserdata(L, std::move(c), ws_client, is_tls);
	for (const auto& [k, v] : headers)
		pConnection->append_header(k, v);

	pConnection->append_header("Cookie", cookies_str);

	if (luaL_newmetatable(L, "WebsocketConnection"))
	{
		lua_createtable(L, 0, 0);

		luaL_register(L, NULL, websocket_connection__index);
		lua_setfield(L, -2, "__index");

		lua_pushcclosure(L, websocket_connection_gc, 0);
		lua_setfield(L, -2, "__gc");
	}

	lua_setmetatable(L, -2);

	CHECK_STACK(L, 1);
	return 1;
}

inline const static luaL_Reg library[] =
{
	{ "tick"   , websocket_client_tick    },
	{ "connect", websocket_client_connect },

	{ NULL, NULL}
};

static int websocket_client_gc(lua_State* L)
{
	WebsocketClient::FromUserdata(L, 1)->~WebsocketClient();
	return 0;
}

extern "C" __declspec(dllexport) int luaopen_websocket(lua_State* L)
{
	STORE_STACK(L);

	// Check if WebsocketClient is already in the registry for the current environment
	lua_pushliteral(L, "WebsocketClientInstance");
	lua_rawget(L, LUA_REGISTRYINDEX);

	// Return existing
	if (lua_type(L, -1) == LUA_TUSERDATA)
	{
		CHECK_STACK(L, 1);
		return 1;
	}

	lua_pop(L, 1);

	WebsocketClient::NewUserdata(L);

	if (luaL_newmetatable(L, "WebsocketClient"))
	{
		lua_createtable(L, 0, 0);

		luaL_register(L, NULL, library);
		lua_setfield(L, -2, "__index");

		lua_pushcclosure(L, websocket_client_gc, 0);
		lua_setfield(L, -2, "__gc");
	}

	lua_setmetatable(L, -2);

	// Save onto registry
	lua_pushliteral(L, "WebsocketClientInstance");
	lua_pushvalue(L, -2);
	lua_rawset(L, LUA_REGISTRYINDEX);

	CHECK_STACK(L, 1);
	return 1;
}