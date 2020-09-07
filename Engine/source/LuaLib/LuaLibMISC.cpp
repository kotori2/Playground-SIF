
#include "LuaLibMISC.h"
#include "CKLBUtility.h"
#include "CKLBLuaTask.h"
#include "CKLBLuaLibENG.h"

static LuaLibMISC libdef(0);
bool LuaLibMISC::m_hacked = false;

LuaLibMISC::LuaLibMISC(DEFCONST* arrConstDef) : ILuaFuncLib(arrConstDef) {}
LuaLibMISC::~LuaLibMISC() {}

void
LuaLibMISC::addLibrary()
{
	addFunction("MISC_DebugHack", LuaLibMISC::luaDebugHack);
}

/* hack for DebugModel, equals to:

DebugModel = {
	initialize = function()
		return {
			isReady = function()
				return false
			end
		}
	end
}
*/
int LuaLibMISC::luaDebugHack(lua_State* L)
{
	if (m_hacked) { return 0; }
	m_hacked = true;
	if (CKLBLuaLibENG::isRelease()) { return 0; }
	lua_getglobal(L, "GLOBAL");
	lua_pushliteral(L, "DebugModel");
	lua_newtable(L);
	{
		lua_pushliteral(L, "LiveDeck");
		{
			lua_newtable(L);
			{
				lua_pushliteral(L, "initialize");
				lua_pushcfunction(L, [](lua_State* L1) {
					lua_newtable(L1);
					lua_pushliteral(L1, "isReady");
					lua_pushcfunction(L1, [](lua_State* L2) {
						lua_pushboolean(L2, false);
						return 1;
						});
					lua_settable(L1, -3);
					return 1;
				});
				lua_settable(L, -3);
			}
		}
		lua_settable(L, -3);
	}
	lua_settable(L, -3);
	return 0;
}
