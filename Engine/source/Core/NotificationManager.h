
#ifndef NotificationManager_h
#define NotificationManager_h

#include "CKLBLuaTask.h"

class NotificationManager : public CKLBLuaTask
{
	friend class CKLBTaskFactory<NotificationManager>;
private:
	NotificationManager();
	virtual ~NotificationManager();

	bool init(CKLBTask* pTask, const char* pause, const char* resume);
public:
	static NotificationManager* create();
	bool initScript(CLuaState& lua);
	int commandScript(CLuaState& lua);
	void execute(u32 deltaT);
	void die();
};

#endif // !NotificationManager_h
