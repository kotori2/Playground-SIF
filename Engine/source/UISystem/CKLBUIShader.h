
#ifndef UIShader_h

#define UIShader_h
#include "CKLBUITask.h"

enum {
	SHADER_ANIM,
	PSHADER_VALUE,

};

class CKLBUIShader : public CKLBUITask
{
	friend class CKLBTaskFactory<CKLBUIShader>;
private:
	CKLBUIShader();
	virtual ~CKLBUIShader();

	bool initCore();
public:
	virtual u32 getClassID();
	bool initUI(CLuaState& lua);
	int commandUI(CLuaState& lua, int argc, int cmd);

	void execute(u32 deltaT);
	void dieUI();

private:
	static	PROP_V2		ms_propItems[];
};

#endif UIShader_h