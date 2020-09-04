#ifndef LUAEDIT_REMOTEDEBUGGER_HPP
#define LUAEDIT_REMOTEDEBUGGER_HPP

// Forward declarations
struct lua_State;

// LuaEdit API
void StartLuaEditRemoteDebugger(int aPort = 32201, lua_State* L = NULL);
void StopLuaEditRemoteDebugger();
const char* GetCurrentLuaVersion();
bool CheckLuaScriptSyntax(const char* aScript, const char* aScriptName, char* aErrBuf, int aErrBufLen);

#endif //LUAEDIT_REMOTEDEBUGGER_HPP