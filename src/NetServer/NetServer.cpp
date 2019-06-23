/**
 * Debugmode Frameserver
 * Copyright (C) 2002-2019 Satish Kumar, All Rights Reserved
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

// NetServer.cpp : Defines the entry point for the application.
//

#include <windows.h>
#include <winsock.h>
#include <dfsc.h>
#include <stdio.h>
#include <tchar.h>

#define APPNAME _T("DebugMode Network Frameserver")

DWORD networkPort = 8278;
TCHAR signpostPath[MAX_PATH];

HINSTANCE ghInst;
DfscData* vars = NULL;
HANDLE varFile = NULL;
HANDLE videoEncSem, videoEncEvent, videoDecEvent;
HANDLE audioEncSem, audioEncEvent, audioDecEvent;

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

void SendReadAudioFrameIndexAndLen(SOCKET s, BOOL readAudio, DWORD frameIndex, DWORD len) {
  BYTE varbuf[32];
  int totsize = 0;

  *(BOOL*)(varbuf + totsize) = readAudio; totsize += sizeof(readAudio);
  *(DWORD*)(varbuf + totsize) = frameIndex; totsize += sizeof(frameIndex);
  *(DWORD*)(varbuf + totsize) = len; totsize += sizeof(len);
  send(s, (const char*)varbuf, totsize, 0);
}

int APIENTRY WinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow) {
  ghInst = hInstance;

  HKEY key = 0;
  RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\DebugMode\\FrameServer"), 0, 0,
      REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, &key, 0);
  if (key) {
    DWORD size = sizeof(networkPort);
    RegQueryValueEx(key, _T("networkPort"), 0, 0, (LPBYTE)&networkPort, &size);
    RegCloseKey(key);
  }

  WORD wVersionRequested;
  WSADATA wsaData;
  wVersionRequested = MAKEWORD(1, 1);
  if (WSAStartup(wVersionRequested, &wsaData) != 0) {
    MessageBox(NULL, _T("Unable to initialize network. Please check if you have latest network drivers."),
        APPNAME, MB_OK);
    return -1;
  }

  SOCKET sockConnect = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = PF_INET;
  addr.sin_addr.S_un.S_addr = INADDR_ANY;
  addr.sin_port = htons((u_short)networkPort);
  if (sockConnect == INVALID_SOCKET ||
      bind(sockConnect, (struct sockaddr*)&addr, sizeof(addr)) != 0 ||
      listen(sockConnect, SOMAXCONN) != 0) {
    MessageBox(NULL, _T("Unable to create network socket. Please check network connections and see if the "
        "port you specified is used by some other program."),
        APPNAME, MB_OK | MB_ICONEXCLAMATION);
    return -1;
  }

  DWORD stream = 0;
  char str[64] = "DfscData";
  _ultoa_s(stream, str + strlen(str), 56, 10);
  varFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(DfscData), str);
  if (GetLastError() != ERROR_ALREADY_EXISTS) {
    CloseHandle(varFile);
    varFile = NULL;
  }
  if (!varFile)
    return -1;
  vars = (DfscData*)MapViewOfFile(varFile, FILE_MAP_WRITE, 0, 0, 0);
  if (!vars)
    return -1;
  _tcscpy_s(signpostPath, _countof(signpostPath), vars->signpostPath);
  videoEncSem = CreateSemaphoreA(NULL, 1, 1, vars->videoEncSemName);
  videoEncEvent = CreateEventA(NULL, FALSE, FALSE, vars->videoEncEventName);
  videoDecEvent = CreateEventA(NULL, FALSE, FALSE, vars->videoDecEventName);
  audioEncSem = CreateSemaphoreA(NULL, 1, 1, vars->audioEncSemName);
  audioEncEvent = CreateEventA(NULL, FALSE, FALSE, vars->audioEncEventName);
  audioDecEvent = CreateEventA(NULL, FALSE, FALSE, vars->audioDecEventName);

  SOCKET sockClient[16];
  int numSockClient = 0;

  while (vars->encStatus != 1) {    // while encoder is not closed
    fd_set readFds;
    FD_ZERO(&readFds);
    FD_SET(sockConnect, &readFds);
    for (int s = 0; s < numSockClient; s++)
      FD_SET(sockClient[s], &readFds);
    struct timeval tmout = { 0, 100000 };
    int rval = select(0, &readFds, NULL, NULL, &tmout);
    if (rval > 0) {
      if (FD_ISSET(sockConnect, &readFds) && numSockClient < sizeof(sockClient) / sizeof(sockClient[0])) {
        sockClient[numSockClient] = accept(sockConnect, NULL, NULL);
        if (sockClient[numSockClient] != INVALID_SOCKET) {
          DWORD size = vars->totalAVDataSize;
          send(sockClient[numSockClient], (const char*)&size, sizeof(size), 0);
          send(sockClient[numSockClient], (const char*)vars, sizeof(DfscData), 0);
          FILE* fp = _tfopen(signpostPath, _T("rb"));
          bool success = false;
          if (fp) {
            fseek(fp, 0, SEEK_END);
            size = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            send(sockClient[numSockClient], (const char*)&size, sizeof(size), 0);
            BYTE fileData[32768];
            size_t i = 0;
            while (i < size) {
              size_t cursize = min(size - i, 32768);
              size_t bytesread = 0;
              bytesread = fread(fileData, 1, cursize, fp);
              if (bytesread != cursize) break;
              send(sockClient[numSockClient], (char*)fileData, cursize, 0);
              i += cursize;
            }
            fclose(fp);
            if (i == size)
              success = true;
          }
          if (success)
            numSockClient++;
          else
            closesocket(sockClient[numSockClient]);
          vars->numNetworkClientsConnected = numSockClient;
        }
      }
      for (int s = 0; s < numSockClient; s++) {
        if (FD_ISSET(sockClient[s], &readFds)) {
          BOOL readAudio = FALSE;
          DWORD frameIndex = 0;
          bool closeconn = false;
          int rval;
          closeconn = ((rval = SocketReadBlock(sockClient[s], (char*)&readAudio, sizeof(readAudio))) == 0);
          if (rval == SOCKET_ERROR) closeconn = true;
          if (!closeconn && rval > 0) {
#ifdef _DEBUG
            char str[32];
            sprintf(str, "v1 %f - ", (float)timeGetTime() / 1000.0f);
            OutputDebugStringA(str);
#endif
            closeconn = ((rval = SocketReadBlock(sockClient[s], (char*)&frameIndex, sizeof(frameIndex))) == 0);
            if (rval == SOCKET_ERROR) closeconn = true;
            if (!closeconn && rval > 0) {
              BYTE* ptr = NULL;
              DWORD len = 0;
              if (readAudio) {
                WaitForSingleObject(audioEncSem, 10000);
                ptr = new BYTE[48000 * 6];
                /*---- Old implementation which read 1 audioframe at a time.
                   for (int f=0; f<100 && vars->encStatus != 1; f++)
                   {
                    vars->audioFrameIndex = frameIndex+f;
                    SetEvent(audioEncEvent);
                    if (WaitForSingleObject(audioDecEvent, INFINITE) == WAIT_OBJECT_0)
                    {
                        if (vars->encStatus != 1)   // encoder closed
                        {
                            memcpy(ptr+len, (LPBYTE)vars+vars->audiooffset, vars->audioBytesRead);
                            len += vars->audioBytesRead;
                        }
                    }
                   }*/
                vars->audioRequestFullSecond = TRUE;
                vars->audioFrameIndex = frameIndex / 100;
                SetEvent(audioEncEvent);
                if (WaitForSingleObject(audioDecEvent, INFINITE) == WAIT_OBJECT_0) {
                  if (vars->encStatus != 1) {                   // encoder closed
                    memcpy(ptr, (LPBYTE)vars + vars->audiooffset, vars->audioBytesRead);
                    len = vars->audioBytesRead;
                  }
                }
                ReleaseSemaphore(audioEncSem, 1, NULL);
              } else {
                WaitForSingleObject(videoEncSem, 10000);
                vars->videoFrameIndex = frameIndex;
                SetEvent(videoEncEvent);
                if (WaitForSingleObject(videoDecEvent, INFINITE) == WAIT_OBJECT_0) {
                  if (vars->encStatus != 1) {                   // encoder closed
                    len = vars->videoBytesRead;
                    ptr = ((LPBYTE)vars) + vars->videooffset;
                  }
                }
                ReleaseSemaphore(videoEncSem, 1, NULL);
              }
              OutputDebugStringA("sending\n");
              SendReadAudioFrameIndexAndLen(sockClient[s], readAudio, frameIndex, len);
              if (len)
                send(sockClient[s], (const char*)ptr, len, 0);
              // OutputDebugString("2\n");
              if (ptr && readAudio)
                delete[] ptr;
            }
#ifdef _DEBUG
            sprintf(str, "v2 %f\n", (float)timeGetTime() / 1000.0f);
            OutputDebugStringA(str);
#endif
          }
          if (closeconn) {
            OutputDebugStringA("Connection closed\n");
            closesocket(sockClient[s]);
            memcpy(sockClient + s, sockClient + s + 1, sizeof(sockClient[0]) * (numSockClient - s - 1));
            numSockClient--;
            vars->numNetworkClientsConnected = numSockClient;
          }
        }
      }
    }
  }

  if (varFile) {
    vars->decStatus = 1;        // decoder closing.
    UnmapViewOfFile(vars);
    CloseHandle(varFile);
    vars = NULL;
    varFile = NULL;
    CloseHandle(videoEncSem);
    CloseHandle(videoEncEvent);
    CloseHandle(videoDecEvent);
    CloseHandle(audioEncSem);
    CloseHandle(audioEncEvent);
    CloseHandle(audioDecEvent);
  }
  if (sockConnect)
    closesocket(sockConnect);
  WSACleanup();

  return 0;
}
