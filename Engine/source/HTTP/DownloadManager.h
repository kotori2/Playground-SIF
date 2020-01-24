
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

using namespace std;
/*
	DownloadManager class allows to download multiple files at the time (TODO)
    Some code copied from https://github.com/hivivo/Downloader
*/
class DownloadManager
{
public:
	DownloadManager();
	static DownloadManager* getInstance();
	int download(char* url, int size, int queueId);
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
    int m_lastId;

    queue<Task> m_waiting;
    static mutex s_waiting;

    int m_threadCount;
    static mutex s_threadCount;

    map<int, void*> m_thread;
    static mutex s_thread;

    static mutex s_callback;

    void runNextTask(int tid);
    static s32 runNextTask(void* /*pThread*/, void* data);

    void callBackOnOneSuccess();
    void callBackOnAllSuccess();
    void callBackOnHttpError();
};

#endif // DownloadManager_h
