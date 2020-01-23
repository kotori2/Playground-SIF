
#ifndef DownloadManager_h
#define DownloadManager_h

#include "CKLBUtility.h"
#include "MultithreadedNetwork.h"
#include <string>
#include <queue>
#include <unordered_map>
#include <thread>
#include <mutex>

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
	int download(string url);
	virtual ~DownloadManager();
protected:
    struct Task
    {
        int id;
        string url;

        Task() : id(0) {}
    };
private:
    int m_lastId;

    queue<Task> m_waiting;
    static mutex s_waiting;

    unordered_map<int, thread> m_running;
    static mutex s_running;

    int m_threadCount;
    static thread s_threadCount;
    static mutex s_threadCountLock;

    void runNextTask();
};

#endif // DownloadManager_h
