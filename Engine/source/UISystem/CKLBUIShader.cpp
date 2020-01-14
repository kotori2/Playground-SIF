
#include "CKLBUIShader.h"

static IFactory::DEFCMD cmd[] = {
	// command values
	{"SHADER_ANIM",		SHADER_ANIM							},
	{"PSHADER_VALUE",	CShaderInstance::PIXEL_SHADER		},
	{"VSHADER_VALUE",	CShaderInstance::VERTEX_SHADER		},

	// constants
	{"PSHADER_PARAM",	CShaderInstance::PIXEL_SHADER		},
	{"VSHADER_PARAM",	CShaderInstance::VERTEX_SHADER		},
	{"SHD_VEC1",		VEC1F				},
	{"SHD_VEC2",		VEC2				},
	{"SHD_VEC3",		VEC3				},
	{"SHD_VEC4",		VEC4				},
	{"SHD_TEX2D",		TEX2D				},
	{"SHD_LOW",			QUALITY_TYPE::LOWP	},
	{"SHD_MED",			QUALITY_TYPE::MEDP	},
	{"SHD_HIGH",		QUALITY_TYPE::HIGHP	},
	{0, 0}
};

static CKLBTaskFactory<CKLBUIShader> factory("UI_Shader", CLS_KLBUISHADER, cmd);


CKLBLuaPropTask::PROP_V2 CKLBUIShader::ms_propItems[] = {
	UI_BASE_PROP
};

CKLBUIShader::CKLBUIShader()
: CKLBUITask()
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

bool
CKLBUIShader::initCore()
{

	return false;
}

bool
CKLBUIShader::initUI(CLuaState& lua)
{
	
	return false;
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
