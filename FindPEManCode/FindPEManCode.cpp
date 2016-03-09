// FindPEManCode.cpp : Defines the entry point for the application.
//



#include "stdafx.h"
#include "FindPEManCode.h"
#include "worker.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
extern HWND hWndListView;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Place code here.

	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_FINDPEMANCODE, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_FINDPEMANCODE));

	MSG msg;

	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FINDPEMANCODE));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_FINDPEMANCODE);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}


//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 500, 700, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	////////////////////////////////////////////
	// Create the lisview for displaying information.
	INITCOMMONCONTROLSEX icex;           // Structure for control initialization.
	icex.dwICC = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&icex);

	RECT rcClient;                       // The parent window's client area.
	GetClientRect(hWnd, &rcClient);

	hWndListView = CreateWindowW(WC_LISTVIEW,
		L"",
		WS_CHILD | LVS_REPORT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, //rcClient.right - rcClient.left,
		CW_USEDEFAULT, //rcClient.bottom - rcClient.top,
		hWnd,
		(HMENU)100,
		hInst,
		NULL);

	LVCOLUMN lvC;
	memset(&lvC, 0, sizeof(LVCOLUMN)); // for safety's sake, make sure lvC's members are valid.
	// Initialize the LVCOLUMN structure.
	// The mask specifies that the format, width, text,
	// and subitem members of the structure are valid.
	lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

	// Add the columns.
	lvC.iSubItem = 0;
	lvC.pszText = L"PID";
	lvC.cx = 60;
	ListView_InsertColumn(hWndListView, 0, &lvC);
	lvC.iSubItem = 1;
	//lvC.cx = 240;
	lvC.pszText = L"Tiến trình cần kiểm tra Virtual Memory các vùng Private ERW/ER";
	ListView_InsertColumn(hWndListView, 1, &lvC);
	//lvC.iSubItem = 2;
	//lvC.pszText = L"WoW64";
	////lvC.cx = 60;
	//ListView_InsertColumn(hWndListView, 2, &lvC);
	ListView_SetColumnWidth(hWndListView, 1, LVSCW_AUTOSIZE_USEHEADER);
	
	ShowWindow(hWndListView, SW_SHOWMAXIMIZED);
	ListView_SetExtendedListViewStyle(hWndListView, LVS_EX_FULLROWSELECT);// LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES |LVS_EX_BORDERSELECT | LVS_EX_ONECLICKACTIVATE
	//UpdateWindow(hWndListView);
	/////////////////////////////////////////////////
	
	DoMainWork();

	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_REFRESH:
			ListView_DeleteAllItems(hWndListView);
			DoMainWork();
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_NOTIFY:
	{
		NMHDR nmh; //  unreferenced
		if (((NMHDR*)lParam)->idFrom == 100)
		{
			LPNMLISTVIEW pNmv = (LPNMLISTVIEW)lParam;
			switch (((NMHDR*)lParam)->code)
			{
			case LVN_COLUMNCLICK:
				if (((NMLISTVIEW*)lParam)->iSubItem == 0)
				{
					SortColumn(hWndListView, ((NMLISTVIEW*)lParam)->iSubItem);
				}
			break;
			case LVN_FIRST + 88U: // Notification code == FFFF FFF4 for resizing column.
			{
				RECT rcClient;                       // The parent window's client area.
				GetClientRect(hWnd, &rcClient);
				MoveWindow(hWndListView, 0, 0, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top, TRUE);
				ListView_SetColumnWidth(hWndListView, 1, LVSCW_AUTOSIZE_USEHEADER);
				/*TCHAR szOutput[256];
				StringCchPrintf(szOutput, sizeof(szOutput) / sizeof(TCHAR), L"Sizing column Code: %u\n", ((NMHDR*)lParam)->code);
				OutputDebugString(szOutput);*/
			}
			break;
			default:
				return DefWindowProc(hWnd, message, wParam, lParam);
			}
		}
	}
	break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code that uses hdc here...
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}