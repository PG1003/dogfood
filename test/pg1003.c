#include "lua.h"
#include "lauxlib.h"

static int pg1003( lua_State *L )
{
    lua_checkstack( L, 1 );
    lua_pushliteral( L, "PG1003" );

    return 1;
}

static const struct luaL_Reg pg1003_functions[] =
{
    { "pg1003", pg1003 },
    { NULL, NULL }
};

LUALIB_API int luaopen_pg1003( lua_State *L )
{
    luaL_newlib( L,  pg1003_functions );

    return 1;
}
