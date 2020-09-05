
#include "AdManager.h"
#include "CKLBUtility.h"

enum {
	AD_PRELOAD,
	AD_SHOW
};

static IFactory::DEFCMD cmd[] = {
	{"AD_PRELOAD",			AD_PRELOAD			},
	{"AD_SHOW",				AD_SHOW				},
	{0, 0}
};

static CKLBTaskFactory<AdManager> factory("AdManager", CLS_ADMANAGER, cmd);

AdManager::AdManager()
: CKLBLuaTask()
, m_callbackFailAdReward(nullptr)
, m_callbackSuccessAdReward(nullptr)
{

}

AdManager::~AdManager()
{

}

u32
AdManager::getClassID()
{
	return CLS_ADMANAGER;
}

AdManager*
AdManager::create()
{
	AdManager* pTask = KLBNEW(AdManager);
	if (!pTask) { return NULL; }
	return pTask;
}

bool
AdManager::initScript(CLuaState& lua)
{
	klb_assert(lua.numArgs() == 2, "No valid amount of args for AdManager");
	m_callbackSuccessAdReward = CKLBUtility::copyString(lua.getString(1));
	m_callbackFailAdReward    = CKLBUtility::copyString(lua.getString(2));
	return regist(NULL, P_NORMAL);
}

void
AdManager::execute(u32 /*deltaT*/)
{

}

void
AdManager::die()
{
	KLBDELETEA(m_callbackSuccessAdReward);
	KLBDELETEA(m_callbackFailAdReward);
}

int 
AdManager::commandScript(CLuaState& lua) {
	int argc = lua.numArgs();
	int cmd = lua.getInt(2);

	switch (cmd) {
	case AD_PRELOAD:
		DEBUG_PRINT("AD_PRELOAD is not implemented"); break;
	case AD_SHOW:
		DEBUG_PRINT("AD_SHOW is not implemented"); break;
	default:
		klb_assertAlways("AdManager: Unknown command: %d", cmd);
	}
	return 0;
}