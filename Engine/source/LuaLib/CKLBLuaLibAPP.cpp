/* 
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
#include "CKLBLuaLibAPP.h"
#include "CKLBScriptEnv.h"	
#include <ctime>

static ILuaFuncLib::DEFCONST luaConst[] = {
	{ "APP_MAIL",		IPlatformRequest::APP_MAIL },		// 各環境のメールアプリ
	{ "APP_BROWSER",	IPlatformRequest::APP_BROWSER },	// 各環境のブラウザアプリ
	{ "APP_UPDATE",		IPlatformRequest::APP_UPDATE },		// 各環境のアップデートアプリ
	{ "APP_SETTINGS",	IPlatformRequest::APP_SETTINGS },	// Location permission
	{ "APP_MAP",		IPlatformRequest::APP_MAP },		// Location related
	{ "APP_ATT",		IPlatformRequest::APP_ATT },		// iOS allow track window
	{ 0, 0 }
};

static CKLBLuaLibAPP libdef(luaConst);

CKLBLuaLibAPP::CKLBLuaLibAPP(DEFCONST * arrConstDef) : ILuaFuncLib(arrConstDef) {}
CKLBLuaLibAPP::~CKLBLuaLibAPP() {}

// Lua関数の追加
void
CKLBLuaLibAPP::addLibrary()
{
	addFunction("APP_CallApplication",		CKLBLuaLibAPP::luaCallApplication);
	addFunction("APP_GetPhysicalMem",		CKLBLuaLibAPP::luaGetPhysicalMem);
	addFunction("APP_DateTimeNow",			CKLBLuaLibAPP::luaDateTimeNow);
	addFunction("APP_SetIdleTimerActivity", CKLBLuaLibAPP::luaSetIdleTimerActivity);
	addFunction("APP_GetBundleID",			CKLBLuaLibAPP::luaGetBundleId);
	addFunction("SBP_setUserDefaults",		CKLBLuaLibAPP::luaSetUserDefaults);
}

int
CKLBLuaLibAPP::luaGetPhysicalMem(lua_State * L)
{
	CLuaState lua(L);
	//int argc = lua.numArgs();

	u32 value = CPFInterface::getInstance().platform().getPhysicalMemKB();
	if (value >= 0x1000000) {
		// 24 bit significant.
		value = 0xFFFFFF;
	}
	lua.retInt(value);
	return 1;
}

int
CKLBLuaLibAPP::luaCallApplication(lua_State * L)
{
	CLuaState lua(L);
	int argc = lua.numArgs();
	if(argc < 1) {
		lua.retBoolean(false);
		return 1;
	}

	bool result = false;
	IPlatformRequest::APP_TYPE type = (IPlatformRequest::APP_TYPE)lua.getInt(1);
	IPlatformRequest& pForm = CPFInterface::getInstance().platform();

	switch(type)
	{
	case IPlatformRequest::APP_MAIL:
		{
			const char * addr = (lua.isNil(2)) ? "" : lua.getString(2);
			const char * subject = (lua.isNil(3)) ? "" : lua.getString(3);
			const char * body = (lua.isNil(4)) ? "" : lua.getString(4);

			result = pForm.callApplication(type, addr, subject, body);
		}
		break;
	case IPlatformRequest::APP_BROWSER:
		{
			const char * url = (lua.isNil(2)) ? "" : lua.getString(2);

			result = pForm.callApplication(type, url);
		}
		break;
	case IPlatformRequest::APP_UPDATE:
		{
			const char * search_key = (argc >= 2 && !lua.isNil(2)) ? lua.getString(2) : "";
			result = pForm.callApplication(type, search_key);
		}
		break;
	case IPlatformRequest::APP_ATT:
		{
			const char* callback = lua.getString(2);
			bool is_request = lua.getBool(3);  // true == request permission, false == check permission
			const char* request_type = is_request ? "request permission" : "check permission";
			DEBUG_PRINT("APP_CallApplication: APP_ATT: %s => ATT_REJECTED", request_type);

			// TODO: Fill this permission
			CKLBScriptEnv::getInstance().call_attResult(callback, IPlatformRequest::ATT_REJECTED);
		}
		break;
	default:
		DEBUG_PRINT("APP_CallApplication: %d is not implemented", type);
		break;
	}
	lua.retBoolean(result);
	return 1;
}

// For C#
bool CKLBLuaLibAPP::callApplication(IPlatformRequest::APP_TYPE type, const char* addr, const char* subject, const char* body)
{
	bool result = false;
	IPlatformRequest& pForm = CPFInterface::getInstance().platform();

	switch(type)
	{
	case IPlatformRequest::APP_MAIL:
		{
			result = pForm.callApplication(type, addr, subject, body);
		}
		break;
    case IPlatformRequest::APP_BROWSER:
        {
            result = pForm.callApplication(type, addr);
        }
        break;
	default:
		break;
	}
	return result;
}

int
CKLBLuaLibAPP::luaDateTimeNow(lua_State * L)
{
	CLuaState lua(L);
	DEBUG_PRINT("APP_DateTimeNow implemented not correctly");
	lua.retInt((u32)std::time(0)); // should be formatted like "YYYY-MM-DD-HH-mm-ss" in JP timezone
	return 1;
}

int
CKLBLuaLibAPP::luaSetIdleTimerActivity(lua_State * L)
{
	// arg 1 is boolean
	DEBUG_PRINT("APP_SetIdleTimerActivity not implemented yet");
	return 1;
}

int
CKLBLuaLibAPP::luaGetBundleId(lua_State* L)
{
	CLuaState lua(L);
	IPlatformRequest& platform = CPFInterface::getInstance().platform();
	lua.retString(platform.getBundleId());
	return 1;
}

int
CKLBLuaLibAPP::luaSetUserDefaults(lua_State* L)
{
	CLuaState lua(L);
	const char* permission_name = lua.getString(1);
	bool granted = lua.getBool(2);
	const char* granted_text = granted ? "true" : "false";
	DEBUG_PRINT("SBP_setUserDefaults: %s, %s", permission_name, granted_text);
	return 0;
}
