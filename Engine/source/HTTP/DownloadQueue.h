#ifndef DownloadQueue_h
#define DownloadQueue_h
#include "CKLBLuaTask.h"
#include <vector>
#include "CKLBHTTPInterface.h"
#include "MultithreadedNetwork.h"

struct MDLData
{
	const char* url;
	const char* callback;
	const char* filename;
	CKLBHTTPInterface* http;

	MDLData(const char* c, const char* f, const char* u)
	{
		url = CKLBUtility::copyString(u);
		callback = CKLBUtility::copyString(c);
		filename = CKLBUtility::copyString(f);
		http = NetworkManager::createConnection();

		http->httpGET(u, false);
	};

	~MDLData()
	{
		KLBDELETEA(url);
		KLBDELETEA(callback);
		KLBDELETEA(filename);

		NetworkManager::releaseConnection(http);
	};
};

typedef std::vector<MDLData*> MicroDLQueue;

class MicroDownload : public CKLBLuaTask
{
friend class CKLBTaskFactory<MicroDownload>;
public:
	u32 getClassID();
	void execute(u32 deltaT);
	void die();
	static MicroDownload* create();
	
	static void Queue(const char* callback, const char* file_target, const char* download_url);
	static void DeleteAll();
	void MainLoop(int deltaT);
private:
	bool initScript(CLuaState& lua);
	static MicroDLQueue queue_list;
};
#endif