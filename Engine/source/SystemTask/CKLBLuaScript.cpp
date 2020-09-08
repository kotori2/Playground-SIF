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
//
//  CKLBLuaScript.cpp
//

#include "CKLBLuaScript.h"

std::queue<std::function<void()>> CKLBLuaScript::m_callback_queue;
void* CKLBLuaScript::m_lock;

CKLBLuaScript::CKLBLuaScript() : CKLBTask() {
    IPlatformRequest& platform = CPFInterface::getInstance().platform();
    m_lock = platform.allocMutex();
}
CKLBLuaScript::~CKLBLuaScript() {
    IPlatformRequest& platform = CPFInterface::getInstance().platform();
    platform.freeMutex(m_lock);
}

bool
CKLBLuaScript::onPause(bool /*bPause*/)
{
	return false;
}

void
CKLBLuaScript::execute(u32 deltaT)
{
    CKLBLuaEnv& env = CKLBLuaEnv::getInstance();
    env.execScript(deltaT);

    IPlatformRequest& platform = CPFInterface::getInstance().platform();
    platform.mutexLock(m_lock);
    if (!m_callback_queue.empty()) {
        auto func = m_callback_queue.front();
        m_callback_queue.pop();
        func();
    }
    platform.mutexUnlock(m_lock);

}

void
CKLBLuaScript::die() {
    // Lua実行環境終了
    //CKLBLuaEnv::getInstance().finishLuaEnv();
}

void CKLBLuaScript::enqueue(std::function<void()> func)
{
    m_callback_queue.push(func);
}


CKLBLuaScript *
CKLBLuaScript::create(const char *bootScriptURL)
{
    // 初期スクリプトのロードおよび初期化処理実行。
    // これ以後、スクリプトからタスクの起動が出来る。
    if(!CKLBLuaEnv::getInstance().setupLuaEnv()) return NULL;    
    if(!CKLBLuaEnv::getInstance().loadScript(bootScriptURL)) {
		// boot script が読めないため、assertを出す。
		klb_assertAlways("NOT FOUND! [ start.lua ]");
		return NULL;
	}
    // 初期スクリプトのロードおよび、初期化関数setup()の実行に成功したら、
    // タスク処理タスクを起動する。
    CKLBLuaScript * pTask = KLBNEW(CKLBLuaScript);
    if(!pTask) return NULL;

    // スクリプトタスクは、必ず P_SCRIPT フェーズで動作する。
    if(!pTask->regist(NULL, P_SCRIPT)) {
        KLBDELETE(pTask);
        return NULL;
    }
    return pTask;
}

u32
CKLBLuaScript::getClassID()
{
	return CLS_KLBTASKSCRIPT;
}