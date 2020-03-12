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
//  CiOSWidget.h
//  GameEngine
//
//

#ifndef CiOSWidget_h
#define CiOSWidget_h

#import <MediaPlayer/MediaPlayer.h>
#import <UIKit/UIKit.h>
#import <WebKit/WebKit.h>
#include "CiOSPlatform.h"
#import "CiOSWebView.h"
#import "CiOSMovieView.h"
#import "CiOSTextView.h"
#include "OSWidget.h"

class CiOSFont;
class CiOSPlatform;

// iOS用の基本形
class CiOSWidget : public IWidget
{
protected:
    CiOSWidget(CiOSPlatform * pParent);
    virtual ~CiOSWidget();

public:
    bool init(int id, int x, int y, int width, int height);

	void move(int x, int y);
	void resize(int width, int height);
	virtual void visible(bool bVisible) = 0;
	virtual void enable(bool bEnable) = 0;
    
    virtual void cmd(int cmd, ...);
    virtual int status();

    inline void size_recovery() {
        set_move(m_x, m_y, m_width, m_height);
    }
protected:
    inline CiOSPlatform& getPlatform() const { return *m_pPlatform; }
    inline float getScale() const { return m_scale; }

    virtual void set_move(int x, int y, int width, int height) = 0;
protected:
    float               m_scale;
    int                 m_x;
    int                 m_y;
    int                 m_width;
    int                 m_height;
    
    int                 m_id;
    CiOSPlatform    *   m_pPlatform;
};

// iOSテキスト入力
class CiOSTextWidget : public CiOSWidget
{
    friend class CiOSPlatform;
private:
    CiOSTextWidget(CiOSPlatform * pParent, int maxlen = 0);
    virtual ~CiOSTextWidget();
public:

    bool create(CONTROL type, int id,
                const char * caption, int x, int y, int width, int height);
    
 	int getTextLength();
    int getTextMaxLength() { return m_maxlen; }
	bool getText(char * pBuf, int maxlen);
	bool setText(const char * string);
	void visible(bool bVisible);
	void enable(bool bEnable);

    void cmd(int cmd, ...);

    // 指定された UITextField を持つ CiOSTextWidget を検索する
    static CiOSTextWidget * searchWidgetField(UITextField *pTextBox);
    static CiOSTextWidget * searchWidgetView(UITextView * pTextView);
    // 全ての CiOSTextWidget の visible を設定する
    static void setAllVisible(bool visible);
    inline void setTouch(bool bTouch) {
        m_touched = bTouch;
        set_color();
    }
    
    inline int getMax() { return m_maxlen; }
    inline int getCharType() { return m_chartype; }
    const char * getFontName();
    UIFont * getFont();
    
private:
    bool createTextField(CONTROL _type, const char *_caption);
    bool createTextView(CONTROL _type, const char *_caption);
    
    void set_move(int x, int y, int width, int height);
    void set_color();
   
    CiOSFont                *   m_pFont;
    NSString                *   m_nsFontname;
    UITextField             *   m_pTextBox;
    CiOSTextView            *   m_pTextView;

    CiOSTextWidget          *   m_prev;
    CiOSTextWidget          *   m_next;
    
    bool                        m_bVisible;
    bool                        m_touched;
    unsigned int                m_cols[4];
    int                         m_maxlen;
    int                         m_chartype;
    int                         m_alignType;
    
    enum E_CONSOLE_TYPE
    {
        E_CONSOLE_TYPE_UITEXTVIEW = 0,
        E_CONSOLE_TYPE_UITEXTFIELD = 1,
    };
    int                         m_consoleType;  // UITextFieldかUITextViewのタイプ(今は固定でUITextViewにしておく)

    static CiOSTextWidget   *   ms_begin;
    static CiOSTextWidget   *   ms_end;
};

// iOS WebView
class CiOSWebWidget : public CiOSWidget
{
    friend class CiOSPlatform;
private:
    CiOSWebWidget(CiOSPlatform * pParent);
    virtual ~CiOSWebWidget();
public:
    bool create(CONTROL type, int id,
                const char * caption, int x, int y, int width, int height,
                const char * token, const char * region, const char * client,
                const char * consumerKey, const char * applicationId, const char * userID);

 	int getTextLength();
	bool getText(char * pBuf, int maxlen);
	bool setText(const char * string);
	void visible(bool bVisible);
	void enable(bool bEnable);
    
    void cmd(int cmd, ...);
    
    inline bool getJump() const { return m_bJump || m_bFirst; }
    inline void setFirst() { m_bFirst = false; }
    
    // 指定された UIWebView を持つ CiOSWebWidget を検索
    static CiOSWebWidget * searchWidget(WKWebView * pWebView);

private:
    void set_move(int x, int y, int width, int height);
    void set_bgcolor();
    
    bool              m_bJump;
    bool              m_bFirst;
    const char    *   m_pNowURL;
    CiOSWebView   *   m_pWebView;
    
    char              m_bufURL[1024];
    unsigned int      m_bgalpha;        // 背景色のアルファ値
    unsigned int      m_bgcolor;        // 背景色

    CiOSWebWidget           *   m_prev;
    CiOSWebWidget           *   m_next;
    
    static CiOSWebWidget    *   ms_begin;
    static CiOSWebWidget    *   ms_end;
};

class CiOSMovieWidget : public CiOSWidget
{
    friend class CiOSPlatform;
private:
    CiOSMovieWidget(CiOSPlatform * pParent);
    virtual ~CiOSMovieWidget();
public:
    bool create(CONTROL type, int id,
                const char * caption, int x, int y, int width, int height);
    
    int getTextLength();
	bool getText(char * pBuf, int maxlen);
	bool setText(const char * string);
	void visible(bool bVisible);
	void enable(bool bEnable);
    
    void cmd(int cmd, ...);
    int status();
    
    inline void setStatus(int stat) { m_status = stat; }
    
    static CiOSMovieWidget * getWidget(void * p);

private:
    void set_move(int x, int y, int width, int height);
   
    CONTROL                     m_type;

    const char              *   m_pNowPATH;
    CiOSMovieView           *   m_pMovieView;
    
    int                         m_status;
    CiOSMovieWidget         *   m_prev;
    CiOSMovieWidget         *   m_next;
    
    static CiOSMovieWidget  *   ms_begin;
    static CiOSMovieWidget  *   ms_end;
};

class CiOSActivityIndicator : public CiOSWidget
{
    friend class CiOSPlatform;
private:
    CiOSActivityIndicator(CiOSPlatform * pParent);
    virtual ~CiOSActivityIndicator();
public:
    bool create(CONTROL type, int id,
                const char * caption, int x, int y, int width, int height);
    
    int getTextLength();
	bool getText(char * pBuf, int maxlen);
	bool setText(const char * string);
	void visible(bool bVisible);
	void enable(bool bEnable);
    
    void cmd(int cmd, ...);
    int status();

private:
    void set_move(int x, int y, int width, int height);
    
    UIActivityIndicatorView         *   m_pActView;
    
    bool                                m_bVisible;
    int                                 m_status;
    float                               m_size;

    CiOSActivityIndicator           *   m_prev;
    CiOSActivityIndicator           *   m_next;
    
    static CiOSActivityIndicator    *   ms_begin;
    static CiOSActivityIndicator    *   ms_end;
};

#endif
