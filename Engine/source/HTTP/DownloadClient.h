
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

	bool initScript		(CLuaState& lua);
	int commandScript	(CLuaState& lua);

	int startDownload	(CLuaState& lua);
	int retryDownload	(CLuaState& lua);
	int reUnzip			(CLuaState& lua);

	void httpFailureCallback(int statusCode);

private:
	typedef struct DOWNLOAD_QUEUE {
		int total;
		char urls[4096][256];
		int size[4096];
	};
	DOWNLOAD_QUEUE		m_queue;

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