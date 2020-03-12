// Implements Micro Download system

#include <vector>

#include "CPFInterface.h"
#include "CKLBScriptEnv.h"
#include "CKLBLuaEnv.h"
#include "CKLBUtility.h"
#include "MultithreadedNetwork.h"

#include "DownloadQueue.h"

static IFactory::DEFCMD cmd[] = {
		{0, 0}
};

static CKLBTaskFactory<MicroDownload> factory("MicroDownload", CLS_MICRODL, cmd);

u32
MicroDownload::getClassID()
{
	return CLS_MICRODL;
}

MicroDownload*
MicroDownload::create()
{
	MicroDownload* pTask = KLBNEW(MicroDownload);
	if (!pTask) return NULL;
	CLuaState l = null;
	if (!pTask->initScript(l)) return NULL;
	return pTask;
}

bool
MicroDownload::initScript(CLuaState& lua)
{
	return regist(NULL, P_NORMAL);
}

void
MicroDownload::execute(u32 deltaT)
{
	MainLoop(deltaT);
}

void
MicroDownload::die()
{
	DeleteAll();
}


MicroDLQueue MicroDownload::queue_list;

void MicroDownload::MainLoop(int )
{
	if (queue_list.empty()) return;

	CKLBScriptEnv& scriptenv = CKLBScriptEnv::getInstance();

	for(MicroDLQueue::iterator i = queue_list.begin(); i != queue_list.end();)
	{
		MDLData* mdl = *i;
		int status_code = mdl->http->getHttpState();

		if(mdl->http->httpRECV() || status_code != (-1))
		{
			// Downloaded, but unsure if it's okay
			if(status_code != 200)
				goto error_mdl;

			i = queue_list.erase(i);

			// Write to file.
			u8* body = mdl->http->getRecvResource();

			if(body == NULL)
				goto error_mdl2;
			
			ITmpFile *f = CPFInterface::getInstance().platform().openTmpFile(mdl->filename);
			
			if(f == NULL)
				goto error_mdl2;

			size_t bodylen = body ? mdl->http->getSize() : 0;
			f->writeTmp(body, bodylen);
			// don't use KLBDELETE, platform uses new
			delete f;
			
			// callback
			scriptenv.call_eventMdlFinish(mdl->callback, mdl->filename, mdl->url, true, status_code);

			delete mdl;
		}
		else if(mdl->http->m_threadStop == 1 && mdl->http->getHttpState() == (-1))
		{
			// Failed.
			error_mdl:
			i = queue_list.erase(i);

			error_mdl2:
			DEBUG_PRINT("MDL Error 2 occured.");
			scriptenv.call_eventMdlFinish(mdl->callback, mdl->filename, mdl->url, false, 500);

			delete mdl;
		}
		else
			++i;
	}
}

void MicroDownload::Queue(const char* callback, const char* filename, const char* url)
{
	MDLData* a = new MDLData(callback, filename, url);

	DEBUG_PRINT("Micro Download: %s ; %s ; %s", callback, filename, url);

	queue_list.push_back(a);
}

void MicroDownload::DeleteAll()
{
	CKLBScriptEnv& scriptenv = CKLBScriptEnv::getInstance();

	while(queue_list.empty() == false)
	{
		MDLData* mdl = queue_list.back();

		queue_list.pop_back();
		scriptenv.call_eventMdlFinish(mdl->callback, mdl->filename, mdl->url, false, -1);
		
		delete mdl;
	}
}
