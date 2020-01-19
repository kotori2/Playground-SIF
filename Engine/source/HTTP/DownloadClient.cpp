
#include "DownloadClient.h"
#include "DownloadManager.h"
#include "CUnZip.h"
#include "CKLBLuaEnv.h"
#include "CKLBUtility.h"
#include "CKLBUpdate.h"
#include "map"

// command constants
enum {
	START_DL,
	RETRY_DL,
	REUNZIP
};

static IFactory::DEFCMD cmd[] = {
	{"START_DL",	START_DL},
	{"RETRY_DL",	RETRY_DL},
	{"REUNZIP",		REUNZIP},
	{0, 0}
};

static CKLBTaskFactory<DownloadClient> factory("DownloadClient", CLS_DOWNLOADCLIENT, cmd);

static std::map<void*, int> ThreadList;

DownloadClient::DownloadClient() 
: CKLBLuaTask()
, m_pipeLine (0)
, m_callbackDownloadFinish(NULL)
, m_callbackUnzipStart(NULL)
, m_callbackUnzipFinish(NULL)
, m_callbackProgress(NULL)
, m_callbackFinish(NULL)
, m_callbackError(NULL)
, m_callbackKbps(NULL)
, m_status()
, m_queue()
{

}

DownloadClient::~DownloadClient()
{
	// done in die
}

u32
DownloadClient::getClassID()
{
	return CLS_DOWNLOADCLIENT;
}

void
DownloadClient::execute(u32 deltaT)
{
	CKLBScriptEnv& sEnv = CKLBScriptEnv::getInstance();
	if (m_status.downloaded == m_status.unzipped && m_status.unzipped == m_queue.total) {
		sEnv.call_eventUpdateComplete(m_callbackFinish, this);
		return;
	} 
	sEnv.call_eventUpdateProgress(m_callbackProgress, this, m_status.downloaded, m_status.unzipped);
}

void
DownloadClient::die()
{
	KLBDELETEA(m_callbackDownloadFinish);
	KLBDELETEA(m_callbackUnzipStart);
	KLBDELETEA(m_callbackUnzipFinish);
	KLBDELETEA(m_callbackProgress);
	KLBDELETEA(m_callbackFinish);
	KLBDELETEA(m_callbackError);
	KLBDELETEA(m_callbackKbps);
}

DownloadClient*
DownloadClient::create()
{
	DownloadClient* Task = KLBNEW(DownloadClient);
	if (!Task) return NULL;
	return Task;
}

bool
DownloadClient::initScript(CLuaState& lua)
{
	const char* callbackDownloadFinish	= lua.getString(1);
	const char* callbackUnzipStart		= lua.getString(2);
	const char* callbackUnzipFinish		= lua.getString(3);
	const char* callbackProgress		= lua.getString(4);
	const char* callbackFinish			= lua.getString(5);
	const char* callbackError			= lua.getString(6);
	const char* callbackKbps			= lua.getString(7);

	m_callbackDownloadFinish	= CKLBUtility::copyString(callbackDownloadFinish);
	m_callbackUnzipStart		= CKLBUtility::copyString(callbackUnzipStart);
	m_callbackUnzipFinish		= CKLBUtility::copyString(callbackUnzipFinish);
	m_callbackProgress			= CKLBUtility::copyString(callbackProgress);
	m_callbackFinish			= CKLBUtility::copyString(callbackFinish);
	m_callbackError				= CKLBUtility::copyString(callbackError);
	m_callbackKbps				= CKLBUtility::copyString(callbackKbps);

	return regist(NULL, P_INPUT);
}

int
DownloadClient::commandScript(CLuaState& lua)
{
	int cmd = lua.getInt(2);
	switch (cmd) {
	case START_DL: {
		startDownload(lua);
		break;
	}
	case RETRY_DL: {
		retryDownload(lua);
		break;
	}
	case REUNZIP: {
		reUnzip(lua);
		break;
	}
	default: {
		klb_assertAlways("DownloadClient: unknown command passed, %d", cmd);
		lua.retBoolean(false);
	}
	}
	return 1;
}

int
DownloadClient::startDownload(CLuaState& lua)
{
	// 3. pipeline
	// 4. table { status, queue_id, url, size }
	// 5. timeout
	IPlatformRequest& platform = CPFInterface::getInstance().platform();
	int argc = lua.numArgs();
	if (argc != 5) return 0;

	m_pipeLine = lua.getInt(3);
	int timeout = lua.getInt(5);
	klb_assert(m_pipeLine >= 1, "DownloadClient: need at least 1 pipe for download");

	u32 json_size = 0;
	const char* json = NULL;
	lua.retValue(4);
	json = CKLBUtility::lua2json(lua, json_size);
	lua.pop(1);
	CKLBJsonItem* pRoot = CKLBJsonItem::ReadJsonData(json, json_size);

	CKLBJsonItem* item = pRoot->child();
	do {
		// TODO: check status
		strcpy(m_queue.urls[m_queue.total], item->searchChild("url")->getString());
		m_queue.size[m_queue.total] = item->searchChild("size")->getInt();
		m_queue.total++;
	} while (item = item->next());
	
	KLBDELETE(pRoot);
	KLBDELETE(item);

	// clear download temp folder
	platform.removeFileOrFolder("file://external/tmpDL/");

	// TODO: create temp folder if not exists

	// start download
	for (int i = 0; i < m_pipeLine; i++) {
		void* thread = platform.createThread(threadFunc, this);
		ThreadList[thread] = i;
	}

	return 1;
}

int
DownloadClient::retryDownload(CLuaState& lua)
{
	lua.printStack();
	klb_assertAlways("Not implemented yet");
	return 0;
}

int
DownloadClient::reUnzip(CLuaState& lua)
{
	lua.printStack();
	klb_assertAlways("Not implemented yet");
	return 0;
}

/*static*/
s32
DownloadClient::threadFunc(void* pThread, void* data)
{
	return ((DownloadClient*)data)->workThread(pThread);
}

s32
DownloadClient::workThread(void* pThread)
{

	IPlatformRequest& platform = CPFInterface::getInstance().platform();
	CKLBScriptEnv& sEnv = CKLBScriptEnv::getInstance();
	if (m_status.dlStarted == m_queue.total) {
		platform.deleteThread(pThread);
		return 1;
	}
	m_status.dlStarted++;
	int num = m_status.dlStarted - 1;

	char filePath[256];
	sprintf(filePath, "file://external/tmpDL/%d.zip", num + 1);

	DownloadManager* mgr = KLBNEWC(DownloadManager, (m_queue.urls[num], filePath, m_queue.size[num]));
	while (!mgr->isFailed() && !mgr->isSuccess()) {
		mgr->execute();
	}
	if (mgr->isSuccess()) {
		m_status.downloaded++;
		DEBUG_PRINT("Download %d of %d success", m_status.downloaded, m_queue.total);
		sEnv.call_eventUpdateDownload(m_callbackDownloadFinish, this, num + 1);

	}
	else {
		DEBUG_PRINT("Download queue id %d failed", num + 1);
		sEnv.call_eventUpdateError(m_callbackError, this, mgr->getErrorCode(), mgr->getStatusCode(), 0);
	}
	KLBDELETE(mgr);

	workThread(pThread);
	return 1;
}