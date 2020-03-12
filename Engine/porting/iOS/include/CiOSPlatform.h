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
/*!
  ¥brief     iOS 用プラットフォーム側インタフェースクラス
  ¥file      CiOSPlatform.h
  */
#ifndef CiOSPlatform_h
#define CiOSPlatform_h

#import <UIKit/UIKit.h>
#import <StoreKit/StoreKit.h>
#import "ViewController.h"
#include "CPFInterface.h"
#include "FileSystem.h"
#include "CiOSAudioManager.h"
#include "CiOSWidget.h"
#include "EAGLView.h"

class CiOSFont
{
public:
	CiOSFont(int size, const char * fontName = 0);
	virtual ~CiOSFont();

	inline UIFont * getFont() { return m_tFont; }
	inline UIFont * getFontScaled() { return m_tFontScaled; }
	inline const char * getFontname() { return m_fontname; }
private:
	UIFont  *   m_tFont;
	UIFont  *   m_tFontScaled;
	const char *  m_fontname;

	static struct FONTLIST {
		const char * alias;
		const char * fontname;
	} ms_fontlist[];
};

class CiOSPlatform : public IPlatformRequest
{
	friend class CiOSWidget;
	friend class CiOSTextWidget;
	friend class CiOSFont;
	friend void assertFunction(int line, const char* file, const char* msg,...);

public:
	CiOSPlatform(UIViewController<UIAlertViewDelegate/*,SKProductsRequestDelegate,SKPaymentTransactionObserver*/> * pViewController, EAGLView * pView, float scale);
    virtual ~CiOSPlatform();

    void detailedLogging(const char * basefile, const char * functionName, int lineNo, const char * format, ...);
    void logging(const char * format, ...);

    // バンドルバージョン取得
    const char* getBundleVersion();
    
	//! Use Encryption for disk I/O
	virtual bool useEncryption();    

    s64 nanotime();
    
    // ファイルシステム
    IReadStream * openReadStream(const char * filename, bool decrypt);
    
    ITmpFile * openTmpFile(const char * tmpPath);
    void removeTmpFile(const char * tmpPath);
	virtual bool removeFileOrFolder	 (const char * filePath);
	virtual u32 getFreeSpaceExternalKB();
	virtual u32 getPhysicalMemKB	 ();
	void excludePathFromBackup(const char * fullpath);

	virtual void*		ifopen	(const char* name, const char* mode);
	virtual void		ifclose	(void* file);
	virtual int			ifseek	(void* file, long int offset, int origin);
	virtual u32			ifread	(void* ptr, u32 size, u32 count, void* file );
	virtual u32			ifwrite	(const void * ptr, u32 size, u32 count, void* file);
	virtual int			ifflush	(void* file);
	virtual long int	iftell	(void* file);
	virtual bool		icreateEmptyFile(const char* name);

    // サウンド系
    void * loadAudio(const char * url, bool is_se = false);
    bool   preLoad(void * handle);
    bool   setBufSize(void * handle, int level);
    void   playAudio(void * handle, s32 _msec=0, float _tgtVol=1.0f, float _startVol=0.0f);
    void   stopAudio(void * handle, s32 _msec=0, float _tgtVol=0.0f);
    void   setMasterVolume(float volume, bool SEmode);
    void   setAudioVolume(void * handle, float volume);
    void   setAudioPan(void * handle, float pan);
    void   releaseAudio(void * handle);
    
    void pauseAudio(void * handle, s32 _msec=0, float _tgtVol=0.0f);
    void resumeAudio(void * handle, s32 _msec=0, float _tgtVol=1.0f);
    void seekAudio(void * handle, s32 millisec);
    s32  tellAudio(void * handle);
    s32  totalTimeAudio(void * handle);
    void setFadeParam(void * _handle, float _tgtVol, u32 _msec);

    s32  getState(void * handle);
    
    //! サウンドとミュージックの並行処理タイプ設定
    void setAudioMultiProcessType( s32 _processType );
    
    //! サウンドの割り込み処理をエンジン側で制御するかどうか
    void setPauseOnInterruption(bool _bPauseOnInterruption);
    
    //! 経過時間を取得(sec)
    s64 getElapsedTime(void);

    //! フォント取得
	bool registerFont(const char* logicalName, const char* physFile, bool default_);
    void * getFont(int size, const char * fontName = 0, float* pAscent = NULL);
    void * getFontSystem(int size, const char * fontName = 0);

    //! フォント破棄
    void deleteFont(void * pFont);
    void deleteFontSystem(void * pFont);

    //! フォントテクスチャ描画
    bool renderText(const char* utf8String, void * pFont, u32 color,    //!< 描画する文字列とフォントの指定
                    u16 width, u16 height, u8 * pBuffer8888,            //!< 描画対象とするテクスチャバッファとそのピクセルサイズ
                    s16 stride, s16 base_x, s16 base_y, u8 embolden, bool use4444);  //!< baseline起点とするテクスチャ内の位置
    // フォントテクスチャ描画情報
    bool getTextInfo(const char* utf8String, void * pFont, STextInfo* pReturnInfo);
    
    void * getGLExtension(const char * ext);

    // 環境上でのフルパス取得
    const char * getFullPath(const char * assetPath, bool * isReadOnly);
  
    //! 動作環境情報文字列を返す。
	const char * getPlatform();

	//! OSコントロールの生成と破棄
	IWidget * createControl(IWidget::CONTROL type, int id,
			const char * caption, int x, int y, int width, int height, ...);
	void destroyControl(IWidget * pControl);

	bool callApplication(APP_TYPE type, ... );

	void * createThread(s32 (*thread_func)(void * hThread, void * data), void * data);
	void exitThread(void * hThread, s32 status);
	bool watchThread(void * hThread, s32 * status);
	void deleteThread(void * hThread);
	void breakThread(void * hThread);

	int genUserID(char * retBuf, int maxlen);
	int genUserPW(const char * salt, char * retBuf, int maxlen);

	bool readyDevID();
	int getDevID(char * retBuf, int maxlen);

	bool setSecureDataID(const char * service_name, const char * user_id);
	bool setSecureDataPW(const char * service_name, const char * pw);
	int getSecureDataID(const char * service_name, char * retBuf, int maxlen);
	int getSecureDataPW(const char * service_name, char * retBuf, int maxlen);

	bool delSecureDataID(const char * service_name);
	bool delSecureDataPW(const char * service_name);

	void initStoreTransactionObserver(void);
	void releaseStoreTransactionObserver(void);
	void buyStoreItems(const char * item_id);
	void getStoreProducts(const char* json, bool currency_mode);
	void finishStoreTransaction(const char *receipt);

	void responseStoreKitProducts(SKProductsResponse* response, ViewController* viewController);

	inline EAGLView * getView() const { return m_pView; }
	inline float getScale() const { return m_scale; }
	float getMasterVolume(bool SEmode) const;

	static CiOSPlatform * getInstance();

	virtual void*	allocMutex		();
	virtual void	freeMutex		(void* mutex);
	virtual void	mutexLock		(void* mutex);
	virtual void	mutexUnlock		(void* mutex);

	virtual void*	allocEventLock	();
	virtual void	freeEventLock	(void* lock);
	virtual void	eventSleep		(void* lock);
	virtual void	eventWakeup		(void* lock);
	
	void	startAlertDialog( const char* title , const char* message ){};

	void	forbidSleep		(bool is_forbidden);
    
    virtual int         HMAC_SHA1        (const char* string, const char* key, int keyLen, char* retbuf);
    virtual int         encryptAES128CBC(const char* plaintext, int plaintextLen, const char* key, unsigned char* out, int outLen);
    virtual int         decryptAES128CBC(unsigned const char* ciphertext, int ciphertextLen, const char* key, char* out, int outLen);
    virtual int         publicKeyEncrypt(unsigned char* plaintext, int plaintextLen, unsigned char* out, int outLen);
    virtual bool        publicKeyVerify(unsigned char* plaintext, int plaintextLen, unsigned char* hash);
    virtual int         getRandomBytes(char* out, int len);
    virtual int         getAuthSecret(char* out, int len);

private:
	struct PF_THREAD {
		jmp_buf   jmp;
		pthread_t id;
		s32 (*thread_func)(void *, void *);
		void * data;
		s32 result;
	};
	static void * ThreadProc(void * data);

	bool setKeyChain(const char * service_name, const char * key, const char * value);
	int getKeyChain(const char * service_name, const char * key, char * retBuf, int maxlen);
	bool delKeyChain(const char * service_name, const char * key);

	int sha512(const char * string, char * buf, int maxlen);

	UIViewController/*<SKProductsRequestDelegate,SKPaymentTransactionObserver>*/    *   m_pViewController;
	EAGLView            *   m_pView;
	float                   m_scale;

	enum {
		STORE_NONE,
		STORE_BUY_PRODUCTS,
		STORE_GET_PRODUCTS
	};
	struct StoreState
	{
		int m_mode;
		int m_count;
		bool m_currency;
		const char* m_callback;
	} m_storeState;

	char                    m_platform[256];

	static CiOSPlatform *   m_instance;
    
    
    /* replace publicKey below if necessary */
    NSString *publicKey = [[NSString alloc] initWithString:@
    "-----BEGIN PUBLIC KEY-----\n"
    "MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDBpUMUVjHWNI5q3ZRjF1vPnh+m\n"
    "aEGdbZkeosVvzLytBy9eYJ9qLYyFXxOY1LiggWyOLS+xEVMpV3A6frI3VewkVuCw\n"
    "na52ssCZcQSBA03Ykeb/cfHk5ChsDUP1vmAbloMb9f++Dow6Z4yubFWmBVMCHA6l\n"
    "fiUDPHjI8JqG56XJKQIDAQAB\n"
    "-----END PUBLIC KEY-----"
    ];
    
};


#endif
