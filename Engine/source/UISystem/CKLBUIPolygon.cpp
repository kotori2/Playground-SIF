/* 
   Copyright 2020 Playground-SIF developers

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
#include "CKLBUIPolygon.h"

enum {
	UI_POLYGON_NEWPATH,
	UI_POLYGON_PUSHPATH,
	UI_POLYGON_NEWHOLE,
	UI_POLYGON_ENDHOLE,
	UI_POLYGON_BUILD,
	UI_POLYGON_ADDPOINT,
	UI_POLYGON_SETTEXTURE,
};

static IFactory::DEFCMD cmd[] = {
	{"UI_POLYGON_NEWPATH",			UI_POLYGON_NEWPATH		},
	{"UI_POLYGON_PUSHPATH",			UI_POLYGON_PUSHPATH		},
	{"UI_POLYGON_NEWHOLE",			UI_POLYGON_NEWHOLE		},
	{"UI_POLYGON_ENDHOLE",			UI_POLYGON_ENDHOLE		},
	{"UI_POLYGON_BUILD",			UI_POLYGON_BUILD		},
	{"UI_POLYGON_ADDPOINT",			UI_POLYGON_ADDPOINT		},
	{"UI_POLYGON_SETTEXTURE",		UI_POLYGON_SETTEXTURE	},

	{0, 0 }
};

static CKLBTaskFactory<CKLBUIPolygon> factory("UI_Polygon", CLS_KLBUIPOLYGON, cmd);

// Allowed Property Keys
CKLBLuaPropTask::PROP_V2 CKLBUIPolygon::ms_propItems[] = {
	UI_BASE_PROP,
	{	"order",			R_UINTEGER,	NULL,									(getBoolT)&CKLBUIPolygon::getOrder,	0	},
	{	"maxpointcount",	UINTEGER,	(setBoolT)&CKLBUIPolygon::setMaxPoint,	(getBoolT)&CKLBUIPolygon::getMaxPoint,	0	}
};

// 引数のインデックス定義
enum {
	ARG_PARENT = 1,
	ARG_ORDER,
	
	ARG_NUMS    = ARG_ORDER,
	ARG_REQUIRE = ARG_ORDER	// 最低限必要なパラメータ数
};

CKLBUIPolygon::CKLBUIPolygon()
: CKLBUITask	()
, m_pPolyline	(NULL) 
{
	setNotAlwaysActive();
	m_newScriptModel = true;
}

CKLBUIPolygon::~CKLBUIPolygon()
{
}

u32
CKLBUIPolygon::getClassID()
{
	return CLS_KLBUIPOLYGON;
}

CKLBUIPolygon*
CKLBUIPolygon::create(CKLBUITask* pParent, CKLBNode* pNode, u32 order)
{
	CKLBUIPolygon* pTask = KLBNEW(CKLBUIPolygon);
    if (!pTask) { return NULL; }
	if (!pTask->init(pParent, pNode, order)) {
		KLBDELETE(pTask);
		return NULL;
	}
	return pTask;
}

bool CKLBUIPolygon::init(CKLBUITask* pParent, CKLBNode* pNode, u32 order) {
    if(!setupNode()) { return false; }
	bool bResult = initCore(order);
	bResult = registUI(pParent, bResult);
	if(pNode) {
		pParent->getNode()->removeNode(getNode());
		pNode->addNode(getNode());
	}
	return bResult;
}

bool 
CKLBUIPolygon::initCore(u32 order)
{
	DEBUG_PRINT("CKLBUIPolygon: NOT IMPLEMENTED");
	if(!setupPropertyList((const char**)ms_propItems,SizeOfArray(ms_propItems))) {
		return false;
	}

	m_maxpointcount = 65535;  // TODO: Slow in performance, fix this
	m_order         = order;
	m_idx			= 0;

	// 必要とされるオブジェクトを生成する
	CKLBRenderingManager& pRdrMgr = CKLBRenderingManager::getInstance();

	m_pPolyline = pRdrMgr.allocateCommandPolyline(m_maxpointcount, order);
	if(!m_pPolyline) {
		return false;
	}

	// 二つのDynSpriteを自分のNodeに登録
	getNode()->setRender(m_pPolyline);
	getNode()->setRenderOnDestroy(true);

	getNode()->markUpMatrix();
	
	return true;
}

bool
CKLBUIPolygon::initUI(CLuaState& lua)
{
	int argc = lua.numArgs();
    if(argc > ARG_NUMS || argc < ARG_REQUIRE) { return false; }

	u32 order = lua.getInt(ARG_ORDER);
	return initCore(order);
}

int
CKLBUIPolygon::commandUI(CLuaState& lua, int argc, int cmd)
{
	// TODO: Not implemented
	int ret = 0;
	switch (cmd)
	{
	case UI_POLYGON_NEWPATH:
	{
		bool result = false;
		if (argc == 2) {
			result = true;
		}
		lua.retBoolean(result);
		ret = 1;
	}
	break;
	case UI_POLYGON_PUSHPATH:
	{
		bool result = false;
		if (argc == 2) {
			result = true;
		}
		lua.retBoolean(result);
		ret = 1;
	}
	break;
	case UI_POLYGON_NEWHOLE:
	{
		bool result = false;
		if (argc == 2) {
			result = true;
		}
		lua.retBoolean(result);
		ret = 1;
	}
	break;
	case UI_POLYGON_ENDHOLE:
	{
		bool result = false;
		if (argc == 2) {
			result = true;
		}
		lua.retBoolean(result);
		ret = 1;
	}
	break;
	case UI_POLYGON_BUILD:
	{
		bool result = false;
		if (argc == 4) {
			int color = lua.getInt(3);
			int alpha = lua.getInt(4);
			result = true;
		}
		lua.retBoolean(result);
		ret = 1;
	}
	break;
	case UI_POLYGON_ADDPOINT:
	{
		bool result = false;
		if (argc == 4) {
			// Simple point, no color
			int x = lua.getInt(3);
			int y = lua.getInt(4);
			result = true;
		} else if (argc == 6) {
			// With color
			int x = lua.getInt(3);
			int y = lua.getInt(4);
			int color = lua.getInt(5);
			int alpha = lua.getInt(6);
			result = true;
		}
		lua.retBoolean(result);
		ret = 1;
	}
	break;
	case UI_POLYGON_SETTEXTURE:
	{
		bool result = false;
		if (argc == 5) {
			const char* texture = lua.getString(3);
			int visible = lua.getInt(4);
			bool unknown = lua.getBoolean(5);
			result = true;
		}
		lua.retBoolean(result);
		ret = 1;
	}
	break;
	}
	return ret;
}

void
CKLBUIPolygon::execute(u32 /*deltaT*/)
{
	// Should never be executed.
	klb_assertAlways("Task execution is not necessary");
}

void
CKLBUIPolygon::dieUI()
{
}
