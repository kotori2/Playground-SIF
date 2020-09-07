
#ifndef LuaLibMISC_h
#define LuaLibMISC_h

#include "ILuaFuncLib.h"

class LuaLibMISC : public ILuaFuncLib
{
public:
	static bool m_hacked;
private:
	LuaLibMISC();
public:
	LuaLibMISC(DEFCONST* arrConstDef);
	virtual ~LuaLibMISC();

	void addLibrary();

private:
	static int luaDebugHack(lua_State* L);
};


#endif // LuaLibMISC_h
