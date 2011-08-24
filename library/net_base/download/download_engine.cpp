
#include "download_engine.h"

#include <WinInet.h>

#include "base/logging.h"
#include "base/synchronization/lock.h"

namespace
{

    DownloadEngine* engine = NULL;
    base::Lock lock;

    void InitUrlComponents(URL_COMPONENTSW* url_components)
    {
        DCHECK(url_components);

        memset(url_components, 0, sizeof(URL_COMPONENTSW));
        url_components->dwStructSize = sizeof(URL_COMPONENTSW);
        url_components->lpszScheme = new wchar_t[INTERNET_MAX_SCHEME_LENGTH];
        url_components->dwSchemeLength = INTERNET_MAX_SCHEME_LENGTH;
        url_components->lpszHostName = new wchar_t[INTERNET_MAX_HOST_NAME_LENGTH];
        url_components->dwHostNameLength = INTERNET_MAX_HOST_NAME_LENGTH;
        url_components->lpszUserName = new wchar_t[INTERNET_MAX_USER_NAME_LENGTH];
        url_components->dwUserNameLength = INTERNET_MAX_USER_NAME_LENGTH;
        url_components->lpszPassword = new wchar_t[INTERNET_MAX_PASSWORD_LENGTH];
        url_components->dwPasswordLength = INTERNET_MAX_PASSWORD_LENGTH;
        url_components->lpszUrlPath = new wchar_t[INTERNET_MAX_PATH_LENGTH];
        url_components->dwUrlPathLength = INTERNET_MAX_PATH_LENGTH;
    }

    void UninitUrlComponents(URL_COMPONENTSW* url_components)
    {
        DCHECK(url_components);

        delete[] url_components->lpszScheme;
        delete[] url_components->lpszHostName;
        delete[] url_components->lpszUserName;
        delete[] url_components->lpszPassword;
        delete[] url_components->lpszUrlPath;
        memset(url_components, 0, sizeof(URL_COMPONENTSW));
    }

}

class AddDownloadTask : public Task
{
public:
    AddDownloadTask(std::vector<std::wstring>* urls)
        : urls_(urls) {}
    virtual ~AddDownloadTask() { delete urls_; }

    virtual void Run()
    {
        LPVOID lpOutBuffer=NULL;
        DWORD dwSize = 0;

        for(std::vector<std::wstring>::iterator it(urls_->begin());
            it!=urls_->end(); ++it)
        {
            URL_COMPONENTSW url_components;
            InitUrlComponents(&url_components);
            if(InternetCrackUrlW(it->c_str(), it->length(), ICU_DECODE,
                &url_components))
            {
                UninitUrlComponents(&url_components);
            }

            HINTERNET connection = InternetOpenW(NULL, INTERNET_OPEN_TYPE_PRECONFIG,
                NULL, NULL, NULL);
             HINTERNET file = InternetOpenUrlW(connection, it->c_str(), NULL, 0,
                 INTERNET_FLAG_RELOAD, 0);

retry:

             // This call will fail on the first pass, because 
             // no buffer is allocated.
             if(!HttpQueryInfo(file,HTTP_QUERY_RAW_HEADERS_CRLF,
                 (LPVOID)lpOutBuffer,&dwSize,NULL))
             {
                 if (GetLastError()==ERROR_HTTP_HEADER_NOT_FOUND)
                 {
                     // Code to handle the case where the header isn't available.
                     return;
                 }		
                 else
                 {
                     // Check for an insufficient buffer.
                     if (GetLastError()==ERROR_INSUFFICIENT_BUFFER)
                     {
                         // Allocate the necessary buffer.
                         lpOutBuffer = new wchar_t[dwSize];

                         // Retry the call.
                         goto retry;				
                     }		
                     else
                     {
                         // Error handling code.
                         return;
                     }		
                 }		
             }
        }
    }

private:
    std::vector<std::wstring>* urls_;
};


NET_BASE_PUBLIC IDownloadEngine* StartDownloadEngine()
{
    base::AutoLock locker(lock);
    DCHECK(!engine) << "download engine has already started";
    if(!engine)
    {
        engine = new DownloadEngine();
        return engine;
    }
    return NULL;
}

DownloadEngine::DownloadEngine()
{
    const char* kThreadName = "CrDownloadMain";
    main_thread_.reset(new base::Thread(kThreadName));
    CHECK(main_thread_->Start());
}

DownloadEngine::~DownloadEngine()
{
    DCHECK(engine);

    base::AutoLock locker(lock);
    engine = NULL;
}

void DownloadEngine::Shutdown()
{
    DCHECK(CalledOnValidThread());

    Stop();

    main_thread_.reset();

    delete this;
}

void DownloadEngine::Stop()
{
    DCHECK(CalledOnValidThread());
}

void DownloadEngine::AddTask(__int64 id, const wchar_t* const* urls,
                             int urls_count)
{
    DCHECK(CalledOnValidThread());

    DCHECK_GT(urls_count, 0);

    std::vector<std::wstring>* v = new std::vector<std::wstring>();
    for(int i=0; i<urls_count; ++i)
    {
        v->push_back(urls[i]);
    }

    main_thread_->message_loop()->PostTask(new AddDownloadTask(v));
}