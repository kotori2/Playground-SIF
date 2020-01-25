
#ifndef DownloadManager_h
#define DownloadManager_h

#include "CKLBUtility.h"
#include "MultithreadedNetwork.h"
#include <string>
#include <queue>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <map>

#define MAX_DOWNLOAD_THREAD 4
/*
	DownloadManager class allows to download multiple files at the time (TODO)
    Some code copied from https://github.com/hivivo/Downloader
*/
class DownloadClient;
class DownloadManager
{
public:
	DownloadManager();
	static DownloadManager* getInstance(DownloadClient* downloadClient);
	int download(char* url, int size, int queueId);
    double getTotalSpeed();
	virtual ~DownloadManager();
protected:
    struct Task
    {
        int id;
        int queueId;
        int size;
        char* url;

        Task() : id(0), queueId(0), size(0), url("\0") {}
    };
private:
    DownloadClient* m_downloadClient;
    int m_lastId;
    bool m_isError;

    std::queue<Task> m_waiting;
    static std::mutex s_waiting;

    int m_threadCount;
    static std::mutex s_threadCount;

    std::map<int, void*> m_thread;
    static std::mutex s_thread;

    std::map<int, double> m_speed;

    void runNextTask(int tid);
    static s32 runNextTask(void* /*pThread*/, void* data);
    void callBackOnHttpError(int statusCode);
};

#endif // DownloadManager_h
