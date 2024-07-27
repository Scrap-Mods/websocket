#include "WebsocketClient.h"
#include "WebsocketConnection.h"

#include <string>
#include <vector>
#include <memory>

void lua_checkargs(lua_State* L, const int argn)
{
    const int top = lua_gettop(L);
    if (top != argn)
        luaL_error(L, "Expected %d arguments, got %d", argn, top);
}

std::string lua_stdcheckstring(lua_State* L, const int idx)
{
    std::size_t size = 0;
    const char* str = luaL_checklstring(L, idx, &size);
    return std::string(str, size);
}


// open, message, close, error
int websocket_connection_on(lua_State* L)
{
    STORE_STACK(L);
    lua_checkargs(L, 3);

    auto* ws_conn = reinterpret_cast<WebsocketConnection*>(luaL_checkudata(L, 1, "WebsocketConnection"));
    const char* szCallback = luaL_checkstring(L, 2);
    luaL_checktype(L, 3, LUA_TFUNCTION);

    // Ensure callback table is valid
    lua_rawgeti(L, LUA_REGISTRYINDEX, ws_conn->m_refCallbacks);

    if (lua_type(L, -1) != LUA_TTABLE)
        luaL_error(L, "Invalid callbacks table");

    lua_pushvalue(L, -2);
    lua_setfield(L, -2, szCallback);

    lua_pop(L, 1);

    lua_pushnil(L);
    CHECK_STACK(L, 1);
    return 1;
}

int websocket_connection_send(lua_State* L)
{
    STORE_STACK(L);
    lua_checkargs(L, 2);

    auto* ws_conn = reinterpret_cast<WebsocketConnection*>(luaL_checkudata(L, 1, "WebsocketConnection"));
    std::string msg = lua_stdcheckstring(L, 2);

    ws_conn->m_connection->send(msg, websocketpp::frame::opcode::text);

    lua_pushnil(L);
    CHECK_STACK(L, 1);
    return 1;
}

int websocket_connection_close(lua_State* L)
{
    STORE_STACK(L);
    lua_checkargs(L, 1);

    auto* ws_conn = reinterpret_cast<WebsocketConnection*>(luaL_checkudata(L, 1, "WebsocketConnection"));
    ws_conn->m_connection->pause_reading();

    try
    {
        ws_conn->m_connection->close(websocketpp::close::status::normal, "");
    }
    catch (const std::exception& e) {}

    lua_pushnil(L);
    CHECK_STACK(L, 1);
    return 1;
}


int websocket_connection_gc(lua_State* L)
{
    STORE_STACK(L);
    lua_checkargs(L, 1);

    auto* ws_conn = reinterpret_cast<WebsocketConnection*>(luaL_checkudata(L, 1, "WebsocketConnection"));
    ws_conn->m_connection->pause_reading();
    try
    {
        //ws_conn->m_connection->close(websocketpp::close::status::normal, "");

        ws_conn->m_connection->terminate({});
    }
    catch (const std::exception& e) { }

    luaL_unref(L, LUA_REGISTRYINDEX, ws_conn->m_refCallbacks);

    ws_conn->m_connection->set_close_handler({});
    ws_conn->m_connection->set_fail_handler({});
    ws_conn->~WebsocketConnection();

    CHECK_STACK(L, 0);
    return 0;
}

luaL_Reg websocket_connection__index[] = {
    {"on", websocket_connection_on},
    {"send", websocket_connection_send},
    {"close", websocket_connection_close},
    {NULL, NULL}
};

int websocket_client_tick(lua_State* L)
{
    STORE_STACK(L);
    lua_checkargs(L, 1);

    auto* ws_client = reinterpret_cast<WebsocketClient*>(luaL_checkudata(L, 1, "WebsocketClient"));
    ws_client->on_tick();

    lua_pushnil(L);
    CHECK_STACK(L, 1);
    return 1;
}

int websocket_client_connect(lua_State* L)
{
    STORE_STACK(L);
    lua_checkargs(L, 2);
    
    auto* ws_client = reinterpret_cast<WebsocketClient*>(luaL_checkudata(L, 1, "WebsocketClient"));
    const std::string uri = lua_stdcheckstring(L, 2);

    websocketpp::lib::error_code ec;
    client::connection_ptr c = ws_client->connect(uri, ec);
    if (ec) luaL_error(L, "connection failed: %s", ec.message().c_str());

    auto* pConnection = reinterpret_cast<WebsocketConnection*>(lua_newuserdata(L, sizeof(WebsocketConnection)));

    lua_createtable(L, 0, 0);
    const int ref = luaL_ref(L, LUA_REGISTRYINDEX);

    new (pConnection) WebsocketConnection(L, std::move(c), ref);

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

luaL_Reg library[] = {
    {"tick", websocket_client_tick},
    {"connect", websocket_client_connect},
    {NULL, NULL}
};

int websocket_client_gc(lua_State* L)
{
    auto* ws_client = reinterpret_cast<WebsocketClient*>(luaL_checkudata(L, 1, "WebsocketClient"));
    ws_client->~WebsocketClient();
    
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

    auto* ws_client = reinterpret_cast<WebsocketClient*>(lua_newuserdata(L, sizeof(WebsocketClient)));
    new (ws_client) WebsocketClient();

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