/**
 * Debugmode Frameserver
 * Copyright (C) 2002-2009 Satish Kumar, All Rights Reserved
 * http://www.debugmode.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <windows.h>
#include <winsock.h>
#include <dfsc.h>
#include <stdio.h>
#include <tchar.h>
#include <shlobj.h>
#include "fscommon_resource.h"

char networkServer[MAX_PATH];
DWORD networkPort = 8278;
char signpostPath[MAX_PATH * 2];
SOCKET sock;
HINSTANCE ghInst, ghResInst;
DfscData* vars = NULL;
HANDLE varFile = NULL;
HANDLE videoEncEvent, videoDecEvent;
HANDLE audioEncEvent, audioDecEvent;
BOOL stopServing = FALSE;
HWND servingdlg;
char installDir[MAX_PATH] = "C:\\Program Files\\Debugmode\\Frameserver";
#define APPNAME "Debugmode FrameServer"

// ------------------------------- copied from fscommon.cpp ----------------------------------

bool LoadCommonResource() {
  HKEY key;

  RegCreateKeyEx(HKEY_LOCAL_MACHINE, "Software\\DebugMode\\FrameServer", 0, 0,
      REG_OPTION_NON_VOLATILE, KEY_READ, 0, &key, 0);
  if (key) {
    DWORD size = sizeof(installDir);
    RegQueryValueEx(key, "InstallDir", 0, 0, (LPBYTE)installDir, &size);
    installDir[size] = 0;
  }
  TCHAR libpath[MAX_PATH];
  _tcscpy(libpath, installDir);
  _tcscat(libpath, _T("\\fscommon.dll"));
  ghResInst = LoadLibrary(libpath);
  if (!ghResInst) {
    MessageBox(NULL, "Unable to load \"fscommon.dll\". Please reinstall.", APPNAME, MB_OK | MB_ICONEXCLAMATION);
    return false;
  }
  return true;
}

void SetFsIconForWindow(HWND wnd) {
  SendMessage(wnd, WM_SETICON, TRUE, (LPARAM)LoadIcon(ghResInst, MAKEINTRESOURCE(IDI_FRAMESERVER)));
  SendMessage(wnd, WM_SETICON, FALSE, (LPARAM)LoadIcon(ghResInst, MAKEINTRESOURCE(IDI_FRAMESERVER)));
}

BOOL CALLBACK WritingSignpostDlgProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
  case WM_INITDIALOG:
    SetFsIconForWindow(dlg);
    return TRUE;
  case WM_COMMAND:
    if (LOWORD(wp) == IDCANCEL) {
      stopServing = TRUE;
      EndDialog(dlg, IDCANCEL);
    }
  }
  return FALSE;
}

BOOL CALLBACK AboutDlgProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
  char copyright[1024];

  switch (msg) {
  case WM_INITDIALOG: LoadString(ghResInst, IDS_ABOUTDLG_COPYRIGHT, copyright, 1024);
    ::SetDlgItemText(dlg, IDC_WARNING, copyright);
    SetFsIconForWindow(dlg);
    return TRUE;
  case WM_COMMAND:
    if (LOWORD(wp) == IDOK || LOWORD(wp) == IDCANCEL)
      EndDialog(dlg, 0);
  }
  return FALSE;
}

// --------------------------------- end copy ----------------------------------------------

int SocketReadBlock(SOCKET s, void* data, int len) {
  int i = len;

  while (len) {
    int rval = recv(s, (char*)data, len, 0);
    if (rval == 0 || rval == SOCKET_ERROR)
      return rval;
    len -= rval;
    data = ((LPBYTE)data) + rval;
  }
  return i;
}

void SocketReadVars(SOCKET s, ...) {
  int count = 0, totsize = 0;
  void* var[8];
  int varsize[8];
  BYTE varbuf[64];
  va_list marker;

  va_start(marker, s);
  for (count = 0; ; count++) {
    var[count] = (void*)va_arg(marker, int);
    if (var[count] == NULL) break;
    varsize[count] = va_arg(marker, int);
    totsize += varsize[count];
  }
  va_end(marker);
  SocketReadBlock(s, varbuf, totsize);
  totsize = 0;
  for (int i = 0; i < count; i++) {
    memcpy(var[i], varbuf + totsize, varsize[i]);
    totsize += varsize[i];
  }
}

void SendReadAudioFrameIndex(SOCKET s, BOOL readAudio, DWORD frameIndex) {
  BYTE varbuf[32];
  int totsize = 0;

  *(BOOL*)(varbuf + totsize) = readAudio; totsize += sizeof(readAudio);
  *(DWORD*)(varbuf + totsize) = frameIndex; totsize += sizeof(frameIndex);
  send(s, (const char*)varbuf, totsize, 0);
}

BOOL CALLBACK NetClientOptionsDlgProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
  HKEY key = 0;

  switch (msg) {
  case WM_INITDIALOG:
    SetFsIconForWindow(dlg);
    memset(networkServer, 0, sizeof(networkServer));
    memset(signpostPath, 0, sizeof(signpostPath));
    strcpy(networkServer, "127.0.0.1");
    signpostPath[0] = 0;
    SHGetSpecialFolderPath(NULL, signpostPath, CSIDL_DESKTOPDIRECTORY, FALSE);
    strcat(signpostPath, "\\netserved.avi");
    RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\DebugMode\\FrameServer", 0, 0,
        REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, &key, 0);
    if (key) {
      DWORD size = sizeof(networkServer);
      RegQueryValueEx(key, "networkServer", 0, 0, (LPBYTE)&networkServer, &size);
      size = sizeof(networkPort);
      RegQueryValueEx(key, "networkPort", 0, 0, (LPBYTE)&networkPort, &size);
      size = sizeof(signpostPath);
      RegQueryValueEx(key, "signpostPath", 0, 0, (LPBYTE)&signpostPath, &size);
      RegCloseKey(key);
    }
    SetDlgItemText(dlg, IDC_EDIT1, networkServer);
    SetDlgItemInt(dlg, IDC_EDIT2, networkPort, FALSE);
    SetDlgItemText(dlg, IDC_EDIT3, signpostPath);
    return TRUE;
  case WM_COMMAND:
    if (LOWORD(wp) == IDC_BROWSE) {
      OPENFILENAME ofn;
      memset(&ofn, 0, sizeof(ofn));
      ofn.lStructSize = sizeof(ofn);
      ofn.hwndOwner = dlg;
      char filter[] = "AVI Files (*.avi)|*.avi|All Files (*.*)|*.*||";
      filter[17] = filter[23] = filter[39] = filter[43] = filter[44] = 0;
      ofn.lpstrFilter = (const char*)filter;
      ofn.lpstrFile = signpostPath;
      ofn.nMaxFile = sizeof(signpostPath) / sizeof(signpostPath[0]);
      ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT;
      if (GetSaveFileName(&ofn))
        SetDlgItemText(dlg, IDC_EDIT3, signpostPath);
    }
    if (LOWORD(wp) == IDOK) {
      GetDlgItemText(dlg, IDC_EDIT3, signpostPath, sizeof(signpostPath));
      GetDlgItemText(dlg, IDC_EDIT1, networkServer, sizeof(networkServer));
      networkPort = GetDlgItemInt(dlg, IDC_EDIT2, NULL, FALSE);
      sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
      if (sock) {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = PF_INET;
        addr.sin_port = htons((USHORT)networkPort);
        addr.sin_addr.S_un.S_addr = inet_addr(networkServer);
        if (addr.sin_addr.S_un.S_addr == INADDR_NONE) {
          struct hostent* hostEnt = gethostbyname(networkServer);
          if (hostEnt)
            addr.sin_addr.S_un.S_addr = *(DWORD*)(hostEnt->h_addr);
        }
        if (connect(sock, (const struct sockaddr*)&addr, sizeof(addr)) != 0) {
          MessageBox(dlg, "Unable to connect to source machine.\n\n"
              "Please check the source machine's address and port.\n"
              "Also check if the source machine has the FrameServer running.", APPNAME, MB_OK);
          closesocket(sock);
          sock = NULL;
        } else {
          EnableWindow(GetDlgItem(dlg, IDC_EDIT1), FALSE);
          EnableWindow(GetDlgItem(dlg, IDC_EDIT2), FALSE);
          EnableWindow(GetDlgItem(dlg, IDC_EDIT3), FALSE);
          EnableWindow(GetDlgItem(dlg, IDC_BROWSE), FALSE);
          EnableWindow(GetDlgItem(dlg, IDOK), FALSE);
          EnableWindow(GetDlgItem(dlg, IDCANCEL), FALSE);
          bool success = false;
          do {
            DWORD size = 0;
            if (SocketReadBlock(sock, &size, sizeof(size)) != sizeof(size)) break;
            varFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,
                sizeof(DfscData) + size, "DfscNetData");
            if (varFile) {
              if (GetLastError() == ERROR_ALREADY_EXISTS) {
                CloseHandle(varFile);
                varFile = NULL;
              }
            }
            vars = (DfscData*)MapViewOfFile(varFile, FILE_MAP_WRITE, 0, 0, 0);
            if (!vars) break;
            if (SocketReadBlock(sock, vars, sizeof(DfscData)) != sizeof(DfscData)) break;
            if (SocketReadBlock(sock, &size, sizeof(size)) != sizeof(size)) break;
            BYTE fileData[32768];
            HANDLE hfile = CreateFile(signpostPath, GENERIC_WRITE, 0, 0, CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL, NULL);
            if (!hfile) break;
            HWND progdlg = CreateDialog(ghResInst, MAKEINTRESOURCE(IDD_WRITINGSIGNPOST),
                dlg, WritingSignpostDlgProc);
            SendMessage(GetDlgItem(progdlg, IDC_PROGRESS1), PBM_SETRANGE32, 0, size);
            SendMessage(GetDlgItem(progdlg, IDC_PROGRESS1), PBM_SETPOS, 0, 0);
            ShowWindow(progdlg, SW_SHOW);
            for (DWORD i = 0; i < size && !stopServing;) {
              DWORD cursize = min(size - i, 32768);
              if (SocketReadBlock(sock, fileData, cursize) != (int)cursize) break;
              DWORD written = 0;
              WriteFile(hfile, fileData, cursize, &written, NULL);
              if (written != cursize) break;
              i += cursize;
              SendMessage(GetDlgItem(progdlg, IDC_PROGRESS1), PBM_SETPOS, i, 0);
              MSG msg;
              while (PeekMessage(&msg, 0, 0, 0, 1)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
              }
            }
            DestroyWindow(progdlg);
            CloseHandle(hfile);
            if (i != size) break;
            success = true;
          } while (0);
          if (!success) {
            if (!stopServing) {
              MessageBox(dlg, "Initialization error!\n\n"
                  "1) Check the source machine's address and port.\n"
                  "2) Close and reopen all applications that were using the frameserved AVI earlier.\n"
                  "3) Also check if the signpost path specified is valid.", APPNAME, MB_OK);
            }
            stopServing = FALSE;
            UnmapViewOfFile(vars);
            CloseHandle(varFile);
            closesocket(sock);
            sock = NULL;
            varFile = NULL;
            vars = NULL;
          }
          EnableWindow(GetDlgItem(dlg, IDC_EDIT1), TRUE);
          EnableWindow(GetDlgItem(dlg, IDC_EDIT2), TRUE);
          EnableWindow(GetDlgItem(dlg, IDC_EDIT3), TRUE);
          EnableWindow(GetDlgItem(dlg, IDC_BROWSE), TRUE);
          EnableWindow(GetDlgItem(dlg, IDOK), TRUE);
          EnableWindow(GetDlgItem(dlg, IDCANCEL), TRUE);
        }
      }
      if (sock) {
        RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\DebugMode\\FrameServer", 0, 0, REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS, 0, &key, 0);
        if (key) {
          DWORD size = sizeof(networkServer);
          RegSetValueEx(key, "networkServer", 0, REG_BINARY, (LPBYTE)&networkServer, size);
          size = sizeof(networkPort);
          RegSetValueEx(key, "networkPort", 0, REG_BINARY, (LPBYTE)&networkPort, size);
          size = sizeof(signpostPath);
          RegSetValueEx(key, "signpostPath", 0, REG_BINARY, (LPBYTE)&signpostPath, size);
          RegCloseKey(key);
        }
        EndDialog(dlg, LOWORD(wp));
      }
    }
    if (LOWORD(wp) == IDCANCEL)
      EndDialog(dlg, LOWORD(wp));
    break;
  }
  return FALSE;
}

BOOL CALLBACK NetClientServingDlgProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
  case WM_INITDIALOG:
    SetFsIconForWindow(dlg);
    break;
  case WM_COMMAND:
    if (LOWORD(wp) == IDCANCEL)
      stopServing = TRUE;
    if (LOWORD(wp) == IDC_ABOUT)
      DialogBox(ghResInst, MAKEINTRESOURCE(IDD_DFABOUTDLG), dlg, AboutDlgProc);
    break;
  }
  return FALSE;
}

int APIENTRY WinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow) {
  ghInst = hInstance;

  WORD wVersionRequested;
  WSADATA wsaData;
  wVersionRequested = MAKEWORD(1, 1);
  if (WSAStartup(wVersionRequested, &wsaData) != 0) {
    MessageBox(NULL, "Unable to initialize network. Please check if you have latest network drivers.", APPNAME, MB_OK);
    return 0;
  }
  if (!LoadCommonResource())
    return 0;

  if (DialogBox(ghResInst, MAKEINTRESOURCE(IDD_NETCLIENT_OPTIONS), NULL, NetClientOptionsDlgProc) != IDOK)
    return 0;

  strcpy(vars->videoEncSemName, "DfscNetVideoEncSemName");
  strcpy(vars->videoEncEventName, "DfscNetVideoEncEventName");
  strcpy(vars->videoDecEventName, "DfscNetVideoDecEventName");
  strcpy(vars->audioEncSemName, "DfscNetAudioEncSemName");
  strcpy(vars->audioEncEventName, "DfscNetAudioEncEventName");
  strcpy(vars->audioDecEventName, "DfscNetAudioDecEventName");
  videoEncEvent = CreateEvent(NULL, TRUE, FALSE, vars->videoEncEventName);
  videoDecEvent = CreateEvent(NULL, FALSE, FALSE, vars->videoDecEventName);
  audioEncEvent = CreateEvent(NULL, TRUE, FALSE, vars->audioEncEventName);
  audioDecEvent = CreateEvent(NULL, FALSE, FALSE, vars->audioDecEventName);
  vars->encStatus = 0;
  vars->videoFrameIndex = -1;
  vars->audioFrameIndex = -1;
  stopServing = FALSE;
  servingdlg = CreateDialog(ghResInst, MAKEINTRESOURCE(IDD_NETCLIENT_SERVING), NULL, NetClientServingDlgProc);
  SetDlgItemText(servingdlg, IDC_EDIT1, signpostPath);
  ShowWindow(servingdlg, SW_SHOW);

  BOOL readAudio = FALSE;
  DWORD frameIndex = 0;
  send(sock, (const char*)&readAudio, sizeof(readAudio), 0);
  send(sock, (const char*)&frameIndex, sizeof(frameIndex), 0);

  DWORD framesread = 0, samplesread = 0;
  DWORD lasttime = timeGetTime() - 2000;
  BYTE* audioBuffer = new BYTE[vars->wfx.nSamplesPerSec * vars->wfx.nBlockAlign * 2];
  DWORD audioBufferStartPos = (DWORD)-1;
  while (!stopServing) {
    char str[64];
    DWORD curtime = timeGetTime();
    DWORD msec = curtime - lasttime;
    if (msec >= 1000) {
      sprintf(str, "%.2lf frames/sec", ((double)framesread * 1000) / msec);
      SetDlgItemText(servingdlg, IDC_VIDEOSTATS, str);
      sprintf(str, "%.2lf samples/sec", ((double)samplesread * 1000) / msec);
      SetDlgItemText(servingdlg, IDC_AUDIOSTATS, str);
      framesread = 0;
      samplesread = 0;
      lasttime = curtime;
    }

    HANDLE evs[2] = { videoEncEvent, audioEncEvent };
    MsgWaitForMultipleObjects(2, evs, FALSE, INFINITE, QS_ALLEVENTS | QS_SENDMESSAGE | QS_MOUSE | QS_MOUSEBUTTON);
    MSG msg;
    while (PeekMessage(&msg, 0, 0, 0, TRUE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    if (stopServing)
      break;

    DWORD size;
    bool noop = true;
#ifdef _DEBUG
    DWORD sttime = timeGetTime(), firsttm = 0;
#endif
    if (WaitForSingleObject(videoEncEvent, 0) == WAIT_OBJECT_0) {
      // OutputDebugString("0\n");
#ifdef _DEBUG
      char str[32]; strcpy(str, "nc> video - ");
      itoa(vars->videoFrameIndex, str + strlen(str), 10);
      strcat(str, "\n");
      sprintf(str, "c1 %f - ", (float)timeGetTime() / 1000.0f);
      OutputDebugString(str);
#endif
      ResetEvent(videoEncEvent);

      SocketReadVars(sock, &readAudio, sizeof(readAudio), &frameIndex, sizeof(frameIndex), &size, sizeof(size), 0);
      if (readAudio || frameIndex != vars->videoFrameIndex) {
#ifdef _DEBUG
        OutputDebugString("getting diff frame ");
#endif
        BYTE* data = new BYTE[size];
        SocketReadBlock(sock, data, size);
        delete[] data;
        readAudio = FALSE;
        frameIndex = vars->videoFrameIndex;
        send(sock, (const char*)&readAudio, sizeof(readAudio), 0);
        send(sock, (const char*)&frameIndex, sizeof(frameIndex), 0);
        SocketReadVars(sock, &readAudio, sizeof(readAudio), &frameIndex, sizeof(frameIndex), &size, sizeof(size), 0);
      }

#ifdef _DEBUG
      sprintf(str, "c2 %f\n", (float)timeGetTime() / 1000.0f);
      OutputDebugString(str);
#endif
      frameIndex++;
      send(sock, (const char*)&readAudio, sizeof(readAudio), 0);
      send(sock, (const char*)&frameIndex, sizeof(frameIndex), 0);

      SocketReadBlock(sock, ((LPBYTE)vars) + vars->videooffset, size);
      OutputDebugString("got\n");
      vars->videoBytesRead = size;
      SetEvent(videoDecEvent);

      framesread++;
      noop = false;
    }
    if (WaitForSingleObject(audioEncEvent, 0) == WAIT_OBJECT_0) {
#ifdef _DEBUG
      char str[32]; strcpy(str, "nc> audio - ");
      itoa(vars->audioFrameIndex, str + strlen(str), 10);
      strcat(str, "\n");
      OutputDebugString(str);
#endif
      ResetEvent(audioEncEvent);
      if ((DWORD)vars->audioFrameIndex < audioBufferStartPos ||
          (DWORD)vars->audioFrameIndex >= audioBufferStartPos + 100) {
        audioBufferStartPos = vars->audioFrameIndex / 100 * 100;

        SocketReadVars(sock, &readAudio, sizeof(readAudio), &frameIndex, sizeof(frameIndex), &size, sizeof(size), 0);
        if (!readAudio || frameIndex != audioBufferStartPos) {
          // either video data, or some unwanted audio data. read and discard it, and issue a fresh request
          BYTE* data = new BYTE[size];
          SocketReadBlock(sock, data, size);
          delete[] data;

          // get audio for a new full second
          readAudio = TRUE;
          frameIndex = audioBufferStartPos;
          send(sock, (const char*)&readAudio, sizeof(readAudio), 0);
          send(sock, (const char*)&frameIndex, sizeof(frameIndex), 0);
          SocketReadVars(sock, &readAudio, sizeof(readAudio), &frameIndex, sizeof(frameIndex), &size, sizeof(size), 0);
        }
        SocketReadBlock(sock, audioBuffer, size);

        frameIndex += 100;
        send(sock, (const char*)&readAudio, sizeof(readAudio), 0);
        send(sock, (const char*)&frameIndex, sizeof(frameIndex), 0);
      }
      int offset = (vars->audioFrameIndex - audioBufferStartPos) * vars->wfx.nSamplesPerSec / 100;
      memcpy(((LPBYTE)vars) + vars->audiooffset, audioBuffer + offset * vars->wfx.nBlockAlign,
          vars->wfx.nBlockAlign * vars->wfx.nSamplesPerSec / 100);
      vars->audioBytesRead = vars->wfx.nBlockAlign * vars->wfx.nSamplesPerSec / 100;
      samplesread += vars->wfx.nSamplesPerSec / 100;
      SetEvent(audioDecEvent);
      noop = false;
    }
    if (noop) {
      fd_set readFds;
      FD_ZERO(&readFds);
      FD_SET(sock, &readFds);
      struct timeval tmout = { 0, 0 };
      int rval = select(0, &readFds, NULL, NULL, &tmout);
      if (FD_ISSET(sock, &readFds)) {
        BYTE val = 0;
        if (recv(sock, (char*)&val, 1, MSG_PEEK) == 0)
          break;
      }
    } else {
#ifdef _DEBUG
      char str[32];
      sprintf(str, "%f, %f\n", (float)(timeGetTime() - sttime) / 1000.0f, (firsttm - sttime) / 1000.0f);
      OutputDebugString(str);
#endif
    }
  }
  delete[] audioBuffer;

  if (varFile) {
    vars->encStatus = 1;        // encoder closing.
    SetEvent(videoDecEvent);
    SetEvent(audioDecEvent);

    CloseHandle(videoEncEvent);
    CloseHandle(videoDecEvent);
    CloseHandle(audioEncEvent);
    CloseHandle(audioDecEvent);
    UnmapViewOfFile(vars);
    CloseHandle(varFile);
    varFile = NULL;
  }
  if (sock)
    closesocket(sock);
  WSACleanup();
  DestroyWindow(servingdlg);

  return 0;
}
