
#include "DownloadManager.h"
#include <DownloadClient.h>

#define MAX_DOWNLOAD_THREAD 4

mutex DownloadManager::s_waiting;
mutex DownloadManager::s_threadCount;
mutex DownloadManager::s_thread;
mutex DownloadManager::s_callback;

DownloadManager::DownloadManager() :
    m_lastId(0),
    m_threadCount(0)
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

s32
DownloadManager::runNextTask(void* /*pThread*/, void* tid)
{
    DownloadManager::getInstance()->runNextTask(*(int*)tid);
}

void
DownloadManager::runNextTask(int tid)
{
    CKLBHTTPInterface* httpIF;
    httpIF = NetworkManager::createConnection();

    // alwasy looking for new task to execute
    while (true)
    {
        Task task;

        // try to fetch next task
        s_waiting.lock();
        if (!m_waiting.empty())
        {
            task = m_waiting.front();
            m_waiting.pop();
        }
        s_waiting.unlock();

        if (task.id == 0) break; // no new task

        // initialize download
        httpIF->reuse();
        httpIF->setDownload("file://external/tmp/1.zip"); //TODO: CHANGE ME
        httpIF->httpGET(task.url, false);

        // check download loop
        while (true)
        {
            // is the download finished
            bool bResult = httpIF->httpRECV();
            // Current downloaded size (may be inacurate)
            s64 size = httpIF->getDwnldSize();
            // Completly download size (accurate but updated at the end)
            s64 completeOnSize = httpIF->getSize();

            //TODO
        }

        //callCallbackSafely(task.id);
    }

    // at last, minus thread count
    s_threadCount.lock();
    bool allDone = --m_threadCount == 0;
    s_threadCount.unlock();

    // remove tid of this thread from m_thread
    s_thread.lock();
    m_thread.erase(tid);
    s_thread.unlock();
}

int 
DownloadManager::download(char* url, int size)
{
    DownloadManager::Task task;
    task.id = ++m_lastId;
    task.url = url;
    task.size = size;

    // put it in to the queue
    s_waiting.lock();
    m_waiting.push(task);
    s_waiting.unlock();

    // check thread count
    bool canCreateThread = false;

    s_threadCount.lock();
    if (m_threadCount < MAX_DOWNLOAD_THREAD)
    {
        canCreateThread = true;
        ++m_threadCount;
    }
    s_threadCount.unlock();

    if (canCreateThread)
    {
        // create new thread to execute the task
        s_thread.lock();
        // find a proper slot to insert this thread
        int tid;
        for (int i = 0; i < MAX_DOWNLOAD_THREAD; i++) {
            if (m_thread.find(i) == m_thread.end()) {
                tid = i;
                break;
            }
        }
        void* newThread = CPFInterface::getInstance().platform().createThread(runNextTask, &tid);
        m_thread[tid] = newThread;
        s_thread.unlock();
    }
    

    return task.id;
}

void
DownloadManager::callBackOnOneSuccess()
{
    s_callback.lock();

    s_callback.unlock();
}

void
DownloadManager::callBackOnAllSuccess()
{
    s_callback.lock();

    s_callback.unlock();
}

void
DownloadManager::callBackOnHttpError()
{
    s_callback.lock();

    s_callback.unlock();
}

DownloadManager::~DownloadManager()
{
    // kill all active threads
    s_thread.lock();
    for (int i = 0; i < MAX_DOWNLOAD_THREAD; i++) {
        if (m_thread.find(i) != m_thread.end()) {
            CPFInterface::getInstance().platform().deleteThread(m_thread[i]);
            m_thread.erase(i);
        }
    }
    s_thread.unlock();
}
