#include <Windows.h>
#include <CommCtrl.h>
#include "config.h"
#include "status.h"
#include "tabset.h"
#include "tab_common.h"
#include "engine.h"

typedef Status_t (*Tab_Handler_t)(TabEvent_t);

typedef struct 
{
	wchar_t tabTitle[CONFIG_MAX_TAB_TITLE_SIZE];
	TabHandlers_t handlers;
} TabInfo_t;

typedef struct {
	HWND parentHwnd;
	HWND rootHwnd;
	HWND tabCtrlHwnd;
	HWND curTabHwnd;
	HINSTANCE hInstance;
	TabInfo_t tabs[CONFIG_MAX_TABS_COUNT];
	int tabCount;
	int curTab;
	TabSet_GdiCollection_t collection;
} TabSet_t;

static TabSet_t g_tabSet;

// {2A2653CF-77F4-4d55-B34F-6D5508A49CEC}
static const GUID teamLeagueGuid = { 0x2a2653cf, 0x77f4, 0x4d55, { 0xb3, 0x4f, 0x6d, 0x55, 0x8, 0xa4, 0x9c, 0xec } };
static const char* teamLeagueStr = "TeamLeague";
static const wchar_t* teamLeagueClass = L"TeamLeagueBabasChessInfoTab";

//////////////////////////////////////////////////////////////////////////
// Tab list goes here
//////////////////////////////////////////////////////////////////////////

typedef struct
{
	Status_t (*getHandlers)(TabHandlers_t*);
} TabInitInfo_t ;

extern Status_t Tab_Current_GetHandlers(TabHandlers_t* );
extern Status_t Tab_Standings_GetHandlers(TabHandlers_t* );
extern Status_t Tab_Finished_GetHandlers(TabHandlers_t* );
extern Status_t Tab_Upcoming_GetHandlers(TabHandlers_t* );
extern Status_t Tab_NotScheduled_GetHandlers(TabHandlers_t* );

static TabInitInfo_t g_initTabList[] = 
{
	{ &Tab_Current_GetHandlers },
	{ &Tab_Upcoming_GetHandlers },
	{ &Tab_Standings_GetHandlers },
	{ &Tab_Finished_GetHandlers },
	{ &Tab_NotScheduled_GetHandlers },
};

//////////////////////////////////////////////////////////////////////////

void Tabset_SelectStyle(HDC hdc, const TabSet_ColorStyle_t* style, TabSet_ColorStyle_t* styleToStore)
{
	if (styleToStore)
	{
		styleToStore->border	= (HPEN)SelectObject(hdc, style->border);
		styleToStore->brush		= (HBRUSH)SelectObject(hdc, style->brush);
		styleToStore->fontColor = SetTextColor(hdc, style->fontColor);
	}
	else
	{
		SelectObject(hdc, style->border);
		SelectObject(hdc, style->brush);
		SetTextColor(hdc, style->fontColor);
	}
}

static void TabSet_CreateColorStyle(TabSet_ColorStyle_t *style, COLORREF bgColor, COLORREF borderColor, COLORREF textColor)
{
	style->brush = CreateSolidBrush(bgColor);
	style->border = CreatePen(PS_SOLID, 0, borderColor);
	style->fontColor = textColor;
}

static void TabSet_DestroyColorStyle(TabSet_ColorStyle_t *style)
{
	DeleteObject(style->border);
	DeleteObject(style->brush);
}

static void TabSet_InitGdiCollection()
{
	TabSet_CreateColorStyle(&g_tabSet.collection.headerCell, CONFIG_COLOR_HEADER, CONFIG_COLOR_PEN, CONFIG_COLOR_PEN);
	TabSet_CreateColorStyle(&g_tabSet.collection.badCell, CONFIG_COLOR_BAD, CONFIG_COLOR_PEN, CONFIG_COLOR_PEN);
	TabSet_CreateColorStyle(&g_tabSet.collection.pourCell, CONFIG_COLOR_POUR, CONFIG_COLOR_PEN, CONFIG_COLOR_PEN);
	TabSet_CreateColorStyle(&g_tabSet.collection.goodCell, CONFIG_COLOR_GOOD, CONFIG_COLOR_PEN, CONFIG_COLOR_PEN);
	TabSet_CreateColorStyle(&g_tabSet.collection.pourOrBadCell, CONFIG_COLOR_POURORBAD, CONFIG_COLOR_PEN, CONFIG_COLOR_PEN);
	TabSet_CreateColorStyle(&g_tabSet.collection.pourOrGoodCell, CONFIG_COLOR_GOODORPOUR, CONFIG_COLOR_PEN, CONFIG_COLOR_PEN);
	TabSet_CreateColorStyle(&g_tabSet.collection.neutralCell, g_tabSet.collection.bgColor, g_tabSet.collection.fgColor, g_tabSet.collection.fgColor);
	g_tabSet.collection.normalFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	g_tabSet.collection.headerFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
}

static void TabSet_DestroyGdiCollection()
{
	TabSet_DestroyColorStyle(&g_tabSet.collection.headerCell);
	TabSet_DestroyColorStyle(&g_tabSet.collection.badCell);
	TabSet_DestroyColorStyle(&g_tabSet.collection.pourCell);
	TabSet_DestroyColorStyle(&g_tabSet.collection.goodCell);
	TabSet_DestroyColorStyle(&g_tabSet.collection.pourOrBadCell);
	TabSet_DestroyColorStyle(&g_tabSet.collection.pourOrGoodCell);
	TabSet_DestroyColorStyle(&g_tabSet.collection.neutralCell);

	DeleteObject(g_tabSet.collection.normalFont);
	DeleteObject(g_tabSet.collection.headerFont);

	memset(&g_tabSet.collection, 0, sizeof(g_tabSet.collection));
}

void Tabset_Destroy()
{
	DestroyWindow(g_tabSet.rootHwnd);
	g_tabSet.rootHwnd = 0;
	TabSet_DestroyGdiCollection();
}

const TabSet_GdiCollection_t * Tabset_GetGdiCollection()
{
	return &g_tabSet.collection;
}

HINSTANCE Tabset_GetHInstance()
{
	return g_tabSet.hInstance;
}

const GUID* Tabset_GetGUID()
{
	return &teamLeagueGuid;
}

const char* Tabset_GetTitle()
{
	return CONFIG_PLUGIN_TAB_TITLE;
}

HBITMAP Tabset_GetBitmap()
{
	return 0;
}

Status_t Tabset_PopulateTabTitle( TabInfo_t *tabInfo) 
{
	if (tabInfo->handlers.onGetTabTitle == NULL)
	{
		wcscpy_s(tabInfo->tabTitle, sizeof(tabInfo->tabTitle)/sizeof(wchar_t), L"<none>");
	}
	else
	{
		char mbTabTitle[CONFIG_MAX_TAB_TITLE_SIZE];
		size_t result;
		Status_t status;
		status = tabInfo->handlers.onGetTabTitle(mbTabTitle, CONFIG_MAX_TAB_TITLE_SIZE);
		if (status != STATUS_OK)
		{
			return status;
		}

		mbstowcs_s(&result, tabInfo->tabTitle, sizeof(tabInfo->tabTitle)/sizeof(wchar_t), mbTabTitle, CONFIG_MAX_TAB_TITLE_SIZE);
	}
	return STATUS_OK;
}

static Status_t Tabset_UpdateTabTitles()
{
	int i;

	for(i = 0; i < g_tabSet.tabCount; ++i)
	{
		TCITEM tcitem;
		Tabset_PopulateTabTitle(&g_tabSet.tabs[i]);
		tcitem.mask = TCIF_TEXT; // TODO  Add icons
		tcitem.pszText = g_tabSet.tabs[i].tabTitle;
		TabCtrl_SetItem(g_tabSet.tabCtrlHwnd, i, &tcitem);
	}

	return STATUS_OK;
}

static Status_t Tabset_ProcessEvent(TabEvent_t event)
{
	switch(event)
	{
	case TABEVENT_DATA_UPDATED:
	case TABEVENT_DATA_UPDATE_ERROR:
		return Tabset_UpdateTabTitles();
	}

	return STATUS_OK;
}

Status_t Tabset_Event(TabEvent_t event)
{
	Status_t status;
	status = Tabset_ProcessEvent(event);
	if (status == STATUS_OK_PROCESSED)
	{
		return STATUS_OK;
	}
	if (status != STATUS_OK && status != STATUS_OK_NOT_PROCESSED)
	{
		return status;
	}

	status = Tab_Common_Event(event);
	if (status == STATUS_OK_PROCESSED)
	{
		return STATUS_OK;
	}
	if (status != STATUS_OK && status != STATUS_OK_NOT_PROCESSED)
	{
		return status;
	}

	if (g_tabSet.curTab >= 0 && g_tabSet.tabs[g_tabSet.curTab].handlers.onEvent)
	{
		return g_tabSet.tabs[g_tabSet.curTab].handlers.onEvent(event);
	}

	return STATUS_OK_TAB_NO_HANDLER;
}

static Status_t Tabset_SelectTab(int index)
{
	Status_t status;
	RECT rcClient;
	if (g_tabSet.curTabHwnd)
	{
		DestroyWindow(g_tabSet.curTabHwnd);
		g_tabSet.curTabHwnd = NULL;
	}

	GetClientRect(g_tabSet.rootHwnd, &rcClient);
	TabCtrl_AdjustRect(g_tabSet.tabCtrlHwnd, FALSE, &rcClient);

	status = g_tabSet.tabs[index].handlers.onCreate(g_tabSet.tabCtrlHwnd, &g_tabSet.curTabHwnd);
	MoveWindow(g_tabSet.curTabHwnd, rcClient.left, rcClient.top, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top, TRUE);

	if (status != STATUS_OK)
	{
		return status;
	}
	g_tabSet.curTab = index;
	return STATUS_OK;
}

static Status_t Tabset_CreateTabset(HWND parent)
{
	RECT rcClient; 
	int i;

	g_tabSet.tabCount = 0;
	g_tabSet.curTab = -1;
	g_tabSet.curTabHwnd = 0;

	GetClientRect(parent, &rcClient);
	InitCommonControls();

	g_tabSet.tabCtrlHwnd = CreateWindowEx(0, WC_TABCONTROL, L"", WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE , 0, 0, rcClient.right, rcClient.bottom, 
		parent, NULL, g_tabSet.hInstance, NULL);
	if (g_tabSet.tabCtrlHwnd == 0)
	{
		return STATUS_ERR_WINDOW_CANNOT_CREATE;
	}

	SendMessage(g_tabSet.tabCtrlHwnd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);

	for (i = 0; i < sizeof(g_initTabList)/sizeof(TabInitInfo_t); ++i)
	{
		Status_t status;
		TabInfo_t *tabInfo = &g_tabSet.tabs[g_tabSet.tabCount];

		if (g_tabSet.tabCount >= CONFIG_MAX_TABS_COUNT)
		{
			return STATUS_ERR_NOT_ENOUGH_ROOM; // TODO make some cleanup here
		}

		memset(&tabInfo->handlers, 0, sizeof(TabHandlers_t));
		status = g_initTabList[i].getHandlers(&tabInfo->handlers);

		if (tabInfo->handlers.onCreate == NULL)
		{
			return STATUS_ERR_TAB_NO_HANDLER;
		}

		status = Tabset_PopulateTabTitle(tabInfo);
		if (status != STATUS_OK)
		{
			return status;
		}

		if (tabInfo->handlers.onEvent != NULL)
		{
			status = tabInfo->handlers.onEvent(TABEVENT_INIT);
			if (status != STATUS_OK)
			{
				return status;
			}

		}

		++g_tabSet.tabCount;
	}

	for(i = 0; i < g_tabSet.tabCount; ++i)
	{
		TCITEM tcitem;
		int index;
		tcitem.mask = TCIF_TEXT; // TODO  Add icons
		tcitem.pszText = g_tabSet.tabs[i].tabTitle;
		index = TabCtrl_InsertItem(g_tabSet.tabCtrlHwnd, i, &tcitem);
		if (index != i)
		{
			return STATUS_ERR_WINDOW_CANNOT_POPULATE;
		}
	}

	return Tabset_SelectTab(0); // TODO make it storeable in config
}

static LRESULT CALLBACK Tabset_Wndproc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_CREATE:
		{
			Status_t status = Tabset_CreateTabset(hwnd);
			if (status != STATUS_OK)
			{
				return -1;
			}
			SetTimer(hwnd, 0, CONFIG_TIMER_POLLING_INTERVAL, 0);
			return 0;
		}
	case WM_SIZE:
		{
			RECT rcClient;
			GetClientRect(hwnd, &rcClient);
			MoveWindow(g_tabSet.tabCtrlHwnd, rcClient.left, rcClient.top, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top, TRUE);    
			if (g_tabSet.curTabHwnd != 0)
			{
				TabCtrl_AdjustRect(g_tabSet.tabCtrlHwnd, FALSE, &rcClient);
				MoveWindow(g_tabSet.curTabHwnd, rcClient.left, rcClient.top, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top, TRUE);
			}
			return 0;
		}
	case WM_NOTIFY:
		{
			NMHDR *mhdr = (LPNMHDR) lParam;
			if (mhdr->hwndFrom == g_tabSet.tabCtrlHwnd && mhdr->code == TCN_SELCHANGE)
			{
				Tabset_SelectTab(TabCtrl_GetCurSel(g_tabSet.tabCtrlHwnd));
				return 14; // No return value
			}
		}
	case WM_TIMER:
		{
			Engine_OnTimer();
			return 0;
		}

	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}

Status_t Tabset_init(HWND parent, HINSTANCE hInstance, COLORREF bgColor, COLORREF textColor)
{
	WNDCLASS wndclass;  
	Status_t status;
	g_tabSet.parentHwnd = parent;
	g_tabSet.hInstance = hInstance;
	g_tabSet.rootHwnd = 0;
	g_tabSet.collection.bgColor = bgColor;
	g_tabSet.collection.fgColor = textColor;

	TabSet_InitGdiCollection();

	status = Tab_Common_Init();
	if (status != STATUS_OK)
	{
		return status;
	}

	wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS ; 
	wndclass.lpfnWndProc = Tabset_Wndproc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = g_tabSet.hInstance; 
	wndclass.hIcon = NULL;
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW); 
	wndclass.hbrBackground = CreateSolidBrush(bgColor); // TODO fix it
	wndclass.lpszMenuName =  NULL;
	wndclass.lpszClassName = teamLeagueClass;
	if (RegisterClass(&wndclass) == 0)
	{
		return STATUS_ERR_WINDOW_CANNOT_REGISTER;
	}

	g_tabSet.rootHwnd = CreateWindow(teamLeagueClass, L"", WS_CHILD|WS_VISIBLE, 0, 0, 0, 0, parent, 0, g_tabSet.hInstance, 0);
	if (g_tabSet.rootHwnd == 0)
	{
		return STATUS_ERR_WINDOW_CANNOT_CREATE;
	}

	return STATUS_OK;
}

HWND Tabset_GetHWND()
{
	return g_tabSet.rootHwnd;
}

