
#ifndef AdManager_h
#define AdManager_h

#include "CKLBLuaTask.h"

class AdManager : public CKLBLuaTask
{
	friend class CKLBTaskFactory<AdManager>;
private:
	const char* m_callbackSuccessAdReward;
	const char* m_callbackFailAdReward;
private:
	AdManager();
	virtual ~AdManager();

	bool init(CKLBTask* pTask, const char* pause, const char* resume);
public:
	static AdManager* create();
	u32 AdManager::getClassID();
	bool initScript(CLuaState& lua);
	int commandScript(CLuaState& lua);
	void execute(u32 deltaT);
	void die();
};

#endif // !AdManager_h
