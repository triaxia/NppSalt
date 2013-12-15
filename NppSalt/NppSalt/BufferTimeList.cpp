#include "BufferTimeList.h"
#include "PluginDefinition.h"
#include "menuCmdID.h"

AutoLock::AutoLock(CRITICAL_SECTION *pSection)
{
    _pSection = pSection;
    EnterCriticalSection(_pSection);
}
AutoLock::~AutoLock()
{
    LeaveCriticalSection(_pSection);
}

BufferTimeList::BufferTimeList(HWND hwnd)
{
    _hwnd = hwnd;
    InitializeCriticalSection(&_sync);
}

BufferTimeList::~BufferTimeList()
{
    AutoLock lock(&_sync);
    for (int idx = 0; idx < _files.size(); idx++)
        delete _files[idx];
}

BufferTime     * BufferTimeList::Find(unsigned long bufferId)
{
    AutoLock lock(&_sync);
    for (int idx = 0; idx < _files.size(); idx++)
    if (_files[idx]->BufferId == bufferId)
        return _files[idx];
    return NULL;
};

void BufferTimeList::Add(unsigned long bufferId)
{
    AutoLock lock(&_sync);
    TCHAR path[MAX_PATH];
    GetBufferFullPath(bufferId, path);

    BufferTime     *pFile = Find(bufferId);
    if (pFile == NULL)
    {
        pFile = new BufferTime    ();
        pFile->BufferId = bufferId;
        generic_strcpy(pFile->Path, path);
        if (GetLastWriteTime(path, &pFile->LastWriteTime))
            _files.push_back(pFile);
        else
            delete pFile;
    }
    else
    {
        generic_strcpy(pFile->Path, path);
        GetLastWriteTime(path, &pFile->LastWriteTime);
    }
}

void BufferTimeList::Delete(unsigned long bufferId)
{
    AutoLock lock(&_sync);
    for (int idx = 0; idx < _files.size(); idx++)
    if (_files[idx]->BufferId == bufferId)
    {
        BufferTime     *pFile = _files[idx];
        _files.erase(_files.begin() + idx);
        delete pFile;
    }
}

bool BufferTimeList::IsChanged(unsigned long bufferId, FILETIME *pTime)
{
    AutoLock lock(&_sync);
    BufferTime     *pFile = Find(bufferId);
    if (pFile == NULL)
        return false;
    return GetLastWriteTime(pFile->Path, pTime) && memcmp(pTime, &pFile->LastWriteTime, sizeof(FILETIME)) != 0;
}

void BufferTimeList::BeforeSave(unsigned long bufferId)
{
    AutoLock lock(&_sync);
    FILETIME time;
    if (!IsChanged(bufferId, &time))
        return;

    TCHAR path[MAX_PATH];
    GetBufferFullPath(bufferId, path);

    BufferTime     *pFile = Find(bufferId);

    SendMessage(_hwnd, WM_COMMAND, IDM_EDIT_SELECTALL, 0);
    SendMessage(_hwnd, WM_COMMAND, IDM_EDIT_COPY, 0);
    SendMessage(_hwnd, WM_COMMAND, IDM_EDIT_DELETE, 0);
    SendMessage(_hwnd, NPPM_RELOADFILE, 0, (LPARAM) pFile->Path); // 0 - no alert
    SendMessage(_hwnd, WM_COMMAND, IDM_FILE_NEW, 0);
    SendMessage(_hwnd, WM_COMMAND, IDM_EDIT_PASTE, 0);

    TCHAR localTime[256];
    ToLocalTime(time, localTime);

    TCHAR newTitle[256];
    SendMessage(_hwnd, NPPM_GETFULLCURRENTPATH, 256, (LPARAM) newTitle);
    TCHAR buffer[1024];
    static const TCHAR * format = TEXT("\"%s\" has been modified by another program (%s).\n\nIt has been re-loaded from the disk.\n\nYour changes have been copied to the \"%s\" window.");
    generic_sprintf(buffer, format, pFile->Path, localTime, newTitle);
    ErrorBox(_hwnd, buffer);
}

void BufferTimeList::Monitor()
{
    vector<TCHAR *> messages;
    try
    {
        AutoLock lock(&_sync);
        for (int idx = 0; idx < _files.size(); idx++)
        {
            FILETIME time;
            if (IsChanged(_files[idx]->BufferId, &time))
            {
                TCHAR localTime[256];
                ToLocalTime(time, localTime);
                TCHAR *pbuffer = new TCHAR[1024];
                generic_sprintf(pbuffer, TEXT("\"%s\" has been modified by another program (%s)."), _files[idx]->Path, localTime);
                messages.push_back(pbuffer);
            }
        }
    }
    catch (...)
    {
        // ignore all
    };

    // flash outside of locks as otherwise it will block messgages which in-turn causes deadlock
    if (messages.size() > 0)
        FlashCaption(messages);
}

void BufferTimeList::FlashCaption(vector<TCHAR *> messages)
{
    // save
    TCHAR title[1024];
    GetWindowText(_hwnd, title, 1024);

    for (int idx = 0; idx < messages.size(); idx++)
    {
        int count = 10;
        while (--count >= 0)
        {
            SetWindowText(_hwnd, messages[idx]);
            FlashWindow(_hwnd, TRUE);
            Sleep(500);
        }
        delete [] messages[idx];
    }

    // restore 
    SetWindowText(_hwnd, title);
    FlashWindow(_hwnd, FALSE);
}

bool
BufferTimeList::GetLastWriteTime(TCHAR * file, FILETIME *pTime)
{
    HANDLE hFile = CreateFile(file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        //TCHAR buffer[1024];
        //generic_sprintf(buffer, TEXT("OpenFile failed: %s"), file);
        //ErrorBox(_hwnd, buffer);
        return false;
    }

    if (!GetFileTime(hFile, NULL, NULL, pTime))
    {
        //TCHAR buffer[1024];
        //generic_sprintf(buffer, TEXT("GetFileTime failed: %s"), file);
        //ErrorBox(_hwnd, buffer);
        ::CloseHandle(hFile);
        return false;
    }

    ::CloseHandle(hFile);
    return true;
}

void BufferTimeList::ToLocalTime(FILETIME fileTime, TCHAR *localTime)
{
    SYSTEMTIME stUTC, stLocal;
    FileTimeToSystemTime(&fileTime, &stUTC);
    SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);
    generic_sprintf(localTime, TEXT("%02d:%02d:%02d %02d/%02d/%d"), 
        stLocal.wHour, stLocal.wMinute, stLocal.wSecond, stLocal.wDay, stLocal.wMonth, stLocal.wYear);
}

void BufferTimeList::GetBufferFullPath(unsigned long bufferId, TCHAR *path)
{
    ::SendMessage(_hwnd, NPPM_GETFULLPATHFROMBUFFERID, bufferId, (LPARAM) path);
}

int BufferTimeList::MsgBox(HWND hwnd, TCHAR *text)
{
    return MessageBox(hwnd, text, TEXT("NppSalt"), MB_OK + MB_ICONINFORMATION);
}

int BufferTimeList::ErrorBox(HWND hwnd, TCHAR *text)
{
    return MessageBox(hwnd, text, TEXT("NppSalt"), MB_OK + MB_ICONERROR);
}

int BufferTimeList::WarningBox(HWND hwnd, TCHAR *text)
{
    return MessageBox(hwnd, text, TEXT("NppSalt"), MB_OK + MB_ICONWARNING);
}
