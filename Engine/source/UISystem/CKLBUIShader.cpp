
#include "CKLBUIShader.h"

enum {
	VSHADER_VALUE = 0,
	PSHADER_VALUE,
	VSHADER_TEXTURE,
	PSHADER_TEXTURE,
	SHADER_ANIM,
	UV_REPEAT,
	SAMPLING_LINEAR
};

static IFactory::DEFCMD cmd[] = {
	// command values
	{"VSHADER_VALUE",	VSHADER_VALUE						},
	{"PSHADER_VALUE",	PSHADER_VALUE						},
	{"VSHADER_TEXTURE",	VSHADER_TEXTURE						},
	{"PSHADER_TEXTURE",	PSHADER_TEXTURE						},
	{"SHADER_ANIM",		SHADER_ANIM							},
	{"UV_REPEAT",		UV_REPEAT							},
	{"SAMPLING_LINEAR",	SAMPLING_LINEAR						},

	// constants
	{"VSHADER_PARAM",	CShaderInstance::VERTEX_SHADER		},
	{"PSHADER_PARAM",	CShaderInstance::PIXEL_SHADER		},
	{"SHD_VEC1",		VEC1F								},
	{"SHD_VEC2",		VEC2								},
	{"SHD_VEC3",		VEC3								},
	{"SHD_VEC4",		VEC4								},
	{"SHD_TEX2D",		TEX2D								},
	{"SHD_LOW",			QUALITY_TYPE::LOWP					},
	{"SHD_MED",			QUALITY_TYPE::MEDP					},
	{"SHD_HIGH",		QUALITY_TYPE::HIGHP					},
	{0, 0}
};

static CKLBTaskFactory<CKLBUIShader> factory("UI_Shader", CLS_KLBUISHADER, cmd);


CKLBLuaPropTask::PROP_V2 CKLBUIShader::ms_propItems[] = {
	UI_BASE_PROP
};

CKLBUIShader::CKLBUIShader()
: CKLBUITask()
, m_shaderName(NULL)
{
	m_newScriptModel = true;
}

CKLBUIShader::~CKLBUIShader()
{
}

u32
CKLBUIShader::getClassID()
{
	return CLS_KLBUISHADER;
}

CKLBUIShader*
CKLBUIShader::create()
{
	CKLBUIShader* pTask = KLBNEW(CKLBUIShader);
	if (!pTask) { return NULL; }
	if (!pTask->init()) {
		KLBDELETE(pTask);
		return NULL;
	}
	return pTask;
}

bool 
CKLBUIShader::init() {
	bool bResult = initCore();
	bResult = registUI(NULL, bResult);
	return bResult;
}

bool
CKLBUIShader::initCore()
{
	return true;
}

bool
CKLBUIShader::initUI(CLuaState& lua)
{
	return initCore();
}

int
CKLBUIShader::commandUI(CLuaState& lua, int argc, int cmd)
{
	return 0;
}

void
CKLBUIShader::execute(u32 /*deltaT*/)
{
}

void
CKLBUIShader::dieUI()
{
}
