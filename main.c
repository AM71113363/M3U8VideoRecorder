#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <commctrl.h>

#define ID_CHECK        1000
#define ID_START        1001
#define ID_STOP         1002

char szClassName[ ] = "WindosVideoRecorder";

HWND hWnd;
HINSTANCE ins;
STARTUPINFO startupinfo;
PROCESS_INFORMATION process;

HWND hCheck,hStart,hStop,hQuality,hExt,hUrl;

UCHAR Url[MAX_PATH];
UCHAR buf[1024];
UCHAR Q[16];
UCHAR VideoExt[8];

void GetInformation(UCHAR *LINK);

void CenterOnScreen()
{
     RECT rcClient, rcDesktop;
	 int nX, nY;
     SystemParametersInfo(SPI_GETWORKAREA, 0, &rcDesktop, 0);
     GetWindowRect(hWnd, &rcClient);
     nX=((rcDesktop.right - rcDesktop.left) / 2) -((rcClient.right - rcClient.left) / 2);
     nY=((rcDesktop.bottom - rcDesktop.top) / 2) -((rcClient.bottom - rcClient.top) / 2);
     SetWindowPos(hWnd, NULL, nX, nY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
     SetWindowPos(hWnd, HWND_TOPMOST,0,0,0,0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);						
  return;
}

void AddQuality(UCHAR *s)
{
    int n=SendMessage(hQuality,CB_ADDSTRING,0,(LPARAM)(LPCSTR)s);
    if(n!=CB_ERR)
       SendMessage(hQuality,CB_SETCURSEL, (WPARAM)n,0);
}


void CheckPlayList()
{
    SendMessage(hQuality,CB_RESETCONTENT,0,0);
    SetWindowText(hWnd,".m3u8 Video Recorder");
    memset(Url, 0, MAX_PATH);
    if(GetWindowText(hUrl,Url,MAX_PATH) < 1)
    {
        MessageBox(hWnd,"m3u8 URL is empty","Error", MB_ICONINFORMATION|MB_SYSTEMMODAL|MB_OK);
        return;
    }
    if(strstr(Url,"https:")) //for now only https allowed
    {
       GetInformation(Url);
    }
    else{ AddQuality("AUTO"); }                     
    EnableWindow(hStart,1);
}
     
     
void StartRecording()
{
	int n,m;
    memset(buf,0,1024);
	memset (&process, 0, sizeof (PROCESS_INFORMATION)) ;
	memset (&startupinfo, 0, sizeof (STARTUPINFO)) ; 
	
	startupinfo.cb = sizeof(STARTUPINFO);
    startupinfo.wShowWindow = SW_SHOW;
    startupinfo.dwFlags = STARTF_USESHOWWINDOW;
	
    memset(Url, 0, MAX_PATH);
    if(GetWindowText(hUrl,Url,MAX_PATH) < 1)
    {
        MessageBox(hWnd,"m3u8 URL is empty","Error", MB_ICONINFORMATION|MB_SYSTEMMODAL|MB_OK);
        return;
    }
    memset(VideoExt,0,8);
    if(GetWindowText(hExt,VideoExt,7) < 1)
    {
        strcpy(VideoExt,".ts\0");
    }
    if(VideoExt[0]!='.')
   {
        strcpy(VideoExt,".ts\0");
    }    
    SetWindowText(hExt,VideoExt);
    n=SendMessage(hQuality, CB_GETCURSEL, 0, 0);
    m=SendMessage(hQuality, CB_GETCOUNT, 0, 0);
    memset(Q,0,16);
    SetWindowText(hWnd,".m3u8 Video Recorder");
    if(n!=m) //is not empty
    {
       m--;
       if(n!=m) //if == is the last
       {
          sprintf(Q," -map 0:p:%i\0",n);  
          SetWindowText(hWnd,Q);   
       }       
    }

    sprintf(buf,"ffmpeg -i %s%s -c copy Video_%d%s\0",Url,Q,GetTickCount(),VideoExt); //DETACHED_PROCESS CREATE_NEW_CONSOLE 
    AllocConsole();
    
    if (!CreateProcess(NULL, buf, NULL, NULL, FALSE, 0, NULL, NULL, &startupinfo, &process))
	{
        MessageBox(hWnd,"!CreateProcess","Error", MB_ICONINFORMATION|MB_SYSTEMMODAL|MB_OK);
        FreeConsole();
        return;
    }
   	 EnableWindow(hStart,0);
	 EnableWindow(hStop,1);
     WaitForSingleObject(process.hProcess, INFINITE);
     FreeConsole();
     EnableWindow(hStart,1);
	 EnableWindow(hStop,0);
	 
     TerminateProcess(process.hProcess,process.dwProcessId);
     TerminateThread(process.hThread,process.dwThreadId);
	 CloseHandle (process.hProcess);
     CloseHandle (process.hThread); 
}


void StopRecording()
{
     TerminateProcess(process.hProcess,process.dwProcessId);
     TerminateThread(process.hThread,process.dwThreadId);
       
	 CloseHandle (process.hProcess);
     CloseHandle (process.hThread);     
}
        
LRESULT CALLBACK WindowProcedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)                  
    {
         case WM_CREATE:
         {
              InitCommonControls();
              hWnd = hwnd;
              HFONT hFont = CreateFont(22, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "ARIAL");
              hStart=CreateWindow("BUTTON","Start",WS_CHILD|WS_VISIBLE|WS_DISABLED,2,2,63,24,hwnd,(HMENU)ID_START,ins,NULL);
              hStop=CreateWindow("BUTTON","Stop",WS_CHILD|WS_VISIBLE|WS_DISABLED,69,2,63,24,hwnd,(HMENU)ID_STOP,ins,NULL);
              hQuality=CreateWindow("combobox", "",WS_CHILD|WS_VISIBLE |CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,137, 2, 111, 80, hwnd, NULL,ins, NULL);  	
		      hExt=CreateWindow("EDIT", ".ts",WS_CHILD|WS_VISIBLE,253, 2, 57, 24, hwnd, NULL,ins, NULL);  	
	      	  SendMessage(hExt, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(1, 0));
              hCheck=CreateWindow("BUTTON","Check",WS_CHILD|WS_VISIBLE,315,2,63,24,hwnd,(HMENU)ID_CHECK,ins,NULL);
              hUrl=CreateWindow("EDIT", "",WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,4, 30, 371, 20, hwnd, NULL,ins, NULL);  	
		      
              CenterOnScreen(hwnd);       
         }
         break;
         case WM_SETFOCUS:
              SetFocus(hUrl);
         break;
         case WM_COMMAND:
         {
              switch(LOWORD(wParam))
              { 
 				  case ID_CHECK:
                  {     
                        CreateThread(0,0,(LPTHREAD_START_ROUTINE)CheckPlayList,0,0,0); 
                  }
                  break; 
 				  case ID_START:
                  {    
                        CreateThread(0,0,(LPTHREAD_START_ROUTINE)StartRecording,0,0,0); 
                  }
                  break; 
                  case ID_STOP:
                  {
                         CreateThread(0,0,(LPTHREAD_START_ROUTINE)StopRecording,0,0,0); 
                  }   
                  break;
              }//switch
         }
         break;
        case WM_DESTROY:
        {
              TerminateProcess(process.hProcess,process.dwProcessId);
              TerminateThread(process.hThread,process.dwThreadId);
              CloseHandle (process.hProcess);
              CloseHandle (process.hThread);
              FreeConsole();
              PostQuitMessage (0); 
        } 
        break;
        default:         
            return DefWindowProc (hwnd, message, wParam, lParam);
    }

    return 0;
}

int WINAPI WinMain (HINSTANCE hThisInstance, HINSTANCE hPrevInstance, LPSTR lpszArgument, int nFunsterStil)
{
            
    MSG messages;    
    WNDCLASSEX wincl; 
    HWND hwnd;    
    ins=hThisInstance;

    wincl.hInstance = hThisInstance;
    wincl.lpszClassName = szClassName;
    wincl.lpfnWndProc = WindowProcedure;
    wincl.style = CS_DBLCLKS;  
    wincl.cbSize = sizeof (WNDCLASSEX);


    wincl.hIcon = LoadIcon (ins,MAKEINTRESOURCE(200));
    wincl.hIconSm = LoadIcon (ins,MAKEINTRESOURCE(200));
    wincl.hCursor = LoadCursor (NULL, IDC_ARROW);
    wincl.lpszMenuName = NULL;  
    wincl.cbClsExtra = 0;  
    wincl.cbWndExtra = 0;      

    wincl.hbrBackground = (HBRUSH) COLOR_BACKGROUND;


    if (!RegisterClassEx (&wincl))
        return 0;

    hwnd = CreateWindowEx(WS_EX_TOPMOST,szClassName,".m3u8 Video Recorder",WS_OVERLAPPED|WS_SYSMENU,CW_USEDEFAULT,CW_USEDEFAULT,
    386,85,HWND_DESKTOP,NULL,hThisInstance,NULL );
    
    ShowWindow (hwnd, nFunsterStil);

    while (GetMessage (&messages, NULL, 0, 0))
    {
         TranslateMessage(&messages);
         DispatchMessage(&messages);
    }

     return messages.wParam;
}

