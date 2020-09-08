
#include "DownloadClient.h"
#include "CKLBLuaScript.h"
#include "CUnZip.h"

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
	{"CKLBUPDATE_DOWNLOAD_NODATA",			CKLBUPDATE_DOWNLOAD_NODATA			}, // NOT USED as of 2020/1/27
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
, m_executeCount(0)
, m_queue()
, m_unzipThread(NULL)
, m_isFinished(false)
, m_isError(false)
{
	auto &pfif = CPFInterface::getInstance().platform();
	m_mutex = pfif.allocMutex();
}

DownloadClient::~DownloadClient()
{
	this->killAllThreads();
	auto& pfif = CPFInterface::getInstance().platform();
	pfif.freeMutex(m_mutex);
}

void
DownloadClient::killAllThreads()
{
	if (m_unzipThread) {
		CPFInterface::getInstance().platform().breakThread(m_unzipThread);
		CPFInterface::getInstance().platform().deleteThread(m_unzipThread);
		m_unzipThread = NULL;
	}
	if(DownloadManager::s_instance) KLBDELETE(DownloadManager::getInstance(this));
}

u32
DownloadClient::getClassID()
{
	return CLS_DOWNLOADCLIENT;
}

void
DownloadClient::execute(u32 deltaT)
{
	// update status every 20 frames
	m_executeCount++;
	if (m_executeCount >= 20) {
		m_executeCount = 0;
		
		CKLBLuaScript::enqueue([=]() {
			DownloadManager* instance = DownloadManager::getInstance(this);
			double speed = instance->getTotalSpeed();
			CKLBScriptEnv::getInstance().call_eventUpdateKbps(m_callbackKbps, this, 0, speed);
		});
		
		// progress should be called here to prevent
		// your network is too fast and downloaded a
		// file before download stage appear
		CKLBLuaScript::enqueue([=]() {
			CKLBScriptEnv::getInstance().call_eventUpdateProgress(m_callbackProgress, this, m_downloadedCount, m_unzippedCount);
		});
        return;
	}
    
    if (m_isFinished && m_callback_queue.empty()) {
        // Both download and unzip finished

		CKLBLuaScript::enqueue([=]() {
			CKLBScriptEnv::getInstance().call_eventUpdateProgress(m_callbackProgress, NULL, m_downloadedCount, m_unzippedCount);
		});
		CKLBLuaScript::enqueue([=]() {
			CKLBScriptEnv::getInstance().call_eventUpdateComplete(m_callbackFinish, NULL);
		});
		m_isFinished = false;
        return;
    }
    
	auto& pfif = CPFInterface::getInstance().platform();
	pfif.mutexLock(m_mutex);
	if (m_callback_queue.empty()) {
		pfif.mutexUnlock(m_mutex);
		return;
	}
    DOWNLOAD_CALLBACK_ITEM q = m_callback_queue.front();
    m_callback_queue.pop();
	pfif.mutexUnlock(m_mutex);
    
    switch(q.type){
        case DOWNLOAD_CLIENT_CALLBACK_ERROR:
            // 1. error code
            // 2. http status code
            // 3. TODO: curl status
            CKLBScriptEnv::getInstance().call_eventUpdateError(m_callbackError, this, q.errorType, q.errorCode, 0);
            
            // make sure all threads was killed
            killAllThreads();
            break;
        case DOWNLOAD_CLIENT_CALLBACK_DOWNLOAD_FINISH:
            CKLBScriptEnv::getInstance().call_eventUpdateDownload(m_callbackDownloadFinish, this, q.queueId);
            break;
        case DOWNLOAD_CLIENT_CALLBACK_UNZIP_START:
            CKLBScriptEnv::getInstance().call_eventUpdateUnzipStart(this->m_callbackUnzipStart, this, q.queueId);
            break;
        case DOWNLOAD_CLIENT_CALLBACK_UNZIP_FINISH:
            CKLBScriptEnv::getInstance().call_eventUpdateUnzipEnd(this->m_callbackUnzipFinish, this, q.queueId);
            break;
        default:
            klb_assertAlways("Wrong download queue callback type");
    }
}

void
DownloadClient::die()
{
	this->killAllThreads();
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
	klb_assert(lua.numArgs() == 7, "No valid amount of args for DownloadClient");
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
		//
		// 3. pipeline count
		//
		klb_assert(argc == 3, "Arguments count should be 3!")
		retryDownload(lua);
		break;
	}
	case REUNZIP: {
		klb_assert(argc == 4, "Arguments count should be 4!")
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
	//IPlatformRequest& platform	= CPFInterface::getInstance().platform();
	DownloadManager* manager	= DownloadManager::getInstance(this);
	
	// create queue task
	createQueue(lua, false);
	//int pipeline = lua.getInt(3);
	for (int i = 0; i < m_queue.total; i++) 
	{
		if (!m_queue.downloaded[i]) {
			int taskId = manager->download(m_queue.urls[i], m_queue.size[i], m_queue.queueIds[i]);
			m_queue.taskIds[i] = taskId;
		}
	}
	m_unzipThread = CPFInterface::getInstance().platform().createThread(unzipThread, this);
	return 1;
}

int
DownloadClient::retryDownload(CLuaState& lua)
{
	// press retry if error occured.
	// no pipeline passed so we should re-use our old queue
	DownloadManager* manager = DownloadManager::getInstance(this);
	for (int i = 0; i < m_queue.total; i++)
	{
		if (!m_queue.downloaded[i]) {
			int taskId = manager->download(m_queue.urls[i], m_queue.size[i], m_queue.queueIds[i]);
			m_queue.taskIds[i] = taskId;
		}
	}
	m_unzipThread = CPFInterface::getInstance().platform().createThread(unzipThread, this);
	return 1;
}

int
DownloadClient::reUnzip(CLuaState& lua)
{
	// if download is finished, file unzipping and
	// client crashed or terminated, client will call 
	// this at boot
	IPlatformRequest& platform = CPFInterface::getInstance().platform();
	// create queue task
	createQueue(lua, true);
	m_unzipThread = platform.createThread(unzipThread, this);
	return 1;
}

// Both download and unzip finished
// Since every download should end with unzip,
// we call this only after unzip

void
DownloadClient::oneSuccessCallback(int queueId)
{
	m_downloadedCount++;
	for (int i = 0; i < m_queue.total; i++) {
		if (m_queue.queueIds[i] == queueId) {
			m_queue.downloaded[i] = true;
            break;
		}
	}
    DOWNLOAD_CALLBACK_ITEM q;
    q.type = DOWNLOAD_CLIENT_CALLBACK_DOWNLOAD_FINISH;
    q.queueId = queueId;
    m_callback_queue.push(q);
}

void
DownloadClient::httpFailureCallback(int statusCode, int errorType)
{
	if (m_isError) { return; }
    DOWNLOAD_CALLBACK_ITEM q;
    q.type = DOWNLOAD_CLIENT_CALLBACK_ERROR;
    q.errorCode = statusCode;
    q.errorType = errorType;
    m_callback_queue.push(q);
}

void
DownloadClient::createQueue(CLuaState& lua, bool isReUnzip)
{
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
		// we need to re-download if status == 2 because that means unzip
		// failes at start up
		// 2020/1/26 but seems klab will not pass them in lua, there is a
		// bug in lua code. manually change database atm
		if (isReUnzip) {
			m_queue.downloaded[m_queue.total] = item->searchChild("status")->getInt() >= 1;
		} else {
			m_queue.downloaded[m_queue.total] = item->searchChild("status")->getInt() == 1;
		}
		
		m_queue.unzipped[m_queue.total] = false;
		m_queue.total++;
	} while ((item = item->next()));

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
            
            DOWNLOAD_CALLBACK_ITEM q;
            q.type = DOWNLOAD_CLIENT_CALLBACK_UNZIP_START;
            q.queueId = that->m_queue.queueIds[i];
            that->m_callback_queue.push(q);
            
			size_t zipEntry = 0;
			char zipPath[64];
			sprintf(zipPath, "file://external/tmpDL/%d.zip", that->m_queue.queueIds[i]); // TODO: Maybe place the format stuffs in somewhere else?
			IPlatformRequest& platform = CPFInterface::getInstance().platform();
            const char* zipPathFull = platform.getFullPath(zipPath);
            CUnZip* unzip = KLBNEWC(CUnZip, (zipPathFull));

			// wait for file write
			for (int j = 0; j < 10; j++) {
				if (!unzip->getStatus()) {
					// use this for now since i don't want to add platfrom code
					// 32ms is about 2 frames
					std::this_thread::sleep_for(std::chrono::milliseconds(32));
					unzip->Open(zipPathFull);
				} else {
					break;
				}
			}

			if (!unzip->getStatus()) {	// invalid zip file
                DOWNLOAD_CALLBACK_ITEM q;
                q.type = DOWNLOAD_CLIENT_CALLBACK_ERROR;
                q.errorType = CKLBUPDATE_UNZIP_ERROR;
                q.errorCode = 1;
                that->m_callback_queue.push(q);
                
				DEBUG_PRINT("[update] invalid zip file");
				return 0;
			}

			zipEntry = unzip->numEntry();

			for (int i = 0; i < zipEntry; i++) {
				if (unzip->readCurrentFileInfo()) {
					unzip->extractCurrentFile("file://external/");
				}

				// actually unzip will immediately finish after extractCurrentFile
				// call isFinishExtract is going to close file ptr etc
				while (!unzip->isFinishExtract()) {
					// wait for finish
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}

				bool bResult = unzip->gotoNextFile();
				if (!bResult && i + 1 != zipEntry) {
					DEBUG_PRINT("gotoNextFile=false %d/%d", i, zipEntry);
					break;
				}
			}

			// ALL files in zip was extracted
			KLBDELETE(unzip);
			unzip = NULL;
			// delete tmp zip file
			char pfPath[80];
			sprintf(pfPath, "file://%s", zipPath);
			CPFInterface::getInstance().platform().removeTmpFile(pfPath);
            
            q.type = DOWNLOAD_CLIENT_CALLBACK_UNZIP_FINISH;
            q.queueId = that->m_queue.queueIds[i];
            that->m_callback_queue.push(q);
            
			that->m_unzippedCount++;
			that->m_queue.unzipped[i] = true;
			DEBUG_PRINT("Unzipping %d finished", that->m_queue.queueIds[i]);
			
		} else {
			std::this_thread::sleep_for(std::chrono::milliseconds(32));
		}
	}
	// empty queue
	that->m_queue.total = 0;
	that->m_isFinished = true;
	return 0;
}
