
#include "DownloadManager.h"
#include <DownloadClient.h>

std::mutex DownloadManager::s_waiting;
std::mutex DownloadManager::s_threadCount;
std::mutex DownloadManager::s_thread;

DownloadManager::DownloadManager() :
    m_lastId(0),
    m_threadCount(0),
    m_downloadClient(NULL),
    m_isError(false)
{

}

DownloadManager* 
DownloadManager::getInstance(DownloadClient* downloadClient)
{
    static DownloadManager* s_instance = NULL;

    if (s_instance == NULL)
    {
        s_instance = new DownloadManager;
    }

    if (downloadClient != NULL) {
        s_instance->m_downloadClient = downloadClient;
    }

    return s_instance;
}

s32
DownloadManager::runNextTask(void* /*pThread*/, void* _tid)
{
    int tid = *(int*)_tid;
    KLBDELETE(_tid);
    DownloadManager::getInstance(NULL)->runNextTask(tid);
    return 1;
}

void
DownloadManager::runNextTask(int tid)
{

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

        CKLBHTTPInterface* httpIF = NetworkManager::createConnection();

        char path[64];
        int downloadedSize = 0;
        sprintf(path, "file://external/tmpDL/%d.zip", task.queueId);
        // initialize download
        httpIF->reuse();
        httpIF->setDownload(path);
        httpIF->httpGET(task.url, false);

        s64 sizeDiff = 0;
        s64 timeDiff = CPFInterface::getInstance().platform().nanotime();

        // check download loop
        while (true)
        {
            // is the download finished
            bool bResult = httpIF->httpRECV();
            // Current downloaded size (may be inacurate)
            s64 size = httpIF->getDwnldSize();
            // Completly download size (accurate but updated at the end)
            s64 completeOnSize = httpIF->getSize();

            // calculate speed of current thread
            if (size != 0) {
                s64 now = CPFInterface::getInstance().platform().nanotime();
                s64 timeDelta = now - timeDiff;
                s64 sizeDelta = size - sizeDiff;
                timeDiff = now;
                sizeDiff = size;
                m_speed[tid] = (((double)sizeDelta) / (double)timeDelta) * 976562.5; // 976562.5 = 10^9/1024
            }
            
            bool isError = httpIF->isError();
            if (isError)
            {
                callBackOnHttpError(-1);
                break;
            }

            if (size != downloadedSize) 
            {
                downloadedSize = size;
            }
            if (bResult) 
            {
                // downloading is done
                if (completeOnSize == task.size) {
                    m_downloadClient->oneSuccessCallback(task.queueId);
                }
                else {
                    callBackOnHttpError(httpIF->getHttpState());
                }
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(32));
        }
        NetworkManager::releaseConnection(httpIF);
    }

    // at last, minus thread count
    s_threadCount.lock();
    bool allDone = --m_threadCount == 0;
    s_threadCount.unlock();

    if (allDone)
    {
        // DONT CALL THIS m_downloadClient->allSuccessCallback();
    }

    // remove tid of this thread from m_thread
    s_thread.lock();
    m_thread.erase(tid);
    s_thread.unlock();
}

int 
DownloadManager::download(char* url, int size, int queueId)
{
    if (m_isError) {
        return -1;
    }
    DownloadManager::Task task;
    task.id = ++m_lastId;
    task.url = url;
    task.size = size;
    task.queueId = queueId;

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
        int *tid = KLBNEW(int); // allocate on heap to prevent de-alloc
        *tid = 0;
        for (int i = 0; i < MAX_DOWNLOAD_THREAD; i++) {
            if (m_thread.find(i) == m_thread.end()) {
                *tid = i;
                break;
            }
        }
        void* newThread = CPFInterface::getInstance().platform().createThread(runNextTask, tid);
        m_speed[*tid] = 0.0;
        m_thread[*tid] = newThread;
        s_thread.unlock();
    }
    

    return task.id;
}

void
DownloadManager::callBackOnHttpError(int statusCode)
{
    m_isError = true;
    // empty download queue
    s_waiting.lock();
    m_waiting.empty();
    s_waiting.unlock();

    // callback
    m_downloadClient->httpFailureCallback(statusCode);
}

double
DownloadManager::getTotalSpeed()
{
    double total = 0.0;
    for (int i = 0; i < MAX_DOWNLOAD_THREAD; i++) {
        total += m_speed[i];
    }
    return total;
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
