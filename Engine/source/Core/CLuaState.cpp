﻿/* 
   Copyright 2013 KLab Inc.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
//
//  CLuaState.cpp
//
#include <stdio.h>
#include "klb_vararg.h"
#include "CLuaState.h"
#include "CKLBLuaEnv.h"
#include "map"

#ifdef _WIN32
#include <Windows.h>
#endif

static std::map<lua_State*, void*> LockList;

namespace
{

int
traceback(lua_State* L)
{
  const char* message = lua_tostring(L, 1);
  if (message)
    {
      luaL_traceback(L, L, message, 1);
    }
  else if (!lua_isnoneornil(L, 1))
    {
      if(!luaL_callmeta(L, 1, "__tostring"))
        {
          lua_pushliteral(L, "(noeror message)");
        }
    }

  return 1;
}

} // noname namespace

CLuaState::CLuaState(lua_State * L) 
: m_L(L) 
{
}

CLuaState::~CLuaState() 
{
}

const char *
CLuaState::getScriptName()
{
    return CKLBLuaEnv::getInstance().nowFile();
}

void
CLuaState::errorMsg(const char *type_name, int argnum)
{
    char buf[128];
	sprintf(buf, "invalid argment type (arg:%d is not %s.)", argnum, type_name);

#ifdef _WIN32// && DEBUG
#ifdef DEBUG
	DEBUG_PRINT(buf);
	DebugBreak();
#endif
#else
    //CKLBLuaEnv::getInstance().errMsg(buf);
    error("%s", buf);
#endif
}

bool
CLuaState::callback(const char * func, const char * argform, ...)
{
	va_list ap;
	va_start(ap, argform);
    luaLock();
	bool result = call_luafunction(0, func, argform, ap);
    luaUnlock();
	va_end(ap);
	return result;
}

bool
CLuaState::retcall(int retnum, const char * func, const char * argform, ...)
{
	va_list ap;
	va_start(ap, argform);
    luaLock();
	bool result = call_luafunction(retnum, func, argform, ap);
    luaUnlock();
	va_end(ap);
	return result;
}

bool
CLuaState::call_luafunction(int retnum, const char *func, const char *argform, va_list ap)
{
    // lua関数の名称をスタックに積む
    lua_getglobal(m_L, func);

    int count = 0;
        
    // 引数をスタックに積む
    if(argform) {
        for(const char * sp = argform; *sp; sp++) {
            switch(*sp)
            {
                case 'B': {
                    int b = va_arg(ap, int);
                    retBoolean((b) ? true : false);
                    count++;
                    break;
                }
                case 'I': {
                    int i = va_arg(ap, int);
                    retInt(i);
                    count++;
                    break;
                }
                case 'N': {
                    double d = va_arg(ap, double);
                    retDouble(d);
                    count++;
                    break;
                }
                case 'S':{
                    const char * str = va_arg(ap, const char *);
					retString(str);
                    count++;
                    break;
                }
                case 'P':{
                    void * p = va_arg(ap, void *);
                    retPointer(p);
                    count++;
                    break;
                }
                case 'G': {
                    const char * p = va_arg(ap, const char *);
                    retGlobal(p);
                    count++;
                    break;
                }
            }
        }
    }
	return call(count, func, retnum);
}

bool
CLuaState::call(int args, const char * func, int nresults)
{
    // call stackを取れるようにtraceback函数をpushしpcallの第四引数を修正.
    int base = lua_gettop(m_L) - args;
    lua_pushcfunction(m_L, traceback);
    lua_insert(m_L, base);
    int result = lua_pcall(m_L, args, nresults, base);
    lua_remove(m_L, base);

    if(result) {
    	const char * msg = NULL;
        
    	switch(result)
    	{
            default:            msg = "unknown error: %s (%s)";             break;
            case LUA_ERRRUN:    msg = "runtime error: %s (%s)";             break;
            case LUA_ERRMEM:    msg = "memory allocation error: %s (%s)";   break;
            case LUA_ERRERR:    msg = "error in error: %s (%s)";            break;
    	}
        // 呼び出しエラー: 指定の関数呼び出しに失敗
		const char * errmsg = getString(-1);
        int buff_len = strlen(msg) + strlen(errmsg) + strlen(func) + 1;
        char* buffer = KLBNEWA(char, buff_len);
#if defined(_WIN32)
        sprintf_s(buffer, buff_len, msg, errmsg, func);
#else
        snprintf(buffer, buff_len, msg, errmsg, func);
#endif // #if defined(_WIN32)
        CKLBLuaEnv::getInstance().errMsg(buffer);
		klb_assertAlways("%s", buffer);
        KLBDELETEA(buffer); // assert発生するとここまで来ない予感はする.
        return false;
    }
    return true;
}

int
CLuaState::error(const char * fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	char msg[1024];
	vsprintf(msg, fmt, ap);
	va_end(ap);

	return luaL_error(m_L, msg);
}

const char* Luatype_names[] = {
	"nil",
	"boolean",
	"light_userdata",
	"number",
	"string",
	"table",
	"function",
	"userdata",
	"coroutine",
	NULL,
};

void CLuaState::printStack() {
	for (int i = 1; i <= numArgs(); i++) {
		int t = lua_type(m_L, i);

		printf("Stack %d is %s", i, Luatype_names[t]);

		switch (t)
		{
		case LUA_TBOOLEAN:
		{
			printf(": %s\n", getBool(i) ? "true" : "false");
			break;
		}
		case LUA_TNUMBER:
		{
			printf(": %lf\n", getDouble(i));
			break;
		}
		case LUA_TSTRING:
		{
			klb_assert(luaL_loadstring(m_L, "return string.format(\"%q\", ({...})[1])") == 0, "Syntax error");
			retValue(i);
			klb_assert(lua_pcall(m_L, 1, 1, 0) == 0, "Syntax error");

			printf(": %s\n", getString(-1));
			lua_pop(m_L, 1);

			break;
		}
		case LUA_TTABLE:
		{
			printf("\nLUA TABLE DUMP START STACK %d\n", i);
			// only one depth level
            const char* str = "for a,b in pairs(({...})[1])do print(a,b)end";
			klb_assert(luaL_loadstring(m_L, str) == 0, "Syntax error");
			retValue(i);
			klb_assert(lua_pcall(m_L, 1, 0, 0) == 0, "Syntax error");
			printf("LUA TABLE DUMP END STACK %d\n", i);

			break;
		}
		default:
		{
			printf(": %p\n", lua_topointer(m_L, i));
			break;
		}
		}
	}
}

void CLuaState::luaLock()
{
    IPlatformRequest& platform = CPFInterface::getInstance().platform();
    void* lock = LockList[m_L];

    if (lock == NULL)
    {
        lock = platform.allocMutex();
        klb_assert(lock, "Failed to alloc mutex for lua state %p", m_L);
        LockList[m_L] = lock;
    }

    platform.mutexLock(lock);
}

void CLuaState::luaUnlock()
{
    void* lock = LockList[m_L];

    klb_assert(lock, "Mutex for lua state %p does not exist", m_L);

    CPFInterface::getInstance().platform().mutexUnlock(lock);
}

