
#ifndef UIShader_h

#define UIShader_h
#include "CKLBUITask.h"

class CKLBUIShader : public CKLBUITask
{
	friend class CKLBTaskFactory<CKLBUIShader>;
private:
	CKLBUIShader();
	virtual ~CKLBUIShader();

	bool initCore();
	bool init();
public:
	virtual u32 getClassID();
	static CKLBUIShader* create();
	bool initUI(CLuaState& lua);
	int commandUI(CLuaState& lua, int argc, int cmd);

	void execute(u32 deltaT);
	void dieUI();

private:
	const char*			m_shaderName;
	static	PROP_V2		ms_propItems[];
};

#endif //UIShader_h