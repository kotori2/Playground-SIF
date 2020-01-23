
#include "DownloadManager.h"
#include <DownloadClient.h>

#define MAX_DOWNLOAD_THREAD 4

DownloadManager::DownloadManager() :
    m_lastId(0)
{

}

DownloadManager* DownloadManager::getInstance()
{
    static DownloadManager* s_instance = NULL;

    if (s_instance == NULL)
    {
        s_instance = new DownloadManager;
    }

    return s_instance;
}

void
DownloadManager::runNextTask()
{
    //TODO
}

int 
DownloadManager::download(string url)
{
    DownloadManager::Task task;
    task.id = ++m_lastId;
    task.url = url;

    // put it in to the queue
    s_waiting.lock();
    m_waiting.push(task);
    s_waiting.unlock();

    // check thread count
    bool canCreateThread = false;

    s_threadCountLock.lock();
    if (m_threadCount < MAX_DOWNLOAD_THREAD)
    {
        canCreateThread = true;
        ++m_threadCount;
    }
    s_threadCountLock.unlock();

    if (canCreateThread)
    {
        // create new thread to execute the task
        thread new_thread = thread(&DownloadManager::runNextTask, this);
    }

    return task.id;
}

DownloadManager::~DownloadManager()
{

}
