
#include "DownloadClient.h"

enum {
	START_DL,
	RETRY_DL,
	REUNZIP
};

static IFactory::DEFCMD cmd[] = {
	// command constants
	{"START_DL",	START_DL	},
	{"RETRY_DL",	RETRY_DL	},
	{"REUNZIP",		REUNZIP		},

	// error contants
	{"CKLBUPDATE_DOWNLOAD_FORBIDDEN",		CKLBUPDATE_DOWNLOAD_FORBIDDEN		},
	{"CKLBUPDATE_DOWNLOAD_INVALID_SIZE",	CKLBUPDATE_DOWNLOAD_INVALID_SIZE	},
	{"CKLBUPDATE_DOWNLOAD_NODATA",			CKLBUPDATE_DOWNLOAD_NODATA			},
	{"CKLBUPDATE_DOWNLOAD_ERROR",			CKLBUPDATE_DOWNLOAD_ERROR			},
	{"CKLBUPDATE_UNZIP_ERROR",				CKLBUPDATE_UNZIP_ERROR				},
	{0, 0}
};

static CKLBTaskFactory<DownloadClient> factory("DownloadClient", CLS_DOWNLOADCLIENT, cmd);

DownloadClient::DownloadClient() 
: CKLBLuaTask()
, m_callbackDownloadFinish(NULL)
, m_callbackUnzipStart(NULL)
, m_callbackUnzipFinish(NULL)
, m_callbackProgress(NULL)
, m_callbackFinish(NULL)
, m_callbackError(NULL)
, m_callbackKbps(NULL)
, m_downloadedCount(0)
, m_unzippedCount(0)
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
	int argc = lua.numArgs();
	int cmd = lua.getInt(2);

	switch (cmd) {
	case START_DL: {
		//
		// 3. pipeline count
		// 4. table { status, queue_id, url, size }
		// 5. timeout
		//
		klb_assert(argc == 5, "Arguments count should be 5!")
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
	default:
		klb_assertAlways("DownloadClient: unknown command passed, %d", cmd);
		lua.retBoolean(false);
	}
	return 1;
}

int
DownloadClient::startDownload(CLuaState& lua)
{
	IPlatformRequest& platform	= CPFInterface::getInstance().platform();
	DownloadManager* manager	= DownloadManager::getInstance(this);

	// erase temp folder
	platform.removeFileOrFolder("file://external/tmpDL/");
	// TODO: create folder if not exists

	// create queue task
	createQueue(lua);

	int pipeline = lua.getInt(3);
	for (int i = 0; i < m_queue.total; i++) 
	{
		int taskId = manager->download(m_queue.urls[i], m_queue.size[i], m_queue.queueIds[i]);
		m_queue.taskIds[i] = taskId;
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

void
DownloadClient::allSuccessCallback()
{
	// do nothing for now
}

void
DownloadClient::oneSuccessCallback(int queueId)
{
	m_downloadedCount++;
	// 4. downloaded
	// 5. unzipped
	CKLBScriptEnv::getInstance().call_eventUpdateProgress(m_callbackError, this, m_downloadedCount, m_unzippedCount);
	CKLBScriptEnv::getInstance().call_eventUpdateDownload(m_callbackError, this, queueId);
}

void
DownloadClient::httpFailureCallback(int statusCode)
{
	// kill threads
	// TODO: Double free if lots of thread got error at the same time
	DownloadManager* instance = DownloadManager::getInstance(this);
	delete instance;

	// 1. error code
	// 2. http status code
	// 3. TODO: curl status
	CKLBScriptEnv::getInstance().call_eventUpdateError(m_callbackError, this, CKLBUPDATE_DOWNLOAD_ERROR, statusCode, 0);
}

void
DownloadClient::createQueue(CLuaState& lua)
{
	// tested only on START_DL command
	// TODO: reset queue

	// start filling task queue
	u32 json_size = 0;
	const char* json = NULL;
	lua.retValue(4);
	json = CKLBUtility::lua2json(lua, json_size);
	lua.pop(1);
	CKLBJsonItem* pRoot = CKLBJsonItem::ReadJsonData(json, json_size);

	CKLBJsonItem* item = pRoot->child();
	do {
		strcpy(m_queue.urls[m_queue.total], item->searchChild("url")->getString());
		m_queue.size[m_queue.total] = item->searchChild("size")->getInt();
		m_queue.queueIds[m_queue.total] = item->searchChild("queue_id")->getInt();
		m_queue.total++;
	} while (item = item->next());

	KLBDELETE(pRoot);
	KLBDELETE(item);
}