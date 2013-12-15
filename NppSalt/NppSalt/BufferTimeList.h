#include <windows.h>
#include <vector>

#ifdef UNICODE
#define generic_strtol wcstol
#define generic_strcpy wcscpy
#define generic_strncpy wcsncpy
#define generic_stricmp wcsicmp
#define generic_strncmp wcsncmp
#define generic_strnicmp wcsnicmp
#define generic_strncat wcsncat
#define generic_strchr wcschr
#define generic_atoi _wtoi
#define generic_itoa _itow
#define generic_atof _wtof
#define generic_strtok wcstok
#define generic_strftime wcsftime
#define generic_fprintf fwprintf
#define generic_sprintf swprintf
#define generic_sscanf swscanf
#define generic_fopen _wfopen
#define generic_fgets fgetws
#define generic_stat _wstat
#define generic_sprintf swprintf
#else
#define generic_strtol strtol
#define generic_strcpy strcpy
#define generic_strncpy strncpy
#define generic_stricmp stricmp
#define generic_strncmp strncmp
#define generic_strnicmp strnicmp
#define generic_strncat strncat
#define generic_strchr strchr
#define generic_atoi atoi
#define generic_itoa itoa
#define generic_atof atof
#define generic_strtok strtok
#define generic_strftime strftime
#define generic_fprintf fprintf
#define generic_sprintf sprintf
#define generic_sscanf sscanf
#define generic_fopen fopen
#define generic_fgets fgets
#define generic_stat _stat
#define generic_sprintf sprintf
#endif

class BufferTime    
{
public:
    TCHAR Path[MAX_PATH];
    FILETIME LastWriteTime;
    unsigned long BufferId;
};

class AutoLock
{
    CRITICAL_SECTION * _pSection;
public:
    AutoLock(CRITICAL_SECTION *pSection);
    virtual ~AutoLock();
};

using namespace std;
class BufferTimeList
{
private:
    vector<BufferTime*> _files;
    HWND _hwnd;
    CRITICAL_SECTION _sync;
public:
    BufferTimeList(HWND hwnd);
    virtual ~BufferTimeList();
    void Add(unsigned long bufferId);
    void Delete(unsigned long bufferId);
    void BeforeSave(unsigned long bufferId);
    void Monitor();
    void GetBufferFullPath(unsigned long bufferId, TCHAR *path);

    bool GetLastWriteTime(TCHAR * file, FILETIME *pTime);
    void ToLocalTime(FILETIME fileTime, TCHAR *localTime);

    static int MsgBox(HWND hwnd, TCHAR *text);
    static int ErrorBox(HWND hwnd, TCHAR *text);
    static int WarningBox(HWND hwnd, TCHAR *text);

private:
    BufferTime* Find(unsigned long bufferId);
    bool IsChanged(unsigned long bufferId, FILETIME *pTime);
    void FlashCaption(vector<TCHAR *> messages);
};
