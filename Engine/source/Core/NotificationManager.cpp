
#include "NotificationManager.h"
#include "CKLBUtility.h"

enum {
	NM_SET_NOTIFICATION,
	NM_REQUIRE_PERMISSION,
	NM_CANCEL_NOTIFICATION,
	NM_GET_ENABLE_NOTIFICATION,
	NM_GET_REMOTE_TOKEN
};

static IFactory::DEFCMD cmd[] = {
	{"NM_SET_NOTIFICATION",			NM_SET_NOTIFICATION			},
	{"NM_REQUIRE_PERMISSION",		NM_REQUIRE_PERMISSION		},
	{"NM_CANCEL_NOTIFICATION",		NM_CANCEL_NOTIFICATION		},
	{"NM_GET_ENABLE_NOTIFICATION",	NM_GET_ENABLE_NOTIFICATION	},
	{"NM_GET_REMOTE_TOKEN",			NM_GET_REMOTE_TOKEN			},
	{0, 0}
};

static CKLBTaskFactory<NotificationManager> factory("NotificationManager", CLS_NOTIFICATIONMANAGER, cmd);

NotificationManager::NotificationManager()
: CKLBLuaTask()
{

}

NotificationManager::~NotificationManager()
{

}

NotificationManager*
NotificationManager::create()
{
	NotificationManager* pTask = KLBNEW(NotificationManager);
	if (!pTask) { return NULL; }
	return pTask;
}

bool
NotificationManager::initScript(CLuaState& lua)
{
	lua.printStack();
	return regist(NULL, P_NORMAL);
}

void
NotificationManager::execute(u32 /*deltaT*/)
{

}

void
NotificationManager::die()
{

}

int 
NotificationManager::commandScript(CLuaState& lua) {
	return 0;
}