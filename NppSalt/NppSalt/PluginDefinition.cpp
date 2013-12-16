//this file is part of notepad++
//Copyright (C)2003 Don HO <donho@altern.org>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "PluginDefinition.h"
#include "menuCmdID.h"
#include "BufferTimeList.h"

//
// The plugin data that Notepad++ needs
//
FuncItem funcItem[nbFunc];
NppData nppData;
BufferTimeList *BufferList = NULL;

void AboutDlg()
{
    BufferTimeList::MsgBox(nppData._nppHandle, 
        TEXT("NppSalt simply prevents the Save operation from overwriting \n changes made by other programs. \n\nBy Peter Gu from Triaxia Limited."));
}

void pluginInit(HANDLE hModule)
{
}

void pluginCleanUp()
{
}

void commandMenuInit()
{
    setCommand(0, TEXT("About..."), AboutDlg, NULL, false);
    BufferList = new BufferTimeList(nppData._nppHandle);
    StartMonitoring();
}

void commandMenuCleanUp()
{
    StopMonitoring();
    delete BufferList;
}

bool setCommand(size_t index,                   // zero based number to indicate the order of command
                TCHAR *cmdName,                 // the command name that you want to see in plugin menu
                PFUNCPLUGINCMD pFunc,           // the symbol of function (function pointer) associated with this command. The body should be defined below. See Step 4.
                ShortcutKey *sk,                // optional.Define a shortcut to trigger this command
                bool check0nInit)               // optional. Make this menu item be checked visually
{
    if (index >= nbFunc)
        return false;

    if (!pFunc)
        return false;

    lstrcpy(funcItem[index]._itemName, cmdName);
    funcItem[index]._pFunc = pFunc;
    funcItem[index]._init2Check = check0nInit;
    funcItem[index]._pShKey = sk;

    return true;
}

void FileOpened(SCNotification *notifyCode)
{
    BufferList->Add(notifyCode->nmhdr.idFrom);
}

void FileSaved(SCNotification *notifyCode)
{
    BufferList->Add(notifyCode->nmhdr.idFrom);
}

void FileClosed(SCNotification *notifyCode)
{
    BufferList->Delete(notifyCode->nmhdr.idFrom);
}

void BeforeSave(SCNotification *notifyCode)
{
    BufferList->BeforeSave(notifyCode->nmhdr.idFrom);
}

void SavePointReached(SCNotification *notifyCode)
{
    unsigned long bufferId = SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);
    BufferList->Add(bufferId);
}

HANDLE hThread = NULL;
bool bMonitoring = false;
DWORD MonitorProc(LPVOID lpdwThreadParam)
{
    bMonitoring = true;
    while (bMonitoring)
    {
        BufferList->Monitor();
        Sleep(10000);
    }
    CloseHandle(hThread);
    return 0;
}

void StartMonitoring()
{
    DWORD tid;
    hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) &MonitorProc, NULL, 0, &tid);
    bMonitoring = true;
}

void StopMonitoring()
{
    bMonitoring = false;
}
