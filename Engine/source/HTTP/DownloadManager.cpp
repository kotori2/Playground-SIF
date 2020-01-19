
#include "DownloadManager.h"

// error codes
enum {
	CKLBUPDATE_DOWNLOAD_FORBIDDEN,
	CKLBUPDATE_DOWNLOAD_INVALID_SIZE,
	CKLBUPDATE_DOWNLOAD_NODATA,
	CKLBUPDATE_DOWNLOAD_ERROR,
	CKLBUPDATE_UNZIP_ERROR
};

DownloadManager::DownloadManager(const char* url, const char* savePath, size_t fileSize)
	: m_http(NULL)
	, m_url(NULL)
	, m_path(NULL)
	, m_size(0)
	, m_dlSize(-1)
	, m_statusCode(-1)
	, m_errorCode(-1)
	, m_success(false)
	, m_failed(false)
{
	m_url = CKLBUtility::copyString(url);
	m_path = CKLBUtility::copyString(savePath);
	m_size = fileSize;

	m_http = NetworkManager::createConnection();
	m_http->reuse();
	m_http->setDownload(savePath);
	m_http->httpGET(url, false);
}

DownloadManager::~DownloadManager()
{
	NetworkManager::releaseConnection(m_http);
	m_http = NULL;
	KLBDELETEA(m_url);
	KLBDELETEA(m_path);
}

void
DownloadManager::execute()
{
	if (m_success || m_failed) {
		// do nothing
		return;
	}

	// get status code
	m_statusCode = m_http->getHttpState();

	if (((m_statusCode >= 400) && (m_statusCode <= 599)) || (m_statusCode == 204)) {
		m_failed = true;
		if (m_statusCode == 403) {
			m_errorCode = CKLBUPDATE_DOWNLOAD_FORBIDDEN;
		}
		else {
			m_errorCode = CKLBUPDATE_DOWNLOAD_ERROR;
		}
	}

	bool bResult = m_http->httpRECV();

	// Current downloaded size (may be inacurate)
	s64 size = m_http->getDwnldSize();
	// Completly download size (accurate but updated at the end)
	s64 completeOnSize = m_http->getSize();

	if (size != m_dlSize) {
		// update downloaded size
		m_dlSize = size;
	}

	if (bResult) {
		if (completeOnSize == m_size) {
			m_success = true;
		}
		else {
			m_failed = true;
			m_errorCode = CKLBUPDATE_DOWNLOAD_INVALID_SIZE;
		}
	}
}