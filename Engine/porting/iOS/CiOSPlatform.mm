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
//  CiOSPlatform.cpp
//
//
#include <iostream>
#include <pthread.h>
#include <time.h>
#include <sys/param.h>
#include <sys/mount.h>
#import <mach/mach_time.h>
#import <CommonCrypto/CommonDigest.h>
#import <Security/Security.h>
#import <StoreKit/StoreKit.h>
#import <sys/xattr.h>
#import <sys/types.h>
#import <sys/sysctl.h>

#include "RenderingFramework.h"
#include "CiOSPlatform.h"
#include "CiOSReadFileStream.h"
#include "CSockReadStream.h"
#include "CiOSAudio.h"
#include "CiOSPathConv.h"
#include "CiOSWidget.h"
#include "CiOSSysResource.h"
#include "CiOSTmpFile.h"
#include "CKLBLuaEnv.h"
#include "CKLBJsonItem.h"
#include "CKLBUtility.h"
#include "CiOSAudioManager.h"
#include "FontRendering.h"

#include "HMAC_SHA1.h"
#include "bin2hex.h"
#import "RSA.h"
#import "AESCipher.h"


#define IDENTIFIER 

CiOSPlatform * CiOSPlatform::m_instance = 0;


// フォント対応表。抽象化されたaliasと、実際iOSに渡すフォント名の対応をとる。
CiOSFont::FONTLIST CiOSFont::ms_fontlist[] = {
	// alias        iOS-name
	{ "Georgia",    "Georgia" },
	{ 0, 0 }
};

CiOSFont::CiOSFont(int size, const char * fontName)
{
	float truesize = size * 5.0f / 6.0f;
	float scale = CiOSPlatform::getInstance()->m_scale;

	for(int i = 0; fontName && ms_fontlist[i].alias; i++) {
		if(!strcmp(ms_fontlist[i].alias, fontName)) {
			m_fontname = ms_fontlist[i].fontname;
			NSString * ns_fontname = [NSString stringWithUTF8String:m_fontname];
			m_tFont = [UIFont fontWithName:ns_fontname size:truesize];
			m_tFontScaled = [UIFont fontWithName:ns_fontname size:(truesize / scale)];
			return;
		}
	}
	// 該当するものがなければシステムフォント
	m_fontname = 0;
	m_tFont = [UIFont systemFontOfSize:truesize];
	m_tFontScaled = [UIFont systemFontOfSize:(truesize / scale)];
}
CiOSFont::~CiOSFont() {}



CiOSPlatform::CiOSPlatform(UIViewController<UIAlertViewDelegate/*,SKProductsRequestDelegate,SKPaymentTransactionObserver*/> * pViewController, EAGLView * pView, float scale)
: m_pViewController(pViewController), m_pView(pView), m_scale(scale)
{
    //-----機種取得
    size_t size;
    sysctlbyname("hw.machine", NULL, &size, NULL, 0);
    char* machine = static_cast<char*>(malloc(size));
    sysctlbyname("hw.machine", machine, &size, NULL, 0);
    /*
     Possible values:
     "i386" = iPhone Simulator
     "iPhone1,1" = iPhone 1G
     "iPhone1,2" = iPhone 3G
     "iPhone2,1" = iPhone 3GS
     "iPhone3,1" = iPhone 4
     "iPod1,1" = iPod touch 1G
     "iPod2,1" = iPod touch 2G
     "iPod3,1" = iPod touch 3G
     "iPod4,1" = iPod touch 4G
     */
    //NSString *platform = [NSString stringWithCString: machine ];
    // ,->_ に整形
    NSString *tmpPlatform = [NSString stringWithCString: machine encoding:NSUTF8StringEncoding];
    NSString *platform = [tmpPlatform stringByReplacingOccurrencesOfString:@"," withString:@"_"];
    if ([platform isEqualToString:@"x86_64"])
        platform = [[NSString alloc] initWithFormat:@"%@", @"iPhone11_8"];
    
    free(machine);
    //-----
    
	NSString * modelname = [[UIDevice currentDevice] model];
	NSArray * aOsVersions = [[[UIDevice currentDevice]systemVersion] componentsSeparatedByString:@"."];
	NSInteger iOsVersionMajor = [[aOsVersions objectAtIndex:0] intValue];
	NSInteger iOsVersionMinor = [[aOsVersions objectAtIndex:1] intValue];
	sprintf(m_platform, "%s %s %d.%d",
            [platform UTF8String],
			[modelname UTF8String],
			(int)iOsVersionMajor,
			(int)iOsVersionMinor);
	m_instance = this;
}
CiOSPlatform::~CiOSPlatform() {
	m_instance = 0;
}


	CiOSPlatform *
CiOSPlatform::getInstance()
{
	return m_instance;
}

bool CiOSPlatform::useEncryption() {
	return true;
}

s64
CiOSPlatform::nanotime()
{
	uint64_t tval = mach_absolute_time();
	mach_timebase_info_data_t base;
	mach_timebase_info(&base);

	return (s64)((tval * base.numer) / base.denom);
}

	void
CiOSPlatform::detailedLogging(const char * basefile, const char * functionName, int lineNo, const char * format, ...)
{
#ifdef DEBUG
#ifndef DEBUG_PRINT_OFF
	NSString * nsformat = [NSString stringWithUTF8String:format];

	// format済み文字列を生成
	va_list ap;
	va_start(ap, format);
	if (![nsformat hasSuffix: @"\n"]) {
		nsformat = [nsformat stringByAppendingString: @"\n"];
	}
	NSString *body = [[NSString alloc] initWithFormat:nsformat arguments:ap];
	NSLog(@"%s",[body UTF8String]);
	[body release];
	va_end(ap);

#endif // #ifndef DEBUG_PRINT_ON
#endif // #ifdef DEBUG
}

	void
CiOSPlatform::logging(const char * format, ...)
{
#ifdef DEBUG
#ifndef DEBUG_PRINT_OFF
	NSString * nsformat = [NSString stringWithUTF8String:format];

	// format済み文字列を生成
	va_list ap;
	va_start(ap, format);
	if (![nsformat hasSuffix: @"\n"]) {
		nsformat = [nsformat stringByAppendingString: @"\n"];
	}
	NSString *body = [[NSString alloc] initWithFormat:nsformat arguments:ap];
	NSLog(@"%s",[body UTF8String]);
	[body release];
	va_end(ap);

#endif // #ifndef DEBUG_PRINT_ON
#endif // #ifdef DEBUG
}

const char* CiOSPlatform::getBundleVersion() {
    //set version string here instead of xcode project setting
    const char* bundle_version = "6.9.1";
    return /*[[[NSBundle mainBundle] objectForInfoDictionaryKey: @"CFBundleVersion"] cStringUsingEncoding:NSUTF8StringEncoding]*/ bundle_version;
}

	ITmpFile *
CiOSPlatform::openTmpFile(const char * tmpPath)
{
	const char * target = "file://external/";
	int len = strlen(target);
	if(!strncmp(tmpPath, target, len)) {
		// 平成24年11月27日(火)
		// CiOSTmpFileのファイルパスの解決の仕方が'file://'を抜いた状態で解釈するため、
		// CiOSTmpFileへ渡すファイルパスのprefixを'file://'分進めて渡しています。
		tmpPath = tmpPath + 7;
		CiOSTmpFile * pTmpFile = new CiOSTmpFile(tmpPath);
		if(!pTmpFile->isReady()) {
			delete pTmpFile;
			pTmpFile = 0;
		}
		return pTmpFile;
	}
	return 0;
}

#include "FileDelete.h"

const char* getFullNativePath(const char* path) {
	return CiOSPathConv::getInstance().fullpath(path + 7);
}

bool
CiOSPlatform::removeFileOrFolder(const char* filePath) {
	const char * target = "file://external/";
	int len = strlen(target);
	if(!strncmp(filePath, target, len)) {
		return deleteFiles(filePath);
	} else {
		return false;
	}
}

void removeTmpFileNative(const char* filePath) {
	remove(filePath);
}

void
CiOSPlatform::removeTmpFile(const char *tmpPath)
{
	const char * target = "file://external/";
	int len = strlen(target);
	if(!strncmp(tmpPath, target, len)) {
		const char * fullpath = CiOSPathConv::getInstance().fullpath(tmpPath + 7);
		remove(fullpath);
		delete [] fullpath;
	}
}

u32
CiOSPlatform::getFreeSpaceExternalKB() {
    struct statfs diskInfo;
    int ret = statfs(CiOSPathConv::getInstance().fullpath("external/"), &diskInfo);
    if (ret != 0) {
        // return 0 for safety in case of fetch error just the same as Android version
        return 0;
    }
    // calculate free space in KB available to non-superusers
    unsigned long long freeKB = (diskInfo.f_bavail * diskInfo.f_bsize) >> 10;
    if (freeKB & ~0xFFFFFF) {
        return 0xFFFFFF;
    }
    return (u32)freeKB;
}

u32
CiOSPlatform::getPhysicalMemKB() {
	//
	// IOS Physical Memory
	//
	size_t length;
	int mib[6];
	int result;

	mib[0] = CTL_HW;
	mib[1] = HW_PHYSMEM;
	length = sizeof(result);
	if (sysctl(mib, 2, &result, &length, NULL, 0) < 0) {
		// If reading fails, return 0
		result = 0;
	} else {
		result >>= 10;
	}
	return (u32)result;
}

void
CiOSPlatform::excludePathFromBackup(const char * fullpath)
{
    NSString *sysVer = [[UIDevice currentDevice] systemVersion];
    if ([sysVer compare:@"5.1" options:NSNumericSearch] != NSOrderedAscending)
    {
        // version>=5.1
        NSError *error = nil;
        NSURL *targetUrl = [NSURL fileURLWithPath:[NSString stringWithUTF8String:(fullpath)]];
        [targetUrl setResourceValue: [NSNumber numberWithBool: YES]
                             forKey: NSURLIsExcludedFromBackupKey
                              error: &error];
    }
    else if ([sysVer compare:@"5.0.1"] == NSOrderedSame)
    {
        // version=5.0.1
        const char *attrName = "com.apple.MobileBackup";
        u_int8_t attrValue = 1;
        
        // could be better to use fileSystemRepresentation for multi-bytes string
        setxattr(fullpath, attrName, &attrValue, sizeof(attrValue), 0, 0);
    }
    else
    {
        // version<=5.0
        // no support for exclusion
    }
}
	IReadStream *
CiOSPlatform::openReadStream(const char *pathname, bool decrypt)
{
	// ファイル名の scheme で、どのファイルを開くべきかが決まる。
	if(!strncmp(pathname, "file://", 7)) {
		// ファクトリには scheme を除いたパスが渡される。
		CiOSReadFileStream * pRds = CiOSReadFileStream::openStream(pathname + 7, 0);
		if (decrypt) { pRds->decryptSetup((const u8*)pathname + 7); }
		return pRds;
	}
	// scheme が asset であれば、作るのは CiOSReadFileStream だが、EXTERN -> INSTALL の順で検索する。
	if(!strncmp(pathname, "asset://", 8)) {
		CiOSReadFileStream * pRds = CiOSReadFileStream::openAssets(pathname, 0);
		if (decrypt) { pRds->decryptSetup((const u8*)pathname + 8); }
		return pRds;
	}

	if(!strncmp(pathname, "socket://", 9)) {
		CSockReadStream * pRds = CSockReadStream::openStream(pathname + 9);
		return pRds;
	}
	if(!strncmp(pathname, "http:", 5)) {
		return 0;
	}
	if(!strncmp(pathname, "https:", 6)) {
		return 0;
	}
	return 0;
}

	void *
CiOSPlatform::loadAudio(const char * url, bool is_se)
{
	return CiOSAudioManager::GetInstance().CreateAudioHandle( url, this );
}

	bool
CiOSPlatform::preLoad(void *handle)
{
	if(!handle) return false;
	CiOSAudio * audio = (CiOSAudio*)handle;
	return audio->loadMem();
}

	bool
CiOSPlatform::setBufSize(void *handle, int level)
{
	if(!handle) return false;
	CiOSAudio * audio = (CiOSAudio *)handle;
	return audio->setBufSize(level);
}

	void
CiOSPlatform::playAudio(void *handle, s32 _msec, float _tgtVol, float _startVol)
{
	if(!handle) return;
	CiOSAudio * audio = (CiOSAudio*)handle;
	audio->play(_msec, _tgtVol, _startVol);
}

	void
CiOSPlatform::stopAudio(void *handle, s32 _msec, float _tgtVol)
{
	if(!handle) return;
	CiOSAudio * audio = (CiOSAudio*)handle;
	audio->stop(_msec, _tgtVol);
}

	void
CiOSPlatform::setMasterVolume(float volume, bool SEmode)
{
	if(SEmode) {
		CiOSAudioManager::GetInstance().SetSEMasterVolume(volume);
	} else {
		CiOSAudioManager::GetInstance().SetBGMMasterVolume(volume);
	}
	CiOSAudioManager::GetInstance().UpdateVolume();
}

	void
CiOSPlatform::setAudioVolume(void *handle, float volume)
{
	if(!handle) return;
	CiOSAudio * audio = (CiOSAudio *)handle;
	audio->setVolume(volume);
}

	void
CiOSPlatform::setAudioPan(void *handle, float pan)
{
	if(!handle) return;
	CiOSAudio * audio = (CiOSAudio *)handle;
	audio->setPan(pan);
}

	void
CiOSPlatform::releaseAudio(void *handle)
{
	if(!handle) return;
	CiOSAudio * audio = (CiOSAudio*)handle;
	audio->closeAudio();
}

	void
CiOSPlatform::pauseAudio(void * handle, s32 _msec, float _tgtVol)
{
	if(!handle) return;
	CiOSAudio * audio = (CiOSAudio*)handle;
	audio->pause(_msec, _tgtVol);
    audio->setLuaPause(true);
}

	void
CiOSPlatform::resumeAudio(void * handle, s32 _msec, float _tgtVol)
{
	if(!handle) return;
	CiOSAudio * audio = (CiOSAudio*)handle;
	audio->resume(_msec, _tgtVol);
    audio->setLuaPause(false);
}

	void
CiOSPlatform::seekAudio(void * handle, s32 millisec)
{
	if(!handle) return;
	CiOSAudio * audio = (CiOSAudio*)handle;
	audio->seek(millisec);
}

	s32
CiOSPlatform::tellAudio(void * handle)
{
	if(!handle) return 0;
	CiOSAudio * audio = (CiOSAudio*)handle;
	return audio->tell();
}

	s32
CiOSPlatform::totalTimeAudio(void * handle)
{
	if(!handle) return 0;
	CiOSAudio * audio = (CiOSAudio*)handle;
	return audio->totalPlayTime();
}

	void
CiOSPlatform::setFadeParam(void * _handle, float _tgtVol, u32 _msec)
{
	if(!_handle) return;
	CiOSAudio * audio = (CiOSAudio*)_handle;
	audio->setFadeParam(_tgtVol, _msec);
}

	s32
CiOSPlatform::getState(void * handle)
{
	if(!handle) return 0;
	CiOSAudio * audio = (CiOSAudio*)handle;
	return audio->getState();
}

//! サウンドとミュージックの並行処理タイプ設定
	void
CiOSPlatform::setAudioMultiProcessType( s32 _processType )
{
	switch( _processType )
	{
		case IClientRequest::E_SOUND_MULTIPROCESS_MUSIC_CUT:
		case IClientRequest::E_SOUND_MULTIPROCESS_SOUND_CUT:
		case IClientRequest::E_SOUND_MULTIPROCESS_SOUND_BGM_CUT:
			CiOSAudioManager::GetInstance().SetAudioMultiProcessType(_processType);
			break;
	}
}

/*!
    @brief  サウンドの割り込み処理をエンジン側で制御するかどうか
    @param[in]  bool _bPauseOnInterruption
    @return     void
 */
void CiOSPlatform::setPauseOnInterruption(bool _bPauseOnInterruption)
{
    CiOSAudioManager::GetInstance().setPauseOnInterruption(_bPauseOnInterruption);
}

/*!
    @brief  経過時間を取得(sec)
    @param[in]  void
    @return     s64     経過時間(sec)
 */
s64 CiOSPlatform::getElapsedTime(void)
{
    struct timeval boottime;
    
    int mib[2] = {CTL_KERN, KERN_BOOTTIME};
    size_t size = sizeof(boottime);
    time_t now;
    time_t uptime = 0;
    (void)time(&now);
    
    if( (sysctl(mib, 2, &boottime, &size, NULL, 0) != -1) && (boottime.tv_sec != 0) )
    {
        uptime = now - boottime.tv_sec;
    }
    return uptime;
}

bool CiOSPlatform::registerFont(const char* logicalName, const char* physFile, bool default_) {
	return FontObject::registerFont(logicalName, physFile, default_);
}

	void *
CiOSPlatform::getFont(int size, const char * fontName, float* pAscent)
{
    FontObject* pFont = FontObject::createFont(fontName, size);
    if (pFont) {
        if (pAscent) {
            *pAscent = pFont->getAscent();
        }
    }
    return pFont;
}

	void *
CiOSPlatform::getFontSystem(int size, const char * fontName)
{
	CiOSFont * pFont = new CiOSFont(size, fontName);
	return (void *)pFont;
}
 
	void
CiOSPlatform::deleteFont(void * pFont)
{
	FontObject::destroyFont((FontObject*)pFont);
}

	void
CiOSPlatform::deleteFontSystem(void * pFont)
{
	CiOSFont * p_pFont = (CiOSFont *)pFont;
	delete p_pFont;
}

	bool
CiOSPlatform::renderText(const char *utf8String, void *pFont, u32 color,
		u16 width, u16 height, u8 *pBuffer8888, 
		s16 stride, s16 base_x, s16 base_y, bool use4444)
{
    FontObject* pObjFont = (FontObject*)pFont;
    if (pObjFont) {
        pObjFont->renderText(base_x, base_y, utf8String, pBuffer8888, color, width, height, stride, use4444);
    }
	return true;
}

	bool
CiOSPlatform::getTextInfo(const char* utf8String, void * pFont, STextInfo* pReturnInfo)
{
    FontObject* pF = (FontObject*)pFont;
    if (pF) {
        pF->getTextInfo(utf8String, pReturnInfo);
    } else {
        pReturnInfo->ascent    = 0.0f;
        pReturnInfo->descent  = 0.0f;
        pReturnInfo->bottom    = 0.0f;
        pReturnInfo->top    = 0.0f;
        pReturnInfo->width    = 0.0f;
        pReturnInfo->height    = 0.0f;
    }
    return true;
}

	void *
CiOSPlatform::getGLExtension(const char * ext)
{
	return 0;
}

const char *
CiOSPlatform::getFullPath(const char *assetPath, bool * isReadOnly)
{
    CiOSPathConv& pathconv = CiOSPathConv::getInstance();
    
    // ファイル名の scheme で、どのファイルを開くべきかが決まる。
    if(!strncmp(assetPath, "file://", 7)) {
        return pathconv.fullpath(assetPath + 7, 0, isReadOnly);
    }
    // scheme が asset であれば、作るのは CiOSReadFileStream だが、EXTERN -> INSTALL の順で検索する。
    if(!strncmp(assetPath, "asset://", 8)) {
        return pathconv.fullpath(assetPath, 0, isReadOnly);
    }
    return 0;
}

	const char *
CiOSPlatform::getPlatform()
{
	return (const char *)m_platform;
}

//! OSコントロールの生成と破棄
	IWidget *
CiOSPlatform::createControl(IWidget::CONTROL type, int id, const char * caption,
		int x, int y, int width, int height, ...)
{
	IWidget * pRetWidget = 0;
	va_list ap;
	va_start(ap, height);
	switch (type) {
		case IWidget::TEXTBOX:
		case IWidget::PASSWDBOX:
			{
				int maxlen = va_arg(ap, int);
				CiOSTextWidget * pWidget = new CiOSTextWidget(this, maxlen);
				bool result = pWidget->create(type, id, caption, x, y, width, height);
				if(!result) {
					delete pWidget;
					pWidget = 0;
				}
				pRetWidget = pWidget;
				break;
			}
		case IWidget::WEBVIEW:
		case IWidget::WEBNOJUMP:
			{
				// WebView の場合、規定のパラメータ以外にAuthorized Header の token と region のデータがつく。
				const char * token  = va_arg(ap, const char *);
				const char * region = va_arg(ap, const char *);
				const char * client = va_arg(ap, const char *);
				const char * cKey   = va_arg(ap, const char *);
				const char * appID  = va_arg(ap, const char *);
				const char * userID = va_arg(ap, const char *);

				CiOSWebWidget * pWidget = new CiOSWebWidget(this);
				bool result = pWidget->create(type, id, caption, x, y, width, height,
						token, region, client, cKey, appID, userID);
				if(!result) {
					delete pWidget;
					pWidget = 0;
				}
				pRetWidget = pWidget;
				break;
			}
		case IWidget::MOVIEPLAYER:
			{
				CiOSMovieWidget * pWidget = new CiOSMovieWidget(this);
				bool result = pWidget->create(type, id, caption, x, y, width, height);
				if(!result) {
					delete pWidget;
					pWidget = 0;
				}
				pRetWidget = pWidget;
				break;
			}
		case IWidget::BGMOVIEPLAYER:
			{
				CiOSMovieWidget * pWidget = new CiOSMovieWidget(this);
				bool result = pWidget->create(type, id, caption, x, y, width, height);
				if(!result) {
					delete pWidget;
					pWidget = 0;
				}
				pRetWidget = pWidget;
				break;
			}
		case IWidget::ACTIVITYINDICATOR:
			{
				CiOSActivityIndicator * pWidget = new CiOSActivityIndicator(this);
				bool result = pWidget->create(type, id, caption, x, y, width, height);
				if(!result) {
					delete pWidget;
					pWidget = 0;
				}
				pRetWidget = pWidget;
				break;
			}
		default:
			{
				break;
			}
	}
	va_end(ap);
	return pRetWidget;
}

	void
CiOSPlatform::destroyControl(IWidget * pWidget)
{
	delete pWidget;
}

	bool
CiOSPlatform::callApplication(IPlatformRequest::APP_TYPE type, ...)
{
    bool result = true;
    va_list ap;
    va_start(ap, type);

    switch (type) {
        case IPlatformRequest::APP_MAIL:
            {
                //UTF8に変換しないと日本語で化ける 2013-3-5 inada-s
                NSString *addr    = [NSString stringWithCString: va_arg(ap, const char *) encoding:NSUTF8StringEncoding];
                NSString *subject = [NSString stringWithCString: va_arg(ap, const char *) encoding:NSUTF8StringEncoding];
                NSString *body    = [NSString stringWithCString: va_arg(ap, const char *) encoding:NSUTF8StringEncoding];

                NSString *url = [[NSString alloc]initWithFormat:@"mailto:%@?subject=%@&body=%@", addr, subject, body];
                url = [[url stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding]stringByReplacingOccurrencesOfString:@"+" withString:@"%2B%"];
                [[UIApplication sharedApplication] openURL:[NSURL URLWithString:url]];
            }
            break;
        case IPlatformRequest::APP_BROWSER:
            {
                NSString *urlString = [NSString stringWithCString: va_arg(ap, const char *) encoding:NSUTF8StringEncoding];
                NSURL *url = [NSURL URLWithString:urlString];
                [[UIApplication sharedApplication] openURL:url];
            }
            break;
        case IPlatformRequest::APP_UPDATE:
            {
                const char * search_key = va_arg(ap, const char *);
                NSString * url_str = [NSString stringWithFormat:@"http://itunes.apple.com/WebObjects/MZStore.woa/wa/viewSoftwareUpdate?id=%s&mt=8", search_key];
                NSURL *url = [NSURL URLWithString:url_str];
                [[UIApplication sharedApplication] openURL:url];
            }
            break;
        default:
            result = false;
            break;
    }
    va_end(ap);
    return result;
}

void *
CiOSPlatform::ThreadProc(void * data)
{
	PF_THREAD * pThread = (PF_THREAD *)data;
	if(!(pThread->result = setjmp(pThread->jmp))) {
		pThread->result = (pThread->thread_func)(pThread, pThread->data);
	}
	return 0;
}

	void *
CiOSPlatform::createThread(s32 (*thread_func)(void *, void *), void *data)
{
	PF_THREAD * thread = new PF_THREAD;
	if (!thread) return 0;
	thread->data = data;
	thread->thread_func = thread_func;
	thread->result = 0;
	pthread_create(&(thread->id), 0, ThreadProc, thread);
	return thread;
}

	void
CiOSPlatform::exitThread(void *hThread, s32 status)
{
	PF_THREAD * pThread = (PF_THREAD *)hThread;
	longjmp(pThread->jmp, status);
}

	bool
CiOSPlatform::watchThread(void *hThread, s32 *status)
{
	PF_THREAD * pThread = (PF_THREAD *)hThread;
	if(pthread_kill(pThread->id, 0) != ESRCH) {
		// スレッドが終了していない
		return true;
	}
	// スレッドは終了しているので、終了コードを *status に返す
	*status = pThread->result;
	return false;   // 終了している
}

	void
CiOSPlatform::deleteThread(void * hThread)
{
	PF_THREAD * pThread = (PF_THREAD *)hThread;
	pthread_join(pThread->id, 0);
	delete pThread;
}

	void
CiOSPlatform::breakThread(void * hThread)
{
	PF_THREAD * pThread = (PF_THREAD *)hThread;
	pthread_kill(pThread->id, SIGKILL);
}

	int
CiOSPlatform::genUserID(char * retBuf, int maxlen)
{
	NSString *uuidString = nil;
	CFUUIDRef uuid = CFUUIDCreate(0);
	if(uuid) {
		uuidString = (NSString *)CFUUIDCreateString(0, uuid);
		CFRelease(uuid);
	}
	const char * cstrUUID = [uuidString UTF8String];
	int i;
	for(i = 0; i < maxlen - 1 && cstrUUID[i]; i++) {
		retBuf[i] = cstrUUID[i];
	}
	retBuf[i] = 0;
	if( uuidString ) [uuidString release];	// 2012.12.12  解放処理追加
	return i;
}

	int
CiOSPlatform::genUserPW(const char * salt, char * retbuf, int maxlen)
{
	char buf[1024];
	time_t tm;
	int rnd = rand();
	time(&tm);

	sprintf(buf, "%d.%ld.%s", rnd, tm, salt);
	return sha512(buf, retbuf, maxlen);
}

	bool
CiOSPlatform::readyDevID()
{
	CiOSSysResource& sysRes = CiOSSysResource::getInstance();
	return sysRes.isReceived();
}

	int
CiOSPlatform::getDevID(char * retBuf, int maxlen)
{
	CiOSSysResource& sysRes = CiOSSysResource::getInstance();
	if(sysRes.isReceived() && sysRes.isSucceed()) {
		return sysRes.getDevID(retBuf, maxlen);
	}
	return 0;
}

	bool
CiOSPlatform::setKeyChain(const char * service_name, const char * key, const char * value)
{
	bool result = true;
	int len = strlen(value);
	NSString * service = [NSString stringWithUTF8String:service_name];
	NSString * nskey = [NSString stringWithUTF8String:key];

	NSMutableDictionary * attributes = nil;
	NSMutableDictionary * query = [NSMutableDictionary dictionary];
	NSData * valueData = [NSData dataWithBytes:value length:len];   

	[query setObject:(id)kSecClassGenericPassword forKey:(id)kSecClass];
	[query setObject:(id)nskey forKey:(id)kSecAttrAccount];
	[query setObject:service forKey:(id)kSecAttrService];    

	OSStatus err = SecItemCopyMatching((CFDictionaryRef)query, 0);
	if(err == noErr) {
		// update Item
		attributes = [NSMutableDictionary dictionary];
		[attributes setObject:valueData forKey:(id)kSecValueData];
		[attributes setObject:[NSDate date] forKey:(id)kSecAttrModificationDate];
		err = SecItemUpdate((CFDictionaryRef)query, (CFDictionaryRef)attributes);
		if(err != noErr) {
			result = false;
		}
	} else if(err == errSecItemNotFound) {
		// new Item
		attributes = [NSMutableDictionary dictionary];
		[attributes setObject:(id)kSecClassGenericPassword forKey:(id)kSecClass];
		[attributes setObject:(id)nskey forKey:(id)kSecAttrAccount];
		[attributes setObject:service forKey:(id)kSecAttrService];
		[attributes setObject:valueData forKey:(id)kSecValueData];

		err = SecItemAdd((CFDictionaryRef)attributes, 0);
		if(err != noErr) {
			result = false;
		}
	} else {
		result = false;
	}
	return result;    
}

	int
CiOSPlatform::getKeyChain(const char * service_name, const char * key, char * retBuf, int maxlen)
{
	NSString * service = [NSString stringWithUTF8String:service_name];
	NSString * nskey = [NSString stringWithUTF8String:key];

	NSMutableDictionary * query = [NSMutableDictionary dictionary];

	[query setObject:(id)kSecClassGenericPassword forKey:(id)kSecClass];
	[query setObject:(id)nskey forKey:(id)kSecAttrAccount];
	[query setObject:service forKey:(id)kSecAttrService];    
	[query setObject:(id)kCFBooleanTrue forKey:(id)kSecReturnData];

	NSData * valueData = nil;
	OSStatus err = SecItemCopyMatching((CFDictionaryRef)query, (CFTypeRef *)&valueData);
	int i = 0;
	if(err == noErr) {
		NSString * ns_value = [[[NSString alloc] initWithData:valueData encoding:NSUTF8StringEncoding] autorelease];
		const char * value = [ns_value UTF8String];
		for(i = 0; i < maxlen - 1 && value[i]; i++) retBuf[i] = value[i];
		retBuf[i] = 0;
	}

	// 2012.12.11  解放追加
	if( valueData ) [valueData release];

	return i;
}

	bool
CiOSPlatform::delKeyChain(const char * service_name, const char * key)
{
	bool result = true;
	NSString * service = [NSString stringWithUTF8String:service_name];
	NSString * nskey = [NSString stringWithUTF8String:key];

	NSMutableDictionary * attributes = nil;
	NSMutableDictionary * query = [NSMutableDictionary dictionary];

	[query setObject:(id)kSecClassGenericPassword forKey:(id)kSecClass];
	[query setObject:(id)nskey forKey:(id)kSecAttrAccount];
	[query setObject:service forKey:(id)kSecAttrService];    

	OSStatus err = SecItemCopyMatching((CFDictionaryRef)query, 0);
	if(err == noErr) {
		// update Item
		attributes = [NSMutableDictionary dictionary];
		[attributes setObject:[NSDate date] forKey:(id)kSecAttrModificationDate];
		err = SecItemDelete((CFDictionaryRef)query);
		if(err != noErr) {
			result = false;
		}
	}
	return result;    

}

	bool
CiOSPlatform::setSecureDataID(const char * service_name, const char * user_id)
{
	return setKeyChain(service_name, "user_id", user_id);
}

	bool
CiOSPlatform::setSecureDataPW(const char * service_name, const char * passwd)
{
	return setKeyChain(service_name, "passwd", passwd);
}

	int
CiOSPlatform::getSecureDataID(const char * service_name, char * retBuf, int maxlen)
{
	return getKeyChain(service_name, "user_id", retBuf, maxlen);
}

	int
CiOSPlatform::getSecureDataPW(const char * service_name, char * retBuf, int maxlen)
{
	return getKeyChain(service_name, "passwd", retBuf, maxlen);
}

	bool
CiOSPlatform::delSecureDataID(const char *service_name)
{
	return delKeyChain(service_name, "user_id");
}

	bool
CiOSPlatform::delSecureDataPW(const char *service_name)
{
	return delKeyChain(service_name, "passwd");
}

	int
CiOSPlatform::sha512(const char * string, char * buf, int maxlen)
{
	int len = strlen(string);
	NSData * data = [NSData dataWithBytes:string length:len];
	uint8_t digest[CC_SHA512_DIGEST_LENGTH];
	CC_SHA512(data.bytes, data.length, digest);
	NSMutableString * output = [NSMutableString stringWithCapacity:CC_SHA512_DIGEST_LENGTH * 2];
	for(int i = 0; i < CC_SHA512_DIGEST_LENGTH; i++) {
		[output appendFormat:@"%02x",digest[i]];
	}

	const char * passwd = [output UTF8String];
	int i;
	for(i = 0; i < maxlen - 1 && passwd[i]; i++) {
		buf[i] = passwd[i];
	}
	buf[i] = 0;
	return i;
}


	void
CiOSPlatform::initStoreTransactionObserver(void)
{
	//[[SKPaymentQueue defaultQueue] addTransactionObserver: m_pViewController];
}

	void
CiOSPlatform::releaseStoreTransactionObserver(void)
{
	//[[SKPaymentQueue defaultQueue] removeTransactionObserver: m_pViewController];
}
	void
CiOSPlatform::buyStoreItems(const char *item_id)
{
//	// item-id が item_id に const char * の配列としてnumで指定された数だけ渡されている。
//	NSSet * set = [NSSet set];
//
//    const char * utf8_id = item_id;
//
//    NSString * str_id = [NSString stringWithUTF8String:utf8_id];
//    set = [set setByAddingObject:str_id];
//
//	SKProductsRequest * productsRequest = [[SKProductsRequest alloc] initWithProductIdentifiers:set];
//	productsRequest.delegate = m_pViewController;
//	[productsRequest start];
//
//	m_storeState.m_mode = STORE_BUY_PRODUCTS;
//	m_storeState.m_count = 1;
//	m_storeState.m_currency = false;
}

	void
CiOSPlatform::finishStoreTransaction(const char * receipt)
{
	//[(ViewController *)m_pViewController finishStoreTransaction:[NSString stringWithFormat:@"%s", receipt]];
}

	void
CiOSPlatform::getStoreProducts(const char* json, bool currency_mode)
{
//	// item-id が item_id に const char * の配列としてnumで指定された数だけ渡されている。
//	NSSet* set = [NSSet set];
//	CKLBJsonItem* root = CKLBJsonItem::ReadJsonData(json,
//			strlen(json));
//	int product_count = 0;
//	if (root)
//	{
//		CKLBJsonItem* child = root->child();
//		while (child)
//		{
//			const char* utf8_id = child->getString();
//			if (utf8_id)
//			{
//				NSString* str_id = [NSString stringWithUTF8String:utf8_id];
//				set = [set setByAddingObject:str_id];
//				++product_count;
//			}
//			child = child->next();
//		}
//	}
//
//	SKProductsRequest * productsRequest = [[SKProductsRequest alloc] initWithProductIdentifiers:set];
//	productsRequest.delegate = m_pViewController;
//	[productsRequest start];
//
//	m_storeState.m_mode = STORE_GET_PRODUCTS;
//	m_storeState.m_count = product_count;
//	m_storeState.m_currency = currency_mode;
}


float CiOSPlatform::getMasterVolume(bool SEmode) const
{
	if( SEmode ) {
		return CiOSAudioManager::GetInstance().GetSEMasterVolume();
	} else {
		return CiOSAudioManager::GetInstance().GetBGMMasterVolume();

	}
	return 1.0f;
}


	void
CiOSPlatform::responseStoreKitProducts(SKProductsResponse* response, ViewController* viewController)
{
//	// 購入なのか、リストが欲しいだけなのかによって処理分離.
//	// 購入の場合は、トランザクションを貼ってpaymentの処理へ移行.
//	// リスト取得の場合はリストをLuaに渡しcallbackを起動する.
//	if (m_storeState.m_mode == STORE_GET_PRODUCTS)
//	{
//
//		// 平成24年12月4日(火)
//		// 言い訳。時間がない。- ->ちょっと整理
//		/*
//		   [{"id":"id","title":"title","description:"description","price":"price"},{"id":"id","title":"title","description:"description","price":"price"},...]
//
//		 */
//		// utf8
//		if (m_storeState.m_count <= 0){
//			return;
//		}
//
//
//		//NSMutableArray *contents = [NSMutableArray array];
//		NSNumberFormatter *numberFormatter = [[NSNumberFormatter alloc] init];
//		[numberFormatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
//
//		if (m_storeState.m_currency)
//		{
//			[numberFormatter setNumberStyle:NSNumberFormatterCurrencyStyle];
//		}
//
//		NSMutableString *jsonString = [NSMutableString stringWithString:@"["];
//		int idx = 0;
//		for (SKProduct* product in response.products) {
//			if (idx > 0) {
//				[jsonString appendString:@","];
//			}
//			++idx;
//			[numberFormatter setLocale:product.priceLocale];
//
//			NSString *identifier = product.productIdentifier;
//			NSString *title = product.localizedTitle;
//			NSString *description = product.description;
//			NSString *price = [numberFormatter stringFromNumber:product.price];
//			NSString *entry = [NSString stringWithFormat:@"{\"id\":\"%@\",\"title\":\"%@\",\"description\":\"%@\",\"price\":\"%@\"}", identifier, title, description, price];
//			[jsonString appendString:entry];
//		}
//
//		[numberFormatter release];
//
//		[jsonString appendString:@"]"];
//
//		NSData * json = [jsonString dataUsingEncoding:NSUTF8StringEncoding];
//#ifdef DEBUG
//		CPFInterface::getInstance().platform().logging("%@",jsonString);
//#endif
//		IClientRequest& cli = CPFInterface::getInstance().client();
//		cli.controlEvent(IClientRequest::E_STORE_GET_PRODUCTS,
//				0,
//				json.length + 1,
//				(void *)[jsonString cStringUsingEncoding:NSUTF8StringEncoding],
//				0,
//				0);
//
//#ifdef USE_EXTERNAL_SDK_LOVELIVE
//		[SDKWrapper onGetProducts:response];
//#endif
//	}
//	else if (m_storeState.m_mode == STORE_BUY_PRODUCTS)
//	{
//		// 購入処理開始
//		for(SKProduct * product in response.products) {
//			SKPayment * payment = [SKPayment paymentWithProduct:product];
//			[[SKPaymentQueue defaultQueue] addPayment:payment];
//		}
//	}
}

void*	CiOSPlatform::allocMutex	()
{
	pthread_mutex_t* pSection = new pthread_mutex_t();
	if (pSection) {
		if (pthread_mutex_init(pSection, NULL)) {
			delete pSection;
			return NULL;
		}
	}
	return pSection;
}

void	CiOSPlatform::freeMutex		(void* mutex)
{
	if (mutex) {
		pthread_mutex_t* pSection = (pthread_mutex_t*)mutex;	
		pthread_mutex_destroy(pSection); // Error handling useless here.
		delete pSection;
	}
}

void	CiOSPlatform::mutexLock		(void* mutex)
{
	if (mutex) {
		pthread_mutex_t* pSection = (pthread_mutex_t*)mutex;
		pthread_mutex_lock(pSection);
	}
}

void	CiOSPlatform::mutexUnlock	(void* mutex)
{
	if (mutex) {
		pthread_mutex_t* pSection = (pthread_mutex_t*)mutex;
		pthread_mutex_unlock(pSection);
	}
}

struct EventMutex {
	pthread_mutex_t		mutex;
	pthread_cond_t		cond;
};

void*	CiOSPlatform::allocEventLock()
{
	EventMutex* pEvent = new EventMutex();
	if (pEvent) {
		bool err = false;
		if (pthread_mutex_init(&pEvent->mutex, NULL) == 0) {
			if (pthread_cond_init(&pEvent->cond, NULL) == 0) {
				// Do nothing.
			} else {
				pthread_mutex_destroy(&pEvent->mutex);
				err = true;
			}
		} else {
			err = true;
		}
		
		if (err) {
			delete pEvent;
			pEvent = NULL;
		}
	}
	return pEvent;
}

void	CiOSPlatform::freeEventLock	(void* lock)
{
	EventMutex* pEvent = (EventMutex*)lock;
	if (pEvent) {
		pthread_mutex_destroy	(&pEvent->mutex);
		pthread_cond_destroy	(&pEvent->cond);		
	}
}

void	CiOSPlatform::eventSleep(void* lock)
{
	EventMutex* pEvent = (EventMutex*)lock;
	if (pEvent) {
		// Own mutex [Lock]
		pthread_mutex_lock		(&pEvent->mutex);
		// [Unlock] and go to [Sleep], atomically.
		pthread_cond_wait		(&pEvent->cond, &pEvent->mutex);
		// [Lock] on wake up.
		
		// [Unlock] again.
		pthread_mutex_unlock	(&pEvent->mutex);
	}
}

void	CiOSPlatform::eventWakeup(void* lock)
{
	EventMutex* pEvent = (EventMutex*)lock;
	if (pEvent) {
		// Own mutex [Lock]
		pthread_mutex_lock		(&pEvent->mutex);
	
		pthread_cond_broadcast	(&pEvent->cond);

		// [Unlock] again.
		pthread_mutex_unlock	(&pEvent->mutex);
	}
}

void	CiOSPlatform::forbidSleep(bool is_forbidden)
{
    [UIApplication sharedApplication].idleTimerDisabled = is_forbidden;
}

void*		CiOSPlatform::ifopen	(const char* name, const char* mode) {
	return fopen(name, mode);
}

void		CiOSPlatform::ifclose	(void* file) {
    if (file) {
        fclose((FILE*)file);
    }
}

int			CiOSPlatform::ifseek	(void* file, long int offset, int origin) {
	return fseek((FILE*)file,offset,origin);
}

u32			CiOSPlatform::ifread	(void* ptr, u32 size, u32 count, void* file ) {
	return fread(ptr, size, count, (FILE*)file);
}

u32			CiOSPlatform::ifwrite	(const void * ptr, u32 size, u32 count, void* file) {
	return fwrite(ptr, size, count, (FILE*)file);
}

int			CiOSPlatform::ifflush	(void* file) {
	return fflush((FILE*)file);
}

long int	CiOSPlatform::iftell	(void* file) {
	return ftell((FILE*)file);
}

bool CiOSPlatform::icreateEmptyFile(const char* name) {
	FILE* f = fopen(name,"a");
	if (f) {
		fclose(f);
		return true;
	}
	return false;
}


int CiOSPlatform::HMAC_SHA1(const char* input, const char* key, int keyLen, char* retbuf) {
    unsigned char rawHash[21];
    CHMAC_SHA1 *ctx = new CHMAC_SHA1;
    ctx->HMAC_SHA1( reinterpret_cast<unsigned char*>(const_cast<char*>(input)), strlen(input),
                    reinterpret_cast<unsigned char*>(const_cast<char*>(key)), keyLen,
                    rawHash);
    const char* tmp = bin2hex(rawHash, 20);
    memcpy(retbuf, tmp, 40);
    retbuf[40] = 0;
    //delete [] tmp;
    delete ctx;
    return 0;
}

int CiOSPlatform::encryptAES128CBC(const char* plaintext, int plaintextLen, const char* _key, unsigned char* out, int outLen)
{
    NSString *content = [[NSString alloc] initWithUTF8String:plaintext];
    NSString *key = [[NSString alloc] initWithUTF8String:_key];
    NSString *encryptStr = [AESCipher encryptAES:content key:_key];
    
    NSData *data = [encryptStr dataUsingEncoding:NSUTF8StringEncoding];
    out = (unsigned char*)[data bytes];
    int ret = (int)[data length];
    
    return ret;
}

int CiOSPlatform::publicKeyEncrypt(unsigned char* plaintext, int plaintextLen, unsigned char* out, int outLen)
{
    NSData *data = [[NSData alloc] initWithBytes:plaintext length:plaintextLen];
    NSData *encryptData = [RSA encryptData:data publicKey:publicKey];
    out = (unsigned char* )[encryptData bytes];
    int ret = (int)[encryptData length];
    
    return ret;
}

bool CiOSPlatform::publicKeyVerify(unsigned char* plaintext, int plaintextLen, unsigned char* hash)
{
    NSData *plainData = [[NSData alloc] initWithBytes:plaintext length:plaintextLen];
    size_t hashLen = strlen((const char*)hash);
    NSData *encryptData = [[NSData alloc] initWithBytes:hash length:hashLen];
    BOOL ret = [RSA verifyPubKey:plainData encryptData:encryptData publicKey:publicKey];
	return (bool)ret;
}

int CiOSPlatform::getRandomBytes(char* out, int len)
{
    NSString *table = [[NSString alloc] initWithFormat:@"%@", @"01234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"];
    NSString *result = [NSMutableString stringWithCapacity:len];
    [result appendFormat:@"%C", [table characterAtIndex:(arc4random() % ([table length]-1))+1]];
    for (int i = 1; i < len; i++)
    {
        [result appendFormat:@"%C", [table characterAtIndex:arc4random() % [table length]]];
    }
    out = (char*)[result UTF8String];
    return (int)YES;
}

int CiOSPlatform::getAuthSecret(char* out, int len)
{
    return 0;
}
