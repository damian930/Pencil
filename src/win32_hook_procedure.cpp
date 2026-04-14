#include "core/core_include.h"
#include "core/core_include.cpp"

#include "windows.h"
#pragma comment(lib, "user32.lib")

extern "C" __declspec( dllexport )
U64 win32_hook_proc(int code, U64 w_param, U64 l_param)
{
  if (code > 0)
  {
    CWPRETSTRUCT* msg_details = (CWPRETSTRUCT*)l_param;
    if (msg_details->message == WM_LBUTTONDOWN)
    {
      printf("Left mouse down event \n");
      // left_mouse_down_event_count += 1;
    }
  }
  return CallNextHookEx(Null, code, w_param, l_param); 
  

  // printf("DLL \n");

  /*
  CHAR szCWPBuf[256]; 
    CHAR szMsg[16]; 
    HDC hdc; 
    static int c = 0; 
    size_t cch;
    HRESULT hResult; 
 
    if (nCode < 0)  // do not process message 
        return CallNextHookEx(myhookdata[IDM_CALLWNDPROC].hhook, nCode, wParam, lParam); 
 
    // Call an application-defined function that converts a message 
    // constant to a string and copies it to a buffer. 
 
    LookUpTheMessage((PMSG) lParam, szMsg); 
 
    hdc = GetDC(gh_hwndMain); 
 
    switch (nCode) 
    { 
        case HC_ACTION:
            hResult = StringCchPrintf(szCWPBuf, 256/sizeof(TCHAR),  
               "CALLWNDPROC - tsk: %ld, msg: %s, %d times   ", 
                wParam, szMsg, c++);
            if (FAILED(hResult))
            {
            // TODO: writer error handler
            }
            hResult = StringCchLength(szCWPBuf, 256/sizeof(TCHAR), &cch);
            if (FAILED(hResult))
            {
            // TODO: write error handler
            } 
            TextOut(hdc, 2, 15, szCWPBuf, cch); 
            break; 
 
        default: 
            break; 
    } 
 
    ReleaseDC(gh_hwndMain, hdc); 
    
    return CallNextHookEx(myhookdata[IDM_CALLWNDPROC].hhook, nCode, wParam, lParam); 
  */
  // return 0;
}
