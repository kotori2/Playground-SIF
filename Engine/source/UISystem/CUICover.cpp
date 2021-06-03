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
#include "CUICover.h"
;
// Command Values
enum {
	UI_COVER_ADDRECT,
	UI_COVER_REMOVERECT,
	UI_COVER_CLEARRECT,
	UI_COVER_SETCOLOR,
	UI_COVER_BUILD,
};

static IFactory::DEFCMD cmd[] = {
	{"UI_COVER_ADDRECT",		UI_COVER_ADDRECT },
	{"UI_COVER_REMOVERECT",		UI_COVER_REMOVERECT },
	{"UI_COVER_CLEARRECT",		UI_COVER_CLEARRECT },
	{"UI_COVER_SETCOLOR",		UI_COVER_SETCOLOR },
	{"UI_COVER_BUILD",			UI_COVER_BUILD },

	{0, 0}
};
static CKLBTaskFactory<CUICover> factory("UI_Cover", CLS_KLBUICOVER, cmd);

// Allowed Property Keys
CKLBLuaPropTask::PROP_V2 CUICover::ms_propItems[] = {
	UI_BASE_PROP,
	{	"order",			UINTEGER,	(setBoolT)&CUICover::setOrder,		(getBoolT)&CUICover::getOrder,		0		},
	{	"alpha",			UINTEGER,	(setBoolT)&CUICover::setAlpha,		(getBoolT)&CUICover::getAlpha,		0		},
	{	"color",			UINTEGER,	(setBoolT)&CUICover::setU24Color,	(getBoolT)&CUICover::getU24Color,	0		},
};

enum {
	ARG_PARENT = 1,

	ARG_ORDER,
	ARG_ALPHA,
	ARG_COLOR,

	ARG_REQUIRE = ARG_COLOR,
	ARG_NUMS = ARG_COLOR
};

CUICover::CUICover()
	: CKLBUITask()
	, m_alpha(0)
	, m_color(0)
	, m_counter(0)
	, m_order(0)
	, m_texture(NULL)
	
{
	setNotAlwaysActive();
	m_newScriptModel = true;
}

CUICover::~CUICover()
{
	if (m_texture) KLBDELETEA(m_texture);
}

u32
CUICover::getClassID()
{
	return CLS_KLBUICOVER;
}

CUICover* CUICover::create(CKLBUITask* parent, CKLBNode* pNode, u32 order, u32 alpha, u32 color) {
	CUICover* pTask = KLBNEW(CUICover);
	if (!pTask) { return NULL; }
	if (!pTask->init(parent, pNode, order, alpha, color)) {
		KLBDELETE(pTask);
		return NULL;
	}
	return pTask;
}

bool
CUICover::init(CKLBUITask* pParent, CKLBNode* pNode, u32 order, u32 alpha, u32 color) {
	if (!setupNode()) return false;
	bool bResult = initCore(order, alpha, color);
	bResult = registUI(pParent, bResult);
	if (pNode) {
		pParent->getNode()->removeNode(getNode());
		pNode->addNode(getNode());
	}
	return bResult;
}

bool
CUICover::initCore(u32 order, u32 alpha, u32 color)
{
	if (!setupPropertyList((const char**)ms_propItems, SizeOfArray(ms_propItems))) {
		return false;
	}

	klb_assert((((s32)order) >= 0), "Order Problem");

	m_order = order;
	m_alpha = alpha;
	m_color = color;
	return true;
}

bool
CUICover::initUI(CLuaState& lua)
{
	int argc = lua.numArgs();

	if (argc < ARG_REQUIRE || argc > ARG_NUMS) { return false; }

	return initCore(
		lua.getInt(ARG_ORDER),
		lua.getInt(ARG_ALPHA),
		lua.getInt(ARG_COLOR)
	);
}

int
CUICover::commandUI(CLuaState& lua, int argc, int cmd)
{
	int ret = 1;
	switch (cmd)
	{
	case UI_COVER_ADDRECT:
	{
		if (argc != 6) {
			lua.retBoolean(false);
			return 1;
		}

		if (m_rects.size() >= MAX_RECTS) {
			lua.retBoolean(false);
			return 1;
		}

		Rect rect;
		rect.x = lua.getInt(3);
		rect.y = lua.getInt(4);
		rect.w = lua.getInt(5);
		rect.h = lua.getInt(6);

		m_rects[m_counter++] = std::move(rect);

		lua.retBoolean(true);
		ret = 1;
	}
	break;
	case UI_COVER_REMOVERECT:
	{
		if (argc != 3) {
			lua.retBoolean(false);
			return 1;
		}
		u32 rect_id = lua.getInt(3);
		if (m_rects.count(rect_id) > 0) {
			m_rects.erase(rect_id);
		}

		lua.retBoolean(true);
		ret = 1;
	}
	break;
	case UI_COVER_CLEARRECT:
	{
		m_rects.clear();

		lua.retBoolean(true);
		ret = 1;
	}
	break;
	case UI_COVER_SETCOLOR:
	{
		if (argc != 4) {
			lua.retBoolean(false);
			return 1;
		}
		u32 color = lua.getInt(3);
		u32 alpha = lua.getInt(4);

		setColor(color & 0xffffff);
		setAlpha(alpha & 0xff);

		lua.retBoolean(true);
		ret = 1;
	}
	break;
	case UI_COVER_BUILD:
	{
		// TODO: Do render job

		lua.retBoolean(true);
		ret = 1;
	}
	break;
	}

	return ret;
}

void
CUICover::execute(u32 /*deltaT*/)
{
	RESET_A;

	//m_pLabel->setViewPortPos(0, 0);
}

void
CUICover::dieUI()
{

}
