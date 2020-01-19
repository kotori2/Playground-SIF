
#ifndef DownloadManager_h
#define DownloadManager_h

#include "CKLBUtility.h"
#include "MultithreadedNetwork.h"

class DownloadManager
{
public:
	DownloadManager(const char* url, const char* savePath, size_t size);
	virtual ~DownloadManager();

	void execute();
	
	inline bool isSuccess()		{ return m_success; }
	inline bool isFailed()		{ return m_failed;	}
	inline int	getStatusCode() { return m_statusCode; }
	inline int	getErrorCode()	{ return m_errorCode; }
private:
	const char*				m_url;
	const char*				m_path;
	size_t					m_size;
	size_t					m_dlSize;
	bool					m_success;
	bool					m_failed;
	int						m_statusCode;
	int						m_errorCode;

	CKLBHTTPInterface*		m_http;
};


#endif // DownloadManager_h
