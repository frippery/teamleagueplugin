#include <vector>

#include "tab_common.h"
#include "dataset.h"
#include "settings.h"
#include <stdio.h>

//////////////////////////////////////////////////////////////////////////
// This file is made using copy-paste of copy-paste method. :) Sorry
//////////////////////////////////////////////////////////////////////////

typedef struct 
{
  std::vector<Basic_GameData_t> games;
	HWND hwnd;
} Tab_NotScheduled_t;

static Tab_NotScheduled_t *g_tab = 0;


static Status_t Tab_NotScheduled_UpdateData() {
	Status_t status;

	BOOL onlyFavorites = Settings::Get()->settings().showfavoritesonly();

	status = Tab_Common_PopulateStdData(onlyFavorites, &g_tab->games, CONFIG_MAX_GAMES_PER_ROUND, GAMESTATUS_NotScheduled);

	if (status != STATUS_OK) {
    g_tab->games.clear();
	}

	if (status == STATUS_ERR_DATA_INVALID) {
		status = STATUS_OK;
	}

	return status;
}


static void Tab_NotScheduled_UpdateScrollBar()
{
	Tab_Common_StdTablePaintData_t sizes;
	RECT rect;
	int totalHeight = 0;
	SCROLLINFO scrollinfo;

	if (g_tab->games.empty()) {
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

	totalHeight = sizes.headerHeight + sizes.rowHeight*g_tab->games.size();

	scrollinfo.cbSize = sizeof(scrollinfo);
	scrollinfo.fMask = SIF_PAGE | SIF_RANGE;
	scrollinfo.nMin = 0;
	scrollinfo.nMax = totalHeight;
	scrollinfo.nPage = rect.bottom - rect.top;

	SetScrollInfo(g_tab->hwnd, SB_VERT, &scrollinfo, TRUE);
}


static void Tab_NotScheduled_PaintDC(HDC hdc, int offset, RECT clientRect)
{
	RECT rect;
	HFONT font = Tabset_GetGdiCollection()->normalFont;
	Tab_Common_StdTablePaintData_t sizes;

	SelectObject(hdc, font);
	Tabset_SelectStyle(hdc, &Tabset_GetGdiCollection()->neutralCell, 0);
	SetBkMode(hdc, TRANSPARENT);

	if (g_tab->games.empty())	{
		DrawText(hdc, L"\n\n\nThere're no unscheduled games.", -1, &clientRect, DT_CENTER);
		return;
	}

	Tab_Common_CalcStdSizes(&clientRect, &sizes);
	clientRect.top -= offset;
	rect = clientRect;

	Tab_Common_DrawSimpleTable(hdc, &clientRect, &sizes, g_tab->games);
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

	return Tab_Common_UpdateCheckBox(g_tab->hwnd, &rect, Dataset::Get().is_data_valid());
}

static Tab_HitPoint_t Tab_NotScheduled_HitTest(POINT point, Tab_Common_SimpleGameData_t *gameData)
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

	return Tab_Common_StdTableHitTest(point, &sizes, g_tab->games, gameData);
}

static LRESULT CALLBACK Tab_NotScheduled_WndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
			Tab_Common_DoPaint(hwnd, Tab_NotScheduled_PaintDC);
			return 0;
		}
	case WM_COMMAND:
		{
			/*			HWND sender = (HWND)(lParam);
			if (sender == g_tab->dropDownHwnd && HIWORD(wParam) == CBN_SELCHANGE)
			{
			int newSel = (int)SendMessage(sender, CB_GETCURSEL, 0, 0);
			RECT rect;
			g_tab->curRound = newSel;
			Settings_GetSettings()->curFinishedDropDownRoundNum = newSel;
			GetClientRect(hwnd, &rect);
			Tab_Finished_UpdateScrollBar();
			rect.top += CONFIG_TABS_COMMON_TOP_MARGIN;
			InvalidateRect(hwnd, &rect, TRUE);
			return 0;
			}
			*/		}
		break;
	case WM_CREATE:
		{
			/* CREATESTRUCT *createStruct = (CREATESTRUCT*)lParam; */
			g_tab->hwnd = hwnd;
			//			Tab_Finished_UpdateDropDown();
			Tab_NotScheduled_UpdateScrollBar();
			Tab_Current_UpdateCheckBox();
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
			Tab_NotScheduled_UpdateScrollBar();
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
			int flags = 0;
			point.x = x;
			point.y = y + Tab_Common_GetScrollOffset(hwnd);

			hitPoint = Tab_NotScheduled_HitTest(point, &gameData);

			switch (hitPoint)
			{
			case HITPOINT_Player1:
			case HITPOINT_Player2:
				flags |= MENUTYPE_Team | MENUTYPE_Player | MENUTYPE_Plugin | MENUTYPE_GameStartable;
				break;
			case HITPOINT_Round: // fall through
			case HITPOINT_Section:
				flags |= MENUTYPE_GameStartable;
				break;
			}
			flags |= MENUTYPE_Plugin;
			Tab_Common_DoMenu(g_tab->hwnd, flags, &gameData);
			return 0;
		}

	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

static Status_t Tab_NotScheduled_Init()
{
	WNDCLASS wndclass;  
	wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS; 
	wndclass.lpfnWndProc = Tab_NotScheduled_WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = Tabset_GetHInstance();
	wndclass.hIcon = NULL;
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW); 
	wndclass.hbrBackground = CreateSolidBrush(Tabset_GetGdiCollection()->bgColor); // TODO fix it
	wndclass.lpszMenuName =  NULL;
	wndclass.lpszClassName = L"moomoo_NotScheduledTab";
	if (!RegisterClass(&wndclass))
	{
		return STATUS_ERR_WINDOW_CANNOT_REGISTER;
	}

	return STATUS_OK;
}

static Status_t Tab_NotScheduled_Event(TabEvent_t event)
{
	switch(event)
	{
	case TABEVENT_CONFIG_CHANGED:
	case TABEVENT_DATA_UPDATED:
		{
			Status_t status = Tab_NotScheduled_UpdateData();
			RECT rect;
			if (status != STATUS_OK)
			{
				return status;
			}

			Tab_NotScheduled_UpdateScrollBar();
			Tab_Current_UpdateCheckBox();

			GetClientRect(g_tab->hwnd, &rect); 
			InvalidateRect(g_tab->hwnd, &rect, TRUE);
			return STATUS_OK;
		}

	case TABEVENT_INIT:
		return Tab_NotScheduled_Init();
	}
	return STATUS_OK_NOT_PROCESSED;
}

static Status_t Tab_NotScheduled_Create(HWND parent, HWND* child)
{
	g_tab = (Tab_NotScheduled_t*)malloc(sizeof(Tab_NotScheduled_t));
	if (!g_tab)
	{
		return STATUS_ERR_NOT_ENOUGH_ROOM;
	}

	Tab_NotScheduled_UpdateData();
	g_tab->hwnd = CreateWindow(L"moomoo_NotScheduledTab", L"", WS_CHILD|WS_VISIBLE|WS_VSCROLL, 0, 0, 0, 0, parent, 0, Tabset_GetHInstance(), 0);
	if (!g_tab->hwnd)
	{
		return STATUS_ERR_WINDOW_CANNOT_CREATE;
	}
	*child = g_tab->hwnd;
	return STATUS_OK;
}

static Status_t Tab_NotScheduled_GetTabTitle(char* title, int length)
{
	int favCount = 0;
	int allCount = 0;
	Status_t status = Tab_Common_PopulateStdData(TRUE, NULL, &favCount, NOT_VALID31, GAMESTATUS_NotScheduled);

	if (status == STATUS_OK)
	{
		status = Tab_Common_PopulateStdData(FALSE, NULL, &allCount, NOT_VALID31, GAMESTATUS_NotScheduled);
	}

	if (status == STATUS_OK)
	{
		sprintf_s(title, length, "Not scheduled (%d/%d)", favCount, allCount);
	}
	else
	{
		sprintf_s(title, length, "Not scheduled");
	}

	return STATUS_OK;
}

Status_t Tab_NotScheduled_GetHandlers(TabHandlers_t* handlers)
{
	handlers->onCreate = Tab_NotScheduled_Create;
	handlers->onEvent = Tab_NotScheduled_Event;
	handlers->onGetTabTitle = Tab_NotScheduled_GetTabTitle;
	return STATUS_OK;
}