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

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  reasonForCall, 
                       LPVOID lpReserved )
{
    switch (reasonForCall)
    {
      case DLL_PROCESS_ATTACH:
        pluginInit(hModule);
        break;

      case DLL_PROCESS_DETACH:
        pluginCleanUp();
        break;

      case DLL_THREAD_ATTACH:
        break;

      case DLL_THREAD_DETACH:
        break;
    }

    return TRUE;
}

extern NppData nppData;
extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
    nppData = notpadPlusData;
    commandMenuInit();
}

extern "C" __declspec(dllexport) const TCHAR * getName()
{
    return NPP_PLUGIN_NAME;
}

extern FuncItem funcItem[nbFunc];
extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *nbF)
{
    *nbF = nbFunc;
    return funcItem;
}

extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode)
{
    switch (notifyCode->nmhdr.code) 
    {
        case NPPN_SHUTDOWN:
        {
            commandMenuCleanUp();
        }
        break;

        case NPPN_FILEBEFORESAVE:
        {
            BeforeSave(notifyCode);
        }
        break;

        case NPPN_FILESAVED:
        {
            FileSaved(notifyCode);
        }
        break;

        case NPPN_FILEOPENED:
        {
            FileOpened(notifyCode);
        }
        break;

        case NPPN_FILECLOSED:
        {
            FileClosed(notifyCode);
        }
        break;

        // occurs after a save or reload is done
        case SCN_SAVEPOINTREACHED:
        {
            SavePointReached(notifyCode);
        }
        break;

        default:
            return;
    }
}


// Here you can process the Npp Messages 
// I will make the messages accessible little by little, according to the need of plugin development.
// Please let me know if you need to access to some messages :
// http://sourceforge.net/forum/forum.php?forum_id=482781
//
extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
    /*
    if (Message == )
    {
        ::MessageBox(NULL, TEXT("move"), TEXT(""), MB_OK);
    }
    */
    return TRUE;
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode()
{
    return TRUE;
}
#endif //UNICODE
