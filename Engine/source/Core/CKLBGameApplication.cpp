﻿/* 
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
//
//  CKLBGameApplication.cpp
//  GameEngine
//
//
#include <stdlib.h>
#include "CKLBGameApplication.h"

#include "CKLBTouchPad.h"
#include "CKLBDeviceKeyEvent.h"
#include "CKLBOSCtrlEvent.h"
#include "CKLBLuaScript.h"
#include "CKLBDrawTask.h"
#include "CKLBTouchEventUI.h"
#include "CKLBLabelNode.h"
#include "MultithreadedNetwork.h"

#ifdef DEBUG_MENU
#include "CKLBDebugMenu.h"
#endif

#if defined (DEBUG_MEMORY)
#include "DebugTracker.h"
#endif

// asset manager
#include "CKLBSWFPlayer.h"
#include "CompositeManagement.h"
#include "NodeAnimationAsset.h"
#include "CKLBDatabase.h"
#include "CKLBAppProperty.h"
#include "AudioAsset.h"
#include "CKLBTexturePacker.h"
#include "CKLBNetAPIKeyChain.h"
#include "CKLBFormGroup.h"
#include "CKLBLanguageDatabase.h"
#include "CompositeManagement.h"
#include "CKLBHTTPInterface.h"
#include "DownloadQueue.h"

// Global Text rendering buffer.
#include "CKLBTextTempBuffer.h"
int CKLBGameApplication::m_decryptedBaseKey[34] = {};
bool CKLBGameApplication::m_isNMAssetKeyDecrypted = false;
char* CKLBGameApplication::m_NMAssetKey = NULL;
int CKLBGameApplication::m_NMAssetKeyLen = NULL;
const uint16_t CKLBGameApplication::encryptedBaseKey[128] = { 0x366, 0xc6, 0x346, 0x2c6, 0x46, 0x106, 0x1e6, 0xa6, 0x306, 0x226, 0x2a6, 0x26, 0x386, 0x2e6, 0x146, 0x406, 0xe6, 0x3a6, 0x166, 0x1c6, 0x266, 0x186, 0x3e6, 0x246, 0x66, 0x206, 0x286, 0x365, 0xc5, 0x2c5, 0x45, 0x105, 0x1e5, 0x325, 0x305, 0x225, 0x2a5, 0x3c5, 0x25, 0x385, 0x2e5, 0x145, 0x1a5, 0x5, 0x405, 0xe5, 0x165, 0x1c5, 0x85, 0x265, 0x125, 0x3e5, 0x245, 0x65, 0x205, 0x285, 0x344, 0x324, 0x3c4, 0x1a4, 0xe4, 0x3a4, 0x84, 0x124, 0x184, 0x3e4, 0x244, 0x64, 0x284, 0x363, 0xc3, 0x43, 0x103, 0x1e3, 0x303, 0x2a3, 0x3c3, 0x383, 0x403, 0x3a3, 0x163, 0x1c3, 0x263, 0x183, 0x3e3, 0x203, 0x362, 0x322, 0x222, 0x2a2, 0x22, 0x382, 0x2e2, 0x1a2, 0x402, 0x1c2, 0x82, 0x122, 0x422, 0x62, 0x202, 0x282, 0x361, 0x341, 0x2c1, 0x321, 0x2a1, 0x381, 0x401, 0x1c1, 0x360, 0x340, 0x40, 0xa0, 0x300, 0x220, 0x2a0, 0x3c0, 0x20, 0x380, 0x2e0, 0x140, 0x400, 0x160, 0x1c0, 0x420, 0x200, 0x280 };

CKLBGameApplication::CKLBGameApplication()
: IClientRequest			()
, m_bootFile				(NULL)
, m_reboot					(false)
, m_frameTime				(16)
, m_outStream				(NULL)
, m_useDefaultDB			(false)
, m_useDefaultFont			(true)
{
}

CKLBGameApplication::~CKLBGameApplication() 
{
	if(m_bootFile ) KLBDELETEA(m_bootFile);	// 2012.12.12  finidhGame内でコメントアウトした物をここへ移動
#if defined (DEBUG_MEMORY)
	CTracker::End();
#endif
	// _CrtDumpMemoryLeaks();
}

/*virtual*/
void 
CKLBGameApplication::pauseGame(bool pause) 
{
	CKLBTaskMgr::getInstance().setFreeze(pause);
}

/*virtual*/
void 
CKLBGameApplication::setInitParam(u32 param, void* /*complexSetup*/) 
{
	m_useDefaultDB	= (param & ENGINE_USE_DEFAULTDB)	!= 0;
	m_useDefaultFont= (param & ENGINE_USE_DEFAULTFONT)	!= 0;
}

bool
CKLBGameApplication::setScreenInfo(bool /*rotate*/,int width, int height)
{
	// phisical screen info
	m_width = width;
	m_height = height;
	return true;
}

/*virtual*/
int 
CKLBGameApplication::getPhysicalScreenWidth() 
{
	return m_width;
}

/*virtual*/
int 
CKLBGameApplication::getPhysicalScreenHeight() 
{
	return m_height;
}

bool
CKLBGameApplication::setFilePath(const char * strPath)
{
	int len = (!strPath) ? 0 : strlen(strPath);
	const char * ptr = (!len) ? "start.lua" : strPath;
	len = strlen(ptr) + sizeof("file://install/");
	char * buf = KLBNEWA(char, len + 1);
	sprintf(buf, "file://install/%s", ptr);
	m_bootFile = (const char *)buf;
    return true;
}

bool
CKLBGameApplication::initGame()
{
	bool res = true;
#if defined (DEBUG_MEMORY)
	CTracker::Init("socket://127.0.0.1:6542",true);
#endif

	AllocationSize allocSize;
	allocSize.dictionnaryNodePoolSize	= 15000;
	allocSize.handlerPoolSize			= 10000;
	allocSize.maxAssetCount				= 1000;
	allocSize.formTemplateNodeCount		= 10000;

	initNMAsset();
	this->setupAllocation(&allocSize);

	srand(3920567);
	if (res) {  res &= CKLBInnerDefManager::initManager(allocSize.formTemplateNodeCount); }

	if (res) {	res &= CKLBTextTempBuffer::allocatorBuffer(200, 40, 4); } // Before InitialTasks !
	if (res) {	res &= initSystem(&allocSize);	}
	if (res) {	res &= initOther();		}
	if (res) {  res &= CKLBDataHandler::init(allocSize.handlerPoolSize); }
	if (res) {  res &= CKLBHTTPInterface::initHTTPLib(); }
	if (res) {  res &= NetworkManager::startNetworkManager(); }

	if (res) {  res &= CKLBDatabase::getInstance().init(m_useDefaultDB ? "file://install/gamedb.db" : NULL,SQLITE_OPEN_READONLY); }
	if (res) {  res &= CKLBScriptEnv::getInstance().setupScriptEnv(); }
	// Init database
	if (res) {	res &= CKLBLanguageDatabase::getInstance().init();	}
	// タスクの立ち上げはすべての初期化が終わった後
	if (res) {	res &= callInitialTasks(m_width, m_height);	}
    m_updateRotation = false;
	return res;
}


void
CKLBGameApplication::initNMAsset() 
{
	if (!m_isNMAssetKeyDecrypted) {
		memset(m_decryptedBaseKey, 0, 34 * sizeof(int));
		for (int i = 0; i < 128; i++) {
			m_decryptedBaseKey[encryptedBaseKey[i] >> 5] |= 1 << (encryptedBaseKey[i] & 0x1F);
		}
		m_NMAssetKeyLen = m_decryptedBaseKey[0] ;
	}

	if (m_NMAssetKey) { free(m_NMAssetKey); }
	m_NMAssetKey = (char*)malloc(sizeof(char) * (m_NMAssetKeyLen + 1));
	klb_assert(m_NMAssetKey, "Failed to allocate memory");

	memset(m_NMAssetKey, 0, sizeof(char) * (m_NMAssetKeyLen + 1));
	for (int i = 0; i < m_NMAssetKeyLen; i++) {
		m_NMAssetKey[i] = (char)m_decryptedBaseKey[i + 1];
	}
}

char*
CKLBGameApplication::getNMAssetKey()
{
	return m_NMAssetKey;
}

int
CKLBGameApplication::getNMAssetKeyLen()
{
	return m_NMAssetKeyLen;
}

bool
CKLBGameApplication::frameFlip(u32 deltaT)
{
    if(m_updateRotation) {
        m_updateRotation = false;

        // changePointingMatrix(m_origin, m_width, m_height);
        // changeScreenMatrix(m_origin, m_width, m_height);
    }
	bool bContinue = CKLBTaskMgr::getInstance().execute(deltaT);
	if(m_reboot) {
		finishGame();
		initGame();
		m_reboot = false;
	}
	return bContinue;
}

s32 
CKLBGameApplication::getFrameTime() 
{
	return m_frameTime;
}

void 
CKLBGameApplication::setFrameTime(s32 time) 
{
	m_frameTime = time;
}

void
CKLBGameApplication::inputPoint(int id, IClientRequest::INPUT_TYPE type, int x, int y)
{
	int cx, cy;
	CKLBDrawResource::getInstance().convPointing(x, y, cx, cy);	// 座標をスケーリング率で変換
	CKLBTouchPadQueue::getInstance().addQueue(id, type, cx, cy);
}

void
CKLBGameApplication::inputDeviceKey(int keyId, char eventType)
{
    CKLBDeviceKeyEventQueue::getInstance().addQueue(keyId, eventType);
}

void
CKLBGameApplication::controlEvent(EVENT_TYPE type, IWidget * pWidget,
									size_t datasize1, void * pData1, size_t datasize2, void * pData2)
{
    CKLBOSCtrlQueue::getInstance().addQueue(type, pWidget, datasize1, pData1, datasize2, pData2);
}

bool
CKLBGameApplication::initLocalSystem(CKLBAssetManager& /*mgrAsset*/)
{
	return true;
}

bool
CKLBGameApplication::initSystem(AllocationSize* pSizes)
{
	//
	// 1. Load Asset (normally request from other asset should kick.
	//
	CKLBAssetManager& pAssetManager = CKLBAssetManager::getInstance();
	pAssetManager.init(pSizes->maxAssetCount, pSizes->dictionnaryNodePoolSize);	// 2012.12.11  コンストラクタから外して明示的に行うように(Reboot時に呼ばれない為)
    
	TexturePacker& p = TexturePacker::getInstance();
	if (!p.init(2048,512,STARTUP_FORMAT)) { // If change needed, please modify the STARTUP_FORMAT define, not the code here.
		return false;
	}

	//
	// OPTIMIZE TRICK : Should order the plugin registration from the least used to the most used.
	//

	CKLBCompositeAssetPlugin*
							pCompositePlugin= KLBNEW(CKLBCompositeAssetPlugin);
	KLBTextureAssetPlugin*	pTexturePlugin	= KLBNEW(KLBTextureAssetPlugin);
	KLBFlashAssetPlugin*	pFlashPlugin	= KLBNEW(KLBFlashAssetPlugin);
	KLBBlendAnimationAssetPlugin*
							pNodeAnimPlugin	= KLBNEW(KLBBlendAnimationAssetPlugin);
	KLBAudioAssetPlugin*	pAudioPlugin	= KLBNEW(KLBAudioAssetPlugin);

	if (pTexturePlugin && pFlashPlugin && pAudioPlugin) {
		pAssetManager.registerAssetPlugIn(pNodeAnimPlugin);
		pAssetManager.registerAssetPlugIn(pAudioPlugin);
		pAssetManager.registerAssetPlugIn(pFlashPlugin);
		pAssetManager.registerAssetPlugIn(pCompositePlugin);	// Form as second.
		pAssetManager.registerAssetPlugIn(pTexturePlugin);		// Register last because most used.
		return initLocalSystem(pAssetManager);
	} else {
		return false;
	}
}

bool
CKLBGameApplication::callInitialTasks(int width, int height)
{
	bool res;

	res  = (CKLBDrawTask::create(true, width, height) != NULL);
	res &= (CKLBTouchPad::create() != NULL);
	res &= (CKLBDeviceKeyEvent::create() != NULL);
    res &= (CKLBOSCtrlEvent::create() != NULL);
	res &= (CKLBTouchEventUITask::create() != NULL);
	res &= (MicroDownload::create() != NULL);
#ifdef DEBUG_MENU
    //
    // Was initialized in CKLBDebugMenu::Create but
    // wanted to be able to load font in LUA first
    // Before creating the menu.
    //
	CKLBDebugResource& dbgRes = CKLBDebugResource::getInstance();
	dbgRes.init();
#endif

	res &= (CKLBScriptEnv::getInstance().boot(m_bootFile) != false);

#ifdef DEBUG_MENU
	if (m_useDefaultFont) {
		res &= (CKLBDebugMenu::create() != NULL);
	}
#endif
    return res;
}
 
bool
CKLBGameApplication::initOther()
{
	return true;
}


void
CKLBGameApplication::changePointingMatrix(ORIGIN origin, int width, int height)
{
	/*
		引数のwidth/height は向きが0のときの幅/高さであるため、
        90/270度の場合は値を入れ替えて扱う必要がある。
	*/
    float pad_matrix[4][6] = {
        // 0[deg]
        {   1.0f,   0.0f,   0.0f,
            0.0f,   1.0f,   0.0f },
        
        // 90[deg]
        {   0.0f,   1.0f,  (float)height,
			-1.0f,   0.0f, 0.0f },
        
        // 180[deg]
        {   -1.0f,  0.0f,   (float)width,
            0.0f,   -1.0f,  (float)height },
        
        //270[deg]
        {   0.0f,   1.0f,   (float)height,
            -1.0f,  0.0f,   0.0f },
    };
    CKLBTouchPadQueue::getInstance().setConvertMatrix(pad_matrix[origin]);    
}

void
CKLBGameApplication::changeScreenMatrix(ORIGIN origin, int width, int height)
{
    GLfloat proj_matrix[4][16] = {
        // 0[deg]
        {   (2.0f/width),       0.0f,					0.0f,	0.0f,
            0.0f,               (-2.0f/height),         0.0f,	0.0f,
            0.0f,               0.0f,					0.0f,	0.0f,
            -1.0f,              1.0f,					1.0f,	1.0f, },
        
        // 90[deg]
        {   0.0f,               (2.0f/height),			0.0f,	0.0f,
            (2.0f/width),       0.0f,                   0.0f,	0.0f,
            0.0f,               0.0f,					0.0f,	0.0f,
            -1.0f,              -1.0f,					1.0f,	1.0f, },
        
        // 180[deg]
        {   (-2.0f/width),      0.0f,					0.0f,	0.0f,
            0.0f,               (2.0f/height),          0.0f,	0.0f,
            0.0f,               0.0f,					0.0f,	0.0f,
            1.0f,               -1.0f,					1.0f,	1.0f, },
        
        // 270[deg]
        {   0.0f,               (-2.0f/height),         0.0f,	0.0f,
            (-2.0f/width),      0.0f,                   0.0f,	0.0f,
            0.0f,               0.0f,					0.0f,	0.0f,
            1.0f,               1.0f,					1.0f,	1.0f, },
    };
    CKLBDrawResource::getInstance().changeProjectionMatrix(proj_matrix[origin], width, height);
}

bool
CKLBGameApplication::reportScreenRotation(ORIGIN /*origin*/, SCRMODE mode)
{
	int value = CKLBAppProperty::getInstance().getValue(CKLBAppProperty::SCRN_TYPE);
    if(value < 0) { return true; }  // 設定されていなければどっちでもいい

	// 設定されている値であればtrue, 違えば false
	return (mode == (SCRMODE)value);
}

void
CKLBGameApplication::changeScreenInfo(ORIGIN origin, int width, int height)
{
    // request from other therad. 
    m_width = width;
    m_height = height;
    m_origin = origin;
    m_updateRotation = true;
}

void
CKLBGameApplication::localFinish()
{
	// empty
}

/*virtual*/
void 
CKLBGameApplication::setupAllocation(AllocationSize* /*pStruct*/) 
{
	//
	// Default implementation does not and do not modify the parameters
	//
	// DO NOT MODIFY.
}

void
CKLBGameApplication::finishGame()
{
#ifdef DEBUG_MENU
	CKLBDebugResource::getInstance().release();
#endif

    CKLBTaskMgr::getInstance().clearTaskList();

	NetworkManager::stopNetworkManager();
	CKLBHTTPInterface::releaseHTTPLib();

	// project local system finish.
	localFinish();



	CKLBFormGroup::getInstance().release();
	CKLBLuaEnv::getInstance().finishLuaEnv();

	CKLBLabelNode::release();

	// Free DB Object if allocated.
	CKLBDatabase::getInstance().release();

	CKLBLanguageDatabase::getInstance().release();

	// Free Temporary global buffer.
	CKLBTextTempBuffer::freeBuffer();

	CKLBNetAPIKeyChain::getInstance().release();

	// Free all singleton in OUR desired order.
	// (final empty destruction of course done by CRT)
	TexturePacker::getInstance().release(); // Release Texture BEFORE Rendering Mgr
	CKLBRenderingManager::getInstance().release();
	CKLBAssetManager::getInstance().release();
	CKLBDataHandler::release();
	CKLBInnerDefManager::releaseManager();

	// KLBDELETEA(m_bootFile);	m_bootFile = NULL; // 2012.12.11  コメントアウト(Reboot時に再生成されない為)
	CKLBScriptEnv::getInstance().finishScriptEnv();
}

void
CKLBGameApplication::reboot()
{
	m_reboot = true;
}


void
CKLBGameApplication::resetViewport()
{
  CKLBDrawResource::getInstance().ResetViewport();
}

FILE* 
CKLBGameApplication::getShellOutput() 
{
	if (m_outStream) {
		return m_outStream;
	} else {
		return stdout;
	}
}

void 
CKLBGameApplication::setShellOutput(FILE* stream) 
{
	m_outStream = stream;
}
