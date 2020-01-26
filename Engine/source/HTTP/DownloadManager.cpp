
#include "DownloadManager.h"
#include <DownloadClient.h>

void* DownloadManager::s_waiting = NULL;
void* DownloadManager::s_threadCount = NULL;
void* DownloadManager::s_thread = NULL;
DownloadManager* DownloadManager::s_instance = NULL;;

DownloadManager::DownloadManager() :
    m_lastId(0),
    m_threadCount(0),
    m_downloadClient(NULL),
    m_isError(false)
{
    IPlatformRequest& pfif = CPFInterface::getInstance().platform();
    if(!s_waiting)      s_waiting = pfif.allocMutex();
    if(!s_threadCount)  s_threadCount = pfif.allocMutex();
    if(!s_thread)       s_thread = pfif.allocMutex();

    // unlock all mutex
    pfif.mutexUnlock(s_waiting);
    pfif.mutexUnlock(s_threadCount);
    pfif.mutexUnlock(s_thread);
}

DownloadManager* 
DownloadManager::getInstance(DownloadClient* downloadClient)
{
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
    IPlatformRequest& pfif = CPFInterface::getInstance().platform();
    // alwasy looking for new task to execute
    while (true)
    {
        Task task;

        // try to fetch next task
        pfif.mutexLock(s_waiting);
        if (!m_waiting.empty())
        {
            task = m_waiting.front();
            m_waiting.pop();
        }
        pfif.mutexUnlock(s_waiting);

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
            int  status = httpIF->getHttpState();
            if (isError || status == 403)
            {
                callBackOnHttpError(status, status == 403 ? CKLBUPDATE_DOWNLOAD_FORBIDDEN : CKLBUPDATE_DOWNLOAD_ERROR);
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
                    callBackOnHttpError(status, CKLBUPDATE_DOWNLOAD_INVALID_SIZE);
                }
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(32));
        }
        NetworkManager::releaseConnection(httpIF);
    }

    // at last, minus thread count
    pfif.mutexLock(s_threadCount);
    bool allDone = --m_threadCount == 0;
    pfif.mutexUnlock(s_threadCount);

    if (allDone)
    {
        // DONT CALL THIS m_downloadClient->allSuccessCallback();
    }

    // remove tid of this thread from m_thread
    pfif.mutexLock(s_thread);
    m_thread.erase(tid);
    pfif.mutexUnlock(s_thread);
}

int 
DownloadManager::download(char* url, int size, int queueId)
{
    if (m_isError) {
        return -1;
    }
    IPlatformRequest& pfif = CPFInterface::getInstance().platform();
    DownloadManager::Task task;
    task.id = ++m_lastId;
    task.url = url;
    task.size = size;
    task.queueId = queueId;

    // put it in to the queue
    pfif.mutexLock(s_waiting);
    m_waiting.push(task);
    pfif.mutexUnlock(s_waiting);

    // check thread count
    bool canCreateThread = false;

    pfif.mutexLock(s_threadCount);
    if (m_threadCount < MAX_DOWNLOAD_THREAD)
    {
        canCreateThread = true;
        ++m_threadCount;
    }
    pfif.mutexUnlock(s_threadCount);

    if (canCreateThread)
    {
        // create new thread to execute the task
        pfif.mutexLock(s_thread);
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
        pfif.mutexUnlock(s_thread);
    }
    

    return task.id;
}

void
DownloadManager::callBackOnHttpError(int statusCode, int errorCode)
{
    IPlatformRequest& pfif = CPFInterface::getInstance().platform();
    m_isError = true;
    // empty download queue
    pfif.mutexLock(s_waiting);
    m_waiting.empty();
    pfif.mutexUnlock(s_waiting);

    // callback
    m_downloadClient->httpFailureCallback(statusCode, errorCode);
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
    IPlatformRequest& pfif = CPFInterface::getInstance().platform();
    s_instance = NULL;
    // kill all active threads
    pfif.mutexLock(s_thread);
    if (m_thread.size() > 0) {
        std::map<int, void*>::iterator iter;
        iter = m_thread.begin();
        while (iter != m_thread.end()) {
            CPFInterface::getInstance().platform().breakThread(iter->second);
            CPFInterface::getInstance().platform().deleteThread(iter->second);
            const int key = iter->first;
            iter++;
            m_thread.erase(key);
            
        }
    }
    pfif.mutexUnlock(s_thread);
}
