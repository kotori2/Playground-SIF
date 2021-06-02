/* 
   Copyright 2013 KLab Inc.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
//
//  CKLBNetAPIKeyChain.h
//  GameEngine
//

#ifndef CKLBNetAPIKeyChain_h
#define CKLBNetAPIKeyChain_h

#include "CKLBUtility.h"
#include <ctime>

/*!
* \class CKLBNetAPIKeyChain
* \brief Net API Key Chain Class
* 
* 
*/
class CKLBNetAPIKeyChain
{
private:
    CKLBNetAPIKeyChain();
    virtual ~CKLBNetAPIKeyChain();
public:
    static CKLBNetAPIKeyChain& getInstance();
	void release();

	inline const char* setUrl(const char* url) {
		KLBDELETEA(m_url);
		if (url) {
			m_url = CKLBUtility::copyString(url);
		}
		else {
			m_url = NULL;
		}
		return m_url;
	}

    inline const char * setToken(const char * token) {
        KLBDELETEA(m_token);
		if (token) {
	        m_token = CKLBUtility::copyString(token);
		} else {
			m_token = NULL;
		}
        return m_token;
    }
    
    inline const char * setRegion(const char * region) {
        KLBDELETEA(m_region);
		if (region) {
	        m_region = CKLBUtility::copyString(region);
		} else {
			m_region = NULL;
		}
        return m_region;
    }
    
    inline const char * setClient(const char * client) {
        KLBDELETEA(m_client);
		if (client) {
	        m_client = CKLBUtility::copyString(client);
		} else {
			m_client = NULL;
		}
        return m_client;
    }
    
    inline const char * setConsumernKey(const char * cKey) {
        KLBDELETEA(m_cKey);
		if (cKey) {
	        m_cKey = CKLBUtility::copyString(cKey);
		} else {
			m_cKey = NULL;
		}
        return m_cKey;
    }

    inline const char * setAppID(const char * appID) {
        KLBDELETEA(m_appID);
		if (appID) {
	        m_appID = CKLBUtility::copyString(appID);
		} else {
			m_appID = NULL;
		}
        return m_appID;
    }

	inline const char * setUserID(const char * userID) {
		KLBDELETEA(m_userID);
		if (userID) {
			m_userID = CKLBUtility::copyString(userID);
		} else {
			m_userID = NULL;
		}
		return m_userID;
	}

	inline const char * setLoginKey(const char * loginKey) {
		KLBDELETEA(m_loginKey);
		if (loginKey) {
			m_loginKey = CKLBUtility::copyString(loginKey);
		}
		else {
			m_loginKey = NULL;
		}
		return m_loginKey;
	}

	inline const char* setLoginPwd(const char* loginPwd) {
		KLBDELETEA(m_loginPwd);
		if (loginPwd) {
			m_loginPwd = CKLBUtility::copyString(loginPwd);
		}
		else {
			m_loginPwd = NULL;
		}
		return m_loginPwd;
	}

	inline const char* setSessionKey(const char* sessionKey) {
		KLBDELETEA(m_sessionKey);
		if (sessionKey) {
			m_sessionKey = CKLBUtility::copyMem(sessionKey, 32);
		}
		else {
			m_sessionKey = NULL;
		}
		return m_sessionKey;
	}

	inline const char* setClientKey(const char* clientKey) {
		KLBDELETEA(m_clientKey);
		if (clientKey) {
			m_clientKey = CKLBUtility::copyMem(clientKey, 32);
		}
		else {
			m_clientKey = NULL;
		}
		return m_clientKey;
	}

	inline const char* setLanguage(const char* language) {
		KLBDELETEA(m_language);
		if (language) {
			m_language = CKLBUtility::copyString(language);
		}
		else {
			m_language = NULL;
		}
		return m_language;
	}
    
	inline const char * getUrl			() const { return m_url;		}
    inline const char * getToken		() const { return m_token;		}
    inline const char * getRegion		() const { return m_region;		}
    inline const char * getClient		() const { return m_client;		}
    inline const char * getConsumerKey	() const { return m_cKey;		}
    inline const char * getAppID		() const { return m_appID;		}
	inline const char * getUserID		() const { return m_userID;		}
	inline const char * getLoginKey		() const { return m_loginKey;	}
	inline const char * getLoginPwd		() const { return m_loginPwd;	}
	inline const char * getSessionKey	() const { return m_sessionKey; }
	inline const char * getClientKey	() const { return m_clientKey;	}
	inline const char * getLanguage		() const { return m_language;	}

	inline int genCmdNumID(char * retBuf, const char * body, time_t timeStamp, int serial) {
		sprintf(retBuf, "%s-%s.%d.%d",
				body, m_token,
				(int)timeStamp, serial);
		int len = (int)strlen(retBuf);
		return len;
	}

	inline char* getAuthorizeString(int nonce) {
		char* authorize = KLBNEWA(char, 256);
		if (m_token) 
			sprintf(authorize, "consumerKey=%s&timeStamp=%d&version=1.1&token=%s&nonce=%d", m_cKey, int(time(NULL)), m_token, nonce);
		else 
			sprintf(authorize, "consumerKey=%s&timeStamp=%d&version=1.1&nonce=%d", m_cKey, int(time(NULL)), nonce);
		return authorize;
	}
private:
	const char		*	m_url;			// server API url
    const char      *   m_token;		// Authorized Token
    const char      *   m_region;		// region
    const char      *   m_client;		// client version
    const char      *   m_cKey;			// consumerKey
    const char      *   m_appID;		// Application ID
	const char		*	m_userID;		// User-ID
	const char		*	m_loginKey;		// login_key
	const char		*	m_loginPwd;		// login_passwd
	const char		*	m_sessionKey;	
	const char		*	m_clientKey;	
	const char		*	m_language;	
};

#endif
