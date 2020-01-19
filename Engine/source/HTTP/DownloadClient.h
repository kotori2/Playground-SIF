
#ifndef DownloadClient_h
#define DownloadClient_h

#include "CKLBLuaTask.h"
#include "CKLBUtility.h"
#include "DownloadManager.h"

// error codes
enum {
	CKLBUPDATE_DOWNLOAD_FORBIDDEN,
	CKLBUPDATE_DOWNLOAD_INVALID_SIZE,
	CKLBUPDATE_DOWNLOAD_NODATA,
	CKLBUPDATE_DOWNLOAD_ERROR,
	CKLBUPDATE_UNZIP_ERROR
};

class DownloadClient : public CKLBLuaTask
{
	friend class CKLBTaskFactory<DownloadClient>;
private:
	DownloadClient();
	virtual ~DownloadClient();

public:
	virtual u32 getClassID();
	static DownloadClient* create();
	void execute(u32 deltaT);
	void die();

	bool initScript(CLuaState& lua);
	int commandScript(CLuaState& lua);

	int startDownload(CLuaState& lua);
	int retryDownload(CLuaState& lua);

private:
	typedef struct DOWNLOAD_QUEUE {
		int total;
		char urls[4096][256];
		int size[4096];
		DOWNLOAD_QUEUE() {
			total = 0;
			size[4096] = 0;
		}
	};

	typedef struct DOWNLOAD_STATUS {
		int dlStarted;
		int downloaded;
		int unzipped;
		DOWNLOAD_STATUS() {
			dlStarted = 0;
			downloaded = 0;
			unzipped = 0;
		}
	};

	int					m_pipeLine;
	DOWNLOAD_STATUS		m_status;
	DOWNLOAD_QUEUE		m_queue;


	s32					workThread(void* pThread);
	static s32			threadFunc(void* pThread, void* data);

	// lua callbacks
	const char*		m_callbackDownloadFinish;
	const char*		m_callbackUnzipStart;
	const char*		m_callbackUnzipFinish;
	const char*		m_callbackProgress;
	const char*		m_callbackFinish;
	const char*		m_callbackError;
	const char*		m_callbackKbps;
};

#endif