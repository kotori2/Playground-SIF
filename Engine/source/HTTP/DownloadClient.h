
#ifndef DownloadClient_h
#define DownloadClient_h

#include "CKLBLuaTask.h"
#include "CKLBUtility.h"
#include "DownloadManager.h"

#define MAX_DOWNLOAD_QUEUE 4096

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

	bool initScript		(CLuaState& lua);
	int commandScript	(CLuaState& lua);

	int startDownload	(CLuaState& lua);
	int retryDownload	(CLuaState& lua);
	int reUnzip			(CLuaState& lua);

	void allSuccessCallback();
	void oneSuccessCallback(int queueId);
	void httpFailureCallback(int statusCode);

	static s32 unzipThread(void* /*pThread*/, void* instance);

private:
	void createQueue	(CLuaState& lua);

	typedef struct DOWNLOAD_QUEUE {
		int total;
		// 4096 is max for now
		char urls[MAX_DOWNLOAD_QUEUE][2048];
		int size[MAX_DOWNLOAD_QUEUE];
		int	queueIds[MAX_DOWNLOAD_QUEUE];
		int	taskIds[MAX_DOWNLOAD_QUEUE];
		bool downloaded[MAX_DOWNLOAD_QUEUE];
		bool unzipped[MAX_DOWNLOAD_QUEUE];
		DOWNLOAD_QUEUE() : total(0), urls(), size(), queueIds(), 
			taskIds(), downloaded(), unzipped() {}
	};
	DOWNLOAD_QUEUE		m_queue;

	int m_downloadedCount;
	int m_unzippedCount;

	int m_executeCount;
	void* m_unzipThread;

	// lua callbacks
	const char*			m_callbackDownloadFinish;
	const char*			m_callbackUnzipStart;
	const char*			m_callbackUnzipFinish;
	const char*			m_callbackProgress;
	const char*			m_callbackFinish;
	const char*			m_callbackError;
	const char*			m_callbackKbps;
};

#endif