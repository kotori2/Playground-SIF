
#ifndef LuaLibCRYPTO_h
#define LuaLibCRYPTO_h

#include "ILuaFuncLib.h"

class LuaLibCRYPTO : public ILuaFuncLib
{
private:
	LuaLibCRYPTO();
public:
	LuaLibCRYPTO(DEFCONST* arrConstDef);
	virtual ~LuaLibCRYPTO();

	void addLibrary();

private:
	static int luaDecryptAES128CBC(lua_State * L);
};


#endif // LuaLibCRYPTO_h
