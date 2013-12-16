/*
Copyright(c) 2013 Peter Gu.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files(the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sub - license, and / or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

This program is free software; you can redistribute it and / or modify it under the terms of the
GNU General Public License as published by the Free Software Foundation; either version 2 of
the License, or(at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program;
if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
Boston, MA 02110 - 1301 USA.
*/
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

BufferTime * BufferTimeList::Find(unsigned long bufferId)
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

    BufferTime  *pFile = Find(bufferId);
    if (pFile == NULL)
    {
        pFile = new BufferTime();
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
        BufferTime *pFile = _files[idx];
        _files.erase(_files.begin() + idx);
        delete pFile;
    }
}

bool BufferTimeList::IsChanged(unsigned long bufferId, FILETIME *pTime)
{
    AutoLock lock(&_sync);
    BufferTime *pFile = Find(bufferId);
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

    BufferTime *pFile = Find(bufferId);

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
    static const TCHAR * format = TEXT("Your changes have NOT been saved because \"%s\" was modified and saved to disk by another program (%s).\n\nThe file has now been reloaded and your changes have been copied to the \"%s\" tab.");
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

    // flash outside of the lock as otherwise it will block messgages which in-turn causes deadlock
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
    generic_sprintf(localTime, TEXT("at %02d:%02d:%02d on %02d/%02d/%d"), 
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
