#include "tab_common.h"
#include "dataset.h"
#include "settings.h"
#include <stdio.h>

//////////////////////////////////////////////////////////////////////////
// This file is made using copy-paste method. :) Sorry
//////////////////////////////////////////////////////////////////////////

typedef struct 
{
	Basic_GameData_t games[CONFIG_MAX_GAMES_PER_ROUND];
	int activeCount;
	int adjournedCount;
	HWND hwnd;
} Tab_Current_t;

static Tab_Current_t *g_tab = 0;


static Status_t Tab_Current_UpdateData()
{
	Status_t status;

	BOOL onlyFavorites = Settings_GetSettings()->showFavoritesOnly;

	status = Tab_Common_PopulateStdData(onlyFavorites, g_tab->games, &g_tab->activeCount, CONFIG_MAX_GAMES_PER_ROUND, GAMESTATUS_InProgress);

	if (status == STATUS_OK)
	{
		status = Tab_Common_PopulateStdData(onlyFavorites, g_tab->games+g_tab->activeCount, &g_tab->adjournedCount, 
			                         CONFIG_MAX_GAMES_PER_ROUND-g_tab->activeCount, GAMESTATUS_Adjourned);
	}

	if (status != STATUS_OK)
	{
		g_tab->activeCount = 0;
		g_tab->adjournedCount = 0;
	}

	if (status == STATUS_ERR_DATA_INVALID)
	{
		status = STATUS_OK;
	}

	return status;
}


static void Tab_Current_UpdateScrollBar()
{
	Tab_Common_StdTablePaintData_t sizes;
	RECT rect;
	int totalHeight = 0;
	SCROLLINFO scrollinfo;

	if (g_tab->adjournedCount <= 0 && g_tab->activeCount <= 0)
	{
		scrollinfo.cbSize = sizeof(scrollinfo);
		scrollinfo.fMask = SIF_PAGE | SIF_RANGE;
		scrollinfo.nMin = 0;
		scrollinfo.nMax = 0;
		scrollinfo.nPage = 1;
		SetScrollInfo(g_tab->hwnd, SB_VERT, &scrollinfo, TRUE);
		return;
	}

	GetClientRect(g_tab->hwnd, &rect);
	rect.top += CONFIG_TABS_COMMON_TOP_MARGIN;

	Tab_Common_CalcStdSizes(&rect, &sizes);

	if (g_tab->activeCount)
	{
		totalHeight = 2*sizes.headerHeight + sizes.rowHeight*g_tab->activeCount;
	}
	if (g_tab->adjournedCount)
	{
		totalHeight += 2*sizes.headerHeight + sizes.rowHeight*g_tab->adjournedCount;
	}

	scrollinfo.cbSize = sizeof(scrollinfo);
	scrollinfo.fMask = SIF_PAGE | SIF_RANGE;
	scrollinfo.nMin = 0;
	scrollinfo.nMax = totalHeight;
	scrollinfo.nPage = rect.bottom - rect.top;

	SetScrollInfo(g_tab->hwnd, SB_VERT, &scrollinfo, TRUE);
}


static void Tab_Current_PaintDC(HDC hdc, int offset, RECT clientRect)
{
	RECT rect;
	Tab_Common_StdTablePaintData_t sizes;

	SelectObject(hdc, Tabset_GetGdiCollection()->normalFont);
	Tabset_SelectStyle(hdc, &Tabset_GetGdiCollection()->neutralCell, 0);
	SetBkMode(hdc, TRANSPARENT);

	if (g_tab->adjournedCount <= 0 && g_tab->activeCount <= 0)
	{
		DrawText(hdc, L"\n\n\nThere're no games in progress at the moment.", -1, &clientRect, DT_CENTER);
		return;
	}

	Tab_Common_CalcStdSizes(&clientRect, &sizes);
	clientRect.top -= offset;
	rect = clientRect;

	if (g_tab->activeCount)
	{
		RECT rect = clientRect;
		rect.bottom = rect.top+CONFIG_TABS_STD_HEADERHEIGHT;
		DrawText(hdc, L"Games in progress", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		clientRect.top = rect.bottom;

		Tab_Common_DrawSimpleTable(hdc, &clientRect, &sizes, g_tab->games, g_tab->activeCount);
		clientRect.top += sizes.headerHeight + sizes.rowHeight*g_tab->activeCount;
	}

	if (g_tab->adjournedCount)
	{
		RECT rect = clientRect;
		rect.bottom = rect.top+CONFIG_TABS_STD_HEADERHEIGHT;
		DrawText(hdc, L"Adjourned games", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		clientRect.top = rect.bottom;

		Tab_Common_DrawSimpleTable(hdc, &clientRect, &sizes, g_tab->games+g_tab->activeCount, g_tab->adjournedCount);
		clientRect.top += sizes.headerHeight + sizes.rowHeight*g_tab->activeCount;
	}

}

static Status_t Tab_Current_UpdateCheckBox()
{
	RECT rect;
	GetClientRect(g_tab->hwnd, &rect);
	rect.left += CONFIG_TABS_COMMON_CHECKBOXLEFTPAD;
	rect.right -= CONFIG_TABS_COMMON_DROPDOWN_WIDTH;
	if (rect.left > rect.right)
		rect.right = rect.left;

	rect.bottom = rect.top + CONFIG_TABS_COMMON_DROPDOWN_HEIGHT;

	return Tab_Common_UpdateCheckBox(g_tab->hwnd, &rect, Dataset_GetDataset()->isDataValid);
}

static Tab_HitPoint_t Tab_Current_HitTest(POINT point, Tab_Common_SimpleGameData_t *gameData, BOOL* isAdjourned)
{
	Tab_Common_StdTablePaintData_t sizes;
	RECT rect;

	GetClientRect(g_tab->hwnd, &rect);
	rect.top += CONFIG_TABS_COMMON_TOP_MARGIN;
	Tab_Common_CalcStdSizes(&rect, &sizes);

	point.x -= sizes.xOffset;
	point.y -= sizes.yOffset;
	point.y -= CONFIG_TABS_COMMON_TOP_MARGIN;
	point.y -= sizes.headerHeight;
	point.y -= CONFIG_TABS_STD_HEADERHEIGHT;

	if (point.y >= sizes.rowHeight*g_tab->activeCount)
	{
		if (g_tab->activeCount)
		{
			point.y -= sizes.rowHeight*g_tab->activeCount;
			point.y -= sizes.headerHeight;
			point.y -= CONFIG_TABS_STD_HEADERHEIGHT;
		}

		*isAdjourned = TRUE;
		return Tab_Common_StdTableHitTest(point, &sizes, g_tab->games+g_tab->activeCount, g_tab->adjournedCount, gameData);
	}
	else
	{
		*isAdjourned = FALSE;
		return Tab_Common_StdTableHitTest(point, &sizes, g_tab->games, g_tab->activeCount, gameData);
	}
}


static LRESULT CALLBACK Tab_Current_WndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT res = 0;
	Status_t status = Tab_Common_WndProc(hwnd, uMsg, wParam, lParam, &res);

	if (status == STATUS_OK_PROCESSED)
	{
		return res;
	}
	else if (status != STATUS_OK && status != STATUS_OK_NOT_PROCESSED)
	{
		DestroyWindow(hwnd); // TODO make something more sencefull here
		return -1;
	}

	if (g_tab == 0) // We can get here after WM_DESTROY
	{
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	switch(uMsg)
	{
	case WM_PAINT:
		{
			Tab_Common_DoPaint(hwnd, Tab_Current_PaintDC);
			return 0;
		}
	case WM_CREATE:
		{
			/* CREATESTRUCT *createStruct = (CREATESTRUCT*)lParam; */
			g_tab->hwnd = hwnd;
			//			Tab_Finished_UpdateDropDown();
			Tab_Current_UpdateCheckBox();
			Tab_Current_UpdateScrollBar();
			return 0;
		}
	case WM_DESTROY:
		{
			free(g_tab);
			g_tab = 0;
			return 0;
		}
	case WM_SIZE:
		{
			Tab_Current_UpdateScrollBar();
			Tab_Current_UpdateCheckBox();
			return 0;
		}
	case WM_LBUTTONDOWN:
		{
			SetFocus(g_tab->hwnd);
			return 0;
		}
	case WM_RBUTTONUP:
		{
			int x = LOWORD(lParam);
			int y = HIWORD(lParam);
			POINT point;
			Tab_Common_SimpleGameData_t gameData;
			Tab_HitPoint_t hitPoint;
			BOOL isAdjourned;
			int flags = 0;
			point.x = x;
			point.y = y + Tab_Common_GetScrollOffset(hwnd);
			
			hitPoint = Tab_Current_HitTest(point, &gameData, &isAdjourned);

			switch (hitPoint)
			{
			case HITPOINT_Player1:
			case HITPOINT_Player2:
				flags |= MENUTYPE_Team | MENUTYPE_Player | MENUTYPE_Plugin;
			case HITPOINT_Round: // fall through
			case HITPOINT_Section:
				if (isAdjourned)
				{
					flags |= MENUTYPE_GameStartable | MENUTYPE_GameExaminable;
				}
				else
				{
					flags |= MENUTYPE_PlayerObservable;
				}
				break;
			}
			flags |= MENUTYPE_Plugin;
			Tab_Common_DoMenu(g_tab->hwnd, flags, &gameData);
			return 0;
		}

	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

static Status_t Tab_Current_Init()
{
	WNDCLASS wndclass;  
	wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS; 
	wndclass.lpfnWndProc = Tab_Current_WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = Tabset_GetHInstance();
	wndclass.hIcon = NULL;
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW); 
	wndclass.hbrBackground = CreateSolidBrush(Tabset_GetGdiCollection()->bgColor); // TODO fix it
	wndclass.lpszMenuName =  NULL;
	wndclass.lpszClassName = L"moomoo_CurrentTab";
	if (!RegisterClass(&wndclass))
	{
		return STATUS_ERR_WINDOW_CANNOT_REGISTER;
	}

	return STATUS_OK;
}

static Status_t Tab_Current_Event(TabEvent_t event)
{
	switch(event)
	{
	case TABEVENT_CONFIG_CHANGED:
	case TABEVENT_DATA_UPDATED:
		{
			Status_t status = Tab_Current_UpdateData();
			RECT rect;
			if (status != STATUS_OK)
			{
				return status;
			}

			Tab_Current_UpdateScrollBar();
			Tab_Current_UpdateCheckBox();

			GetClientRect(g_tab->hwnd, &rect); 
			InvalidateRect(g_tab->hwnd, &rect, FALSE);
			return STATUS_OK;
		}

	case TABEVENT_INIT:
		return Tab_Current_Init();
	}
	return STATUS_OK_NOT_PROCESSED;
}

static Status_t Tab_Current_Create(HWND parent, HWND* child)
{
	g_tab = (Tab_Current_t*)malloc(sizeof(Tab_Current_t));
	if (!g_tab)
	{
		return STATUS_ERR_NOT_ENOUGH_ROOM;
	}

	Tab_Current_UpdateData();
	g_tab->hwnd = CreateWindow(L"moomoo_CurrentTab", L"", WS_CHILD|WS_VISIBLE|WS_VSCROLL, 0, 0, 0, 0, parent, 0, Tabset_GetHInstance(), 0);
	if (!g_tab->hwnd)
	{
		return STATUS_ERR_WINDOW_CANNOT_CREATE;
	}
	*child = g_tab->hwnd;
	return STATUS_OK;
}

static Status_t Tab_Current_GetTabTitle(char* title, int length)
{
	int favCount = 0;
	int allCount = 0;
	Status_t status = Tab_Common_PopulateStdData(TRUE, NULL, &favCount, NOT_VALID31, GAMESTATUS_InProgress);

	if (status == STATUS_OK)
	{
		status = Tab_Common_PopulateStdData(FALSE, NULL, &allCount, NOT_VALID31, GAMESTATUS_InProgress);
	}

	if (status == STATUS_OK)
	{
		sprintf_s(title, length, "Current (%d/%d)", favCount, allCount);
	}
	else
	{
		sprintf_s(title, length, "Current");
	}

	return STATUS_OK;
}


Status_t Tab_Current_GetHandlers(TabHandlers_t* handlers)
{
	handlers->onCreate = Tab_Current_Create;
	handlers->onEvent = Tab_Current_Event;
	handlers->onGetTabTitle = Tab_Current_GetTabTitle;
	return STATUS_OK;
}