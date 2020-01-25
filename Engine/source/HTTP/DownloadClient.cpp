
#include "DownloadClient.h"
#include <CUnZip.h>

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
	// TODO: !!!!! we should not erase the folder because there could be
	// some unfinished zips
	// but idk when should we erase it.
	platform.removeFileOrFolder("file://external/tmpDL/");
	
	// create queue task
	createQueue(lua);
	int pipeline = lua.getInt(3);
	for (int i = 0; i < m_queue.total; i++) 
	{
		int taskId = manager->download(m_queue.urls[i], m_queue.size[i], m_queue.queueIds[i]);
		m_queue.taskIds[i] = taskId;
	}
	CPFInterface::getInstance().platform().createThread(unzipThread, this);
	return 1;
}

int
DownloadClient::retryDownload(CLuaState& lua)
{
	// if error occured and downloaded zip is more then
	// a threshold (4 currently), client will call this
	// instead of re-download all zips
	lua.printStack();
	klb_assertAlways("Not implemented yet");
	return 0;
}

int
DownloadClient::reUnzip(CLuaState& lua)
{
	// if all download are finished and
	// client crashed or terminated, 
	// client will call this
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
	for (int i = 0; i < m_queue.total; i++) {
		if (m_queue.queueIds[i] == queueId) {
			m_queue.downloaded[i] = true;
		}
	}
	// 4. downloaded
	// 5. unzipped
	CKLBScriptEnv::getInstance().call_eventUpdateProgress(m_callbackProgress, this, m_downloadedCount, m_unzippedCount);
	CKLBScriptEnv::getInstance().call_eventUpdateDownload(m_callbackDownloadFinish, this, queueId);
}

void
DownloadClient::httpFailureCallback(int statusCode)
{
	// TODO: kill all threads

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
		m_queue.downloaded[m_queue.total] = item->searchChild("status")->getInt() == 1;
		m_queue.unzipped[m_queue.total] = false;
		m_queue.total++;
	} while (item = item->next());

	KLBDELETE(pRoot);
	KLBDELETE(item);
}

s32
DownloadClient::unzipThread(void* /*pThread*/, void* instance)
{
	DownloadClient* that = (DownloadClient*)instance;
	// check if there is a downloaded file to unzip
	while (that->m_unzippedCount < that->m_queue.total) {
		int i = that->m_unzippedCount;
		// start unzip
		if (that->m_queue.downloaded[i]) {
			DEBUG_PRINT("Start unzipping %d", that->m_queue.queueIds[i]);
			int zipEntry = 0;
			char zipPath[64];
			sprintf(zipPath, "external/tmpDL/%d.zip", that->m_queue.queueIds[i]); // TODO: Maybe place the format stuffs in somewhere else?
			CUnZip* unzip = KLBNEWC(CUnZip, (zipPath));

			// wait for file write
			for (int j = 0; j < 10; j++) {
				if (!unzip->getStatus()) {
					// use this for now since i don't want to add platfrom code
					// 32ms is about 2 frames
					std::this_thread::sleep_for(std::chrono::milliseconds(32));
					unzip->Open(zipPath);
				} else {
					break;
				}
			}

			if (!unzip->getStatus()) {	// invalid zip file
				CKLBScriptEnv::getInstance().call_eventUpdateError(that->m_callbackError, that, CKLBUPDATE_UNZIP_ERROR, 1, 0);
				DEBUG_PRINT("[update] invalid zip file");
				// TODO: how to handle thread after call an error?
				return 0;
			}

			zipEntry = unzip->numEntry();
			while (true) {
				// finish extracting CURRENT file in zip
				if (unzip->isFinishExtract()) {
					bool bResult = unzip->gotoNextFile();
					// ALL files in zip was extracted
					if (!bResult) {
						KLBDELETE(unzip);
						unzip = NULL;
						// delete tmp zip file
						// CPFInterface::getInstance().platform().removeTmpFile(zipPath);
						CKLBScriptEnv::getInstance().call_eventUpdateUnzipEnd(that->m_callbackUnzipFinish, that, that->m_queue.queueIds[i]);
						that->m_unzippedCount++;
						DEBUG_PRINT("Unzipping %d finished", that->m_queue.queueIds[i]);
						break;
					} else {
						if (unzip->readCurrentFileInfo()) {
							unzip->extractCurrentFile("file://external/");
						}
					}
				}

			}
		}
	}
	return 0;
}