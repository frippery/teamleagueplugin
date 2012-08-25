#define _WIN32_WINNT 0x0501

#include "tab_common.h"
#include <Windows.h>
#include <CommCtrl.h>
#include "tabset.h"
#include "config.h"
#include "dataset.h"
#include "engine.h"
#include <stdio.h>
#include "settings.h"
#include <assert.h>

typedef struct  
{
	HWND buttonHwnd;
	HWND showOnlyFavsHwnd;
} Tab_Common_t;

static Tab_Common_t g_tabCommon;

Status_t Tab_Common_Init()
{
	g_tabCommon.buttonHwnd = 0;
	g_tabCommon.showOnlyFavsHwnd = 0;
	InitCommonControls(); 
	return STATUS_OK;
}

static void Tab_Common_UpdateButtonStatus()
{
	if (g_tabCommon.buttonHwnd) {
		if (Dataset::Get().is_update_in_progress())	{
			SendMessage(g_tabCommon.buttonHwnd, WM_SETTEXT, (WPARAM)0, (LPARAM)L"Updating...");
		}	else {
			SendMessage(g_tabCommon.buttonHwnd, WM_SETTEXT, (WPARAM)0, (LPARAM)L"Update");
		}
		EnableWindow(g_tabCommon.buttonHwnd, Engine_IsOnline() && !Dataset::Get().is_update_in_progress());
	}
}

Status_t Tab_Common_Event(TabEvent_t event)
{
	switch(event)
	{
	case TABEVENT_DATA_UPDATE_REQUESTED:
		{
			if (g_tabCommon.buttonHwnd)
			{
				EnableWindow(g_tabCommon.buttonHwnd, FALSE);
			}
			return STATUS_OK;
		}
	
	case TABEVENT_DATA_UPDATED:
	case TABEVENT_DATA_UPDATING:
	case TABEVENT_DATA_UPDATE_ERROR:
	case TABEVENT_ONLINE:
	case TABEVENT_OFFLINE:
		{
			Tab_Common_UpdateButtonStatus();
			return STATUS_OK;
		}
	};
	return STATUS_OK_NOT_PROCESSED;
}

Status_t Tab_Common_WndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *res)
{
	(void)res;
	switch(uMsg)
	{
	case WM_CTLCOLORBTN:
	case WM_CTLCOLORSTATIC:
		{
/*			HDC dc = (HDC)wParam;
			SetTextColor(dc, RGB(0,255,0)); // TabSet_GetFgColor()); */
			*res = (LRESULT)Tabset_GetGdiCollection()->neutralCell.brush;
			return STATUS_OK_PROCESSED;
		}
	case WM_CREATE:
		{
			RECT rcClient; 
			GetClientRect(hwnd, &rcClient); 
			g_tabCommon.buttonHwnd = CreateWindowEx(0, WC_BUTTON, L"Update", WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE ,
				                                     0, 0, rcClient.right, rcClient.bottom, hwnd, NULL, Tabset_GetHInstance(), NULL);
			if (g_tabCommon.buttonHwnd == 0)
			{
				return STATUS_ERR_WINDOW_CANNOT_CREATE;
			}
			SendMessage(g_tabCommon.buttonHwnd, WM_SETFONT, (WPARAM)Tabset_GetGdiCollection()->normalFont, TRUE);
			Tab_Common_UpdateButtonStatus();
			return STATUS_OK;
		}
	case WM_SIZE:
		{
			RECT rcClient; 
			GetClientRect(hwnd, &rcClient);
			MoveWindow(g_tabCommon.buttonHwnd, rcClient.right-CONFIG_TABS_UPDATE_BUTTON_WIDTH,
				         rcClient.top, CONFIG_TABS_UPDATE_BUTTON_WIDTH, CONFIG_TABS_UPDATE_BUTTON_HEIGHT, TRUE);
			return STATUS_OK;
		}
	case WM_VSCROLL:
		{
			RECT rect;
			SCROLLINFO scrollinfo;

			scrollinfo.cbSize = sizeof(SCROLLINFO);
			scrollinfo.fMask = SIF_ALL;
			GetScrollInfo(hwnd, SB_VERT, &scrollinfo);
			scrollinfo.fMask = SIF_POS;

			switch (LOWORD(wParam))
			{
			case SB_THUMBPOSITION:
			case SB_THUMBTRACK:
				scrollinfo.nPos = scrollinfo.nTrackPos;
				break;
			case SB_BOTTOM:
				scrollinfo.nPos = scrollinfo.nMax;
				break;
			case SB_TOP:
				scrollinfo.nPos = scrollinfo.nMin;
				break;
			case SB_LINEDOWN:
				scrollinfo.nPos += CONFIG_TABS_SCROLLBAR_STEP;
				break;
			case SB_LINEUP:
				scrollinfo.nPos -= CONFIG_TABS_SCROLLBAR_STEP;
				break;
			case SB_PAGEDOWN:
				scrollinfo.nPos += scrollinfo.nPage;
				break;
			case SB_PAGEUP:
				scrollinfo.nPos -= scrollinfo.nPage;
				break;
			}
			SetScrollInfo(hwnd, SB_VERT, &scrollinfo, TRUE);

			GetClientRect(hwnd, &rect);
			rect.top += CONFIG_TABS_COMMON_TOP_MARGIN;
			InvalidateRect(hwnd, &rect, FALSE);

			return STATUS_OK_PROCESSED;
		}
	case WM_DESTROY:
		{
			g_tabCommon.showOnlyFavsHwnd = 0;
			return STATUS_OK;
		}
	case WM_COMMAND:
		{
			HWND sender = (HWND)(lParam);
			if (sender == g_tabCommon.buttonHwnd)
			{
				Engine_RequestData();
				SetFocus(hwnd);
				return STATUS_OK_PROCESSED;
			}
			else if (sender == g_tabCommon.showOnlyFavsHwnd)
			{
				bool showOnlyFavs = (SendMessage(g_tabCommon.showOnlyFavsHwnd, BM_GETCHECK, 0, 0) == BST_CHECKED);
				Settings::Get()->settings().set_showfavoritesonly(showOnlyFavs);
				Settings::Get()->Store();
				Tabset_Event(TABEVENT_CONFIG_CHANGED);
			}
			return STATUS_OK_NOT_PROCESSED;
		}
	case WM_MOUSEWHEEL:
		{
			RECT rect;
			SCROLLINFO scrollinfo;

			scrollinfo.cbSize = sizeof(SCROLLINFO);
			scrollinfo.fMask = SIF_POS;
			GetScrollInfo(hwnd, SB_VERT, &scrollinfo);

			scrollinfo.nPos -= (signed short)HIWORD(wParam) / 3;

			scrollinfo.fMask = SIF_POS;
			SetScrollInfo(hwnd, SB_VERT, &scrollinfo, TRUE);

			GetClientRect(hwnd, &rect);
			rect.top += CONFIG_TABS_COMMON_TOP_MARGIN;
			InvalidateRect(hwnd, &rect, FALSE);
			
			return STATUS_OK_PROCESSED;
		}
	case WM_PAINT:
		{
			if (!Dataset::Get().is_data_valid()) {
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hwnd, &ps);
				HFONT font = Tabset_GetGdiCollection()->normalFont;
				RECT rect;

				SetTextColor(hdc, Tabset_GetGdiCollection()->fgColor);
				SetBkMode(hdc, TRANSPARENT);

				SelectObject(hdc, font);
				GetClientRect(hwnd, &rect);

				DrawText(hdc, L"\n\n\nTournament data has not been updated yet.\nPress \"Update\" button in top right corner to update.", -1, &rect, 
					        DT_CENTER);

				DrawTextA(hdc, PLUGIN_REVISION ". Build date: " __DATE__ " " __TIME__ , -1, &rect, 
					DT_BOTTOM | DT_RIGHT | DT_SINGLELINE);

				EndPaint(hwnd, &ps);

				return STATUS_OK_PROCESSED;
			}
			return STATUS_OK_NOT_PROCESSED;
		}
	default:
		return STATUS_OK_NOT_PROCESSED;
	}
}

Status_t Tab_Common_UpdateDropDown(HWND *dropDownHwnd, HWND parent, BOOL hasData, int width, int height)
{
	if (*dropDownHwnd)
	{
		SendMessage(*dropDownHwnd, CB_RESETCONTENT, 0, 0);
	}

	(void)hasData;
	if (!hasData)
	{
		if (*dropDownHwnd)
		{
			DestroyWindow(*dropDownHwnd);
			*dropDownHwnd = 0;
		}

		return STATUS_OK;
	}

	if (*dropDownHwnd == 0)
	{
		RECT rcClient; 
		GetClientRect(parent, &rcClient); 
		*dropDownHwnd = CreateWindowExA(0, WC_COMBOBOXA, "", WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL ,
			rcClient.left, rcClient.top, width, height, parent, NULL, Tabset_GetHInstance(), NULL);

	}
	if (*dropDownHwnd == 0)
	{
		return STATUS_ERR_WINDOW_CANNOT_CREATE;
	}

	SendMessage(*dropDownHwnd, WM_SETFONT, (WPARAM)Tabset_GetGdiCollection()->normalFont, TRUE);

	return STATUS_OK;
}

Status_t Tab_Common_UpdateCheckBox(HWND parent, const RECT* rect, BOOL show)
{
	if (!show)
	{
		if (g_tabCommon.showOnlyFavsHwnd)
		{
			DestroyWindow(g_tabCommon.showOnlyFavsHwnd);
			g_tabCommon.showOnlyFavsHwnd = 0;
		}
		return STATUS_OK;
	}

	if (g_tabCommon.showOnlyFavsHwnd == 0)
	{
		g_tabCommon.showOnlyFavsHwnd = CreateWindowEx(0, WC_BUTTON, L"Show only favorite teams", BS_AUTOCHECKBOX | WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
			rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top, parent, NULL, Tabset_GetHInstance(), NULL);
	}
	else
	{
		MoveWindow(g_tabCommon.showOnlyFavsHwnd, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top, TRUE);
	}

	if (g_tabCommon.showOnlyFavsHwnd == 0)
	{
		return STATUS_ERR_WINDOW_CANNOT_CREATE;
	}
	SendMessage(g_tabCommon.showOnlyFavsHwnd, WM_SETFONT, (WPARAM)Tabset_GetGdiCollection()->normalFont, TRUE);

	if (Settings::Get()->settings().showfavoritesonly()) {
		SendMessage(g_tabCommon.showOnlyFavsHwnd, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
	} else {
		SendMessage(g_tabCommon.showOnlyFavsHwnd, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
	}
	return STATUS_OK;
}

const TabSet_ColorStyle_t* Tab_Common_GetGameStyle(int minScore, int maxScore)
{
	if (minScore == 2)
	{
		return &Tabset_GetGdiCollection()->goodCell;
	}
	else if (minScore == 1 && maxScore == 1)
	{
		return &Tabset_GetGdiCollection()->pourCell;
	}
	else if (minScore == 1)
	{
		return &Tabset_GetGdiCollection()->pourOrGoodCell;
	}
	else if (maxScore == 0)
	{
		return &Tabset_GetGdiCollection()->badCell;
	}
	else if (maxScore == 1)
	{
		return &Tabset_GetGdiCollection()->pourOrBadCell;
	}

	return &Tabset_GetGdiCollection()->neutralCell;
}

const TabSet_ColorStyle_t* Tab_Common_GetTeamGameStyle(int minScore, int maxScore)
{
	if (minScore > 4) minScore = 2;
	else if (minScore < 4) minScore = 0;
	else minScore = 1;

	if (maxScore > 4) maxScore = 2;
	else if (maxScore < 4) maxScore = 0;
	else maxScore = 1;

	return Tab_Common_GetGameStyle(minScore, maxScore);
}

const TabSet_ColorStyle_t* Tab_Common_GetTeamGameStyleByStatus(Game_Status_t status)
{
	switch(status)
	{
	case GAMESTATUS_Win_1_0:
	case GAMESTATUS_Win_i_o:
	case GAMESTATUS_Win_plus_minus:
		return &Tabset_GetGdiCollection()->goodCell;

	case GAMESTATUS_Lose_0_1:
	case GAMESTATUS_Lose_o_i:
	case GAMESTATUS_Lose_minus_plus:
		return &Tabset_GetGdiCollection()->badCell;

	case GAMESTATUS_Draw_12_12:
	case GAMESTATUS_Draw_i2_i2:
	case GAMESTATUS_DoubleForfeit_o_o:
	case GAMESTATUS_DoubleForfeit_minus_minus:
		return &Tabset_GetGdiCollection()->pourCell;
	}

	return &Tabset_GetGdiCollection()->neutralCell;
}

const TabSet_ColorStyle_t* Tab_Common_GetTeamGametStyle(const TeamGame_t *teamGame)
{
	int minScore = 0;
	int maxScore = 4*2;
	int i;

	for(i = 0; i < teamGame->team_games_size(); ++i) {
		switch(teamGame->team_games(i).status()) {
		case GAMESTATUS_Win_1_0:
		case GAMESTATUS_Win_i_o:
		case GAMESTATUS_Win_plus_minus:
			minScore += 2;
			break;

		case GAMESTATUS_Lose_0_1:
		case GAMESTATUS_Lose_o_i:
		case GAMESTATUS_Lose_minus_plus:
			maxScore -= 2;
			break;

		case GAMESTATUS_Draw_12_12:
		case GAMESTATUS_Draw_i2_i2:
		case GAMESTATUS_DoubleForfeit_o_o:
		case GAMESTATUS_DoubleForfeit_minus_minus:
			minScore += 1;
			maxScore -= 1;
			break;
		}
	}
	return Tab_Common_GetTeamGameStyle(minScore, maxScore);
}

int Tab_Common_GetScrollOffset(HWND hwnd)
{
	SCROLLINFO scrollinfo;
	scrollinfo.cbSize = sizeof(scrollinfo);
	scrollinfo.fMask = SIF_POS;
	GetScrollInfo(hwnd, SB_VERT, &scrollinfo);
	return scrollinfo.nPos;
}

void Tab_Common_DoPaint(HWND hwnd, PaintDCFunc_t func)
{
	RECT clientRect;
	PAINTSTRUCT ps;
	HDC dc;
	HBITMAP hbitmap;

	BeginPaint(hwnd, &ps);
	GetClientRect(hwnd, &clientRect);
	clientRect.top += CONFIG_TABS_COMMON_TOP_MARGIN;


	dc = CreateCompatibleDC(ps.hdc);
	hbitmap = (HBITMAP)CreateCompatibleBitmap(ps.hdc, clientRect.right-clientRect.left, clientRect.bottom - clientRect.top);
	hbitmap = (HBITMAP)SelectObject(dc, hbitmap);

	{
		TabSet_ColorStyle_t oldStyle;
		HPEN pen = CreatePen(PS_NULL, 0, 0);
		Tabset_SelectStyle(dc, &Tabset_GetGdiCollection()->neutralCell, &oldStyle);
		SelectObject(dc, pen);
		Rectangle(dc, 0, 0, clientRect.right-clientRect.left+1, clientRect.bottom-clientRect.top+1);
		Tabset_SelectStyle(dc, &oldStyle, 0);
		DeleteObject(pen);
	}
	{
		RECT userRect = clientRect;
		int scrollOffset = Tab_Common_GetScrollOffset(hwnd);
		TabSet_ColorStyle_t oldStyle;

		Tabset_SelectStyle(dc, &Tabset_GetGdiCollection()->neutralCell, &oldStyle);

		userRect.bottom -= userRect.top;
		userRect.right -= userRect.left;
		userRect.top = 0;
		userRect.left = 0;

		func(dc, scrollOffset, userRect);

		Tabset_SelectStyle(dc, &oldStyle, 0);
	}

	BitBlt(ps.hdc, clientRect.left, clientRect.top, clientRect.right-clientRect.left, clientRect.bottom - clientRect.top,
		dc, 0, 0, SRCCOPY);

	hbitmap = (HBITMAP)SelectObject(dc, hbitmap);
	DeleteObject(hbitmap);
	DeleteObject(dc);

	EndPaint(hwnd, &ps);
}

void Tab_Common_CalcStdSizes(const RECT* rect, Tab_Common_StdTablePaintData_t* sizes)
{
	int baseWidth;
	sizes->yOffset = 0;
	sizes->totalWidth = rect->right - rect->left - 2;

	if (sizes->totalWidth > CONFIG_TABS_STD_MAX_TABLEWIDTH)
	{
		sizes->totalWidth = CONFIG_TABS_STD_MAX_TABLEWIDTH;
	}

	baseWidth =  sizes->totalWidth / 9;
	sizes->totalWidth = baseWidth * 9;

	sizes->xOffset = ((rect->right - rect->left) - sizes->totalWidth) / 2;
	sizes->headerHeight = CONFIG_TABS_STD_HEADERHEIGHT;
	sizes->rowHeight = CONFIG_TABS_STD_GAMEHEIGHT;

	sizes->roundWidth = baseWidth;
	sizes->sectionWidth = baseWidth * 2;
	sizes->playerWidth = baseWidth * 3;
}

Tab_HitPoint_t Tab_Common_StdTableHitTest(POINT point, const Tab_Common_StdTablePaintData_t *sizes, const vector<Basic_GameData_t>& data, 
																          Tab_Common_SimpleGameData_t *gameData)
{
	int index;
	if (point.x < 0 || point.y < 0 || point.y >= sizes->rowHeight*count || point.x >= sizes->totalWidth )
	{
		return HITPOINT_Milk;
	}

	index = point.y / sizes->rowHeight;
	if (index >= count)
	{
		assert(FALSE);
		return HITPOINT_Milk;
	}

	gameData->gameId = data[index].gameId;
  gameData->player1 = data[index].player1;
  gameData->player2 = data[index].player2;
  gameData->team1 = data[index].team1;
  gameData->team2 = data[index].team2;

	if (point.x < sizes->roundWidth)
	{
		return HITPOINT_Round;
	}
	point.x -= sizes->roundWidth;

	if (point.x < sizes->sectionWidth)
	{
		return HITPOINT_Section;
	}
	point.x -= sizes->sectionWidth;

	if (point.x < sizes->playerWidth)
	{
		return HITPOINT_Player1;
	}
	point.x -= sizes->playerWidth;

	if (point.x < sizes->playerWidth)	{
    std::swap(gameData->player1, gameData->player2);
    std::swap(gameData->team1, gameData->team2);
		return HITPOINT_Player2;
	}

	assert(FALSE);
	return HITPOINT_Milk;
}

void Tab_Common_DrawSimpleTable( HDC hdc, RECT *clientRect, const Tab_Common_StdTablePaintData_t *sizes, const std::vector<Basic_GameData_t>& data) {
	int i;
	RECT rect = *clientRect;

	//////////////////////////////////////////////////////////////////////////
	/// Drawing Header
	//////////////////////////////////////////////////////////////////////////

	rect.top    = clientRect->top + sizes->yOffset;
	rect.left   = clientRect->left + sizes->xOffset;
	rect.bottom = rect.top + sizes->headerHeight;

	rect.right = rect.left + sizes->roundWidth;
	DrawText(hdc,  L"Round", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	rect.left   = rect.right;

	rect.right = rect.left + sizes->sectionWidth;
	DrawText(hdc,  L"Section", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	rect.left   = rect.right;

	rect.right = rect.left + sizes->playerWidth;
	DrawText(hdc,  L"White", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	rect.left   = rect.right;

	rect.right = rect.left + sizes->playerWidth;
	DrawText(hdc,  L"Black", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	rect.left   = rect.right;
	rect.top = rect.bottom;

	for(i = 0; i < count; ++i)
	{
		const Basic_GameData_t *game = &data[i];
		rect.left = clientRect->left + sizes->xOffset;
		rect.right = rect.left + sizes->totalWidth;
		rect.bottom = rect.top + sizes->rowHeight;
		Rectangle(hdc, rect.left, rect.top, rect.right+1, rect.bottom+1);
		{
			int top = rect.top;
			char buf[256];

			rect.right = rect.left + sizes->roundWidth;
			sprintf_s(buf, sizeof(buf), "%d", game->Round);
			DrawTextA(hdc, buf, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE /*| DT_END_ELLIPSIS*/);
			rect.left = rect.right;

			rect.right = rect.left + sizes->sectionWidth;
			sprintf_s(buf, sizeof(buf), "%s", game->secton);
			DrawTextA(hdc, buf, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
			rect.left = rect.right;

			rect.right = rect.left + sizes->playerWidth;
			rect.bottom = rect.top + CONFIG_TABS_UPCOMING_LINEHEIGHT;
			sprintf_s(buf, sizeof(buf), "%s", game->player1);
			DrawTextA(hdc, buf, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
			rect.top = rect.bottom;
			rect.bottom = rect.top + CONFIG_TABS_UPCOMING_LINEHEIGHT;
			sprintf_s(buf, sizeof(buf), "(%s)", game->team1);
			DrawTextA(hdc, buf, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
			rect.left = rect.right;

			rect.top = top;
			rect.right = rect.left + sizes->playerWidth;
			rect.bottom = rect.top + CONFIG_TABS_UPCOMING_LINEHEIGHT;
			sprintf_s(buf, sizeof(buf), "%s", game->player2);
			DrawTextA(hdc, buf, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
			rect.top = rect.bottom;
			rect.bottom = rect.top + CONFIG_TABS_UPCOMING_LINEHEIGHT;
			sprintf_s(buf, sizeof(buf), "(%s)", game->team2);
			DrawTextA(hdc, buf, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
			rect.left = rect.right;
		}
		rect.top = rect.bottom;
	}
	{
		int top = clientRect->top + sizes->yOffset + sizes->headerHeight;
		int bottom = rect.bottom;
		int left = clientRect->left + sizes->xOffset;

		left += sizes->roundWidth;
		MoveToEx(hdc, left, top, 0);
		LineTo(hdc, left, bottom);

		left += sizes->sectionWidth;
		MoveToEx(hdc, left, top, 0);
		LineTo(hdc, left, bottom);

		left += sizes->playerWidth;
		MoveToEx(hdc, left, top, 0);
		LineTo(hdc, left, bottom);

		left += sizes->playerWidth;
		MoveToEx(hdc, left, top, 0);
		LineTo(hdc, left, bottom);
	}
}

Status_t Tab_Common_PopulateStdData(BOOL onlyFavorites, 
                                    std::vector<Basic_GameData_t>* entries,
                                    int maxCount, Game_Status_t status ) {
	const Dataset& data = Dataset::Get();
	int i;

	if (!data.is_data_valid()) {
		return STATUS_ERR_DATA_INVALID;
	}

  entries->clear();

	for(i = 0; i < data.get_season().team_games_size(); ++i) {
		const TeamGame_t& teamGame = data.get_season().team_games(i);
		BOOL processThisTeamGame = TRUE;

		BOOL team1IsFavorite = FALSE;
		BOOL team2IsFavorite = TRUE;
		int j;

		if (onlyFavorites) {
			team1IsFavorite = Settings::Get()->IsTeamFavorite(teamGame.team1());
			team2IsFavorite = Settings::Get()->IsTeamFavorite(teamGame.team2());
			processThisTeamGame = team1IsFavorite || team2IsFavorite;
		}

		if (!processThisTeamGame)	{
			continue;
		}

		for(j = 0; j < teamGame.team_games_size(); ++j) {
			const Game_t* game = &teamGame.team_games(j);
			if (game->status() != status)	{
				continue;
			}

			{
        Basic_GameData_t newEntry;
				newEntry.gameId = game->game_id();
				newEntry.Round = teamGame.round();

        newEntry.secton = teamGame.section();

				if (game->is_player1_white())	{
          newEntry.team1 = teamGame.team1();
          newEntry.team2 = teamGame.team2();
          newEntry.player1 = game->player1();
          newEntry.player2 = game->player2();
				}	else {
          newEntry.team1 = teamGame.team2();
          newEntry.team2 = teamGame.team1();
          newEntry.player1 = game->player2();
          newEntry.player2 = game->player1();
				}
        entries->push_back(newEntry);
			}
		}
	}
	return STATUS_OK;
}

typedef enum
{
	// Common
	MENUITEM_Plugin_Settings = 1,
	MENUITEM_Plugin_UpdateData,

	// Team
	MENUITEM_Add_Team_To_Favs,
	MENUITEM_List_a_Team,
	// MENUITEM_Plugin_ObserveAll,

	// Team
	MENUITEM_Add_Team2_To_Favs,
	MENUITEM_List_a_Team2,
	// MENUITEM_Plugin_ObserveAll,

	// Player
	MENUITEM_Player_Follow,
	MENUITEM_Player_Finger,
	MENUITEM_Player_Add_To_Notify,
	MENUITEM_Player_Remove_From_Notify,
	MENUITEM_Player_Observe,
	// MENUITEM_Player_Add_To_Autoobserve,

	// Game
	MENUITEM_Game_Observe,
	MENUITEM_Game_Start,
	MENUITEM_Game_Examine,
} Tab_COmmon_MenuItem;

Status_t Tab_Common_DoMenu(HWND hwnd, unsigned int flags, const Tab_Common_SimpleGameData_t* gameData)
{
	POINT point;
	HMENU menu = CreatePopupMenu();
	MENUITEMINFO item;
	int selectedItem;
	char buf[256];
	item.cbSize = sizeof(item);
	item.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
	item.fType = MFT_STRING;

	(void)gameData;


	if (flags & MENUTYPE_GameStartable && Engine_IsOnline() && 
		  (gameData->player1 == Engine_GetUserName() || gameData->player2 == Engine_GetUserName()))	{
		MENUITEMINFOA item;
		item.cbSize = sizeof(item);
		item.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
		item.fType = MFT_STRING;

		sprintf_s(buf, sizeof(buf), "Play this game now!");
		item.wID = MENUITEM_Game_Start;
		item.dwTypeData =  buf;
		item.cch = (UINT)strlen(buf)+1;
		item.fState = MFS_ENABLED;
		InsertMenuItemA(menu, 70, FALSE, &item);

		item.fMask = MIIM_TYPE | MIIM_STATE;
		item.fType = MFT_SEPARATOR;
		item.fState = MFS_ENABLED;
		InsertMenuItemA(menu, 70, FALSE, &item);
	}

	if (flags & MENUTYPE_GameExaminable)
	{
		MENUITEMINFOA item;
		item.cbSize = sizeof(item);
		item.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
		item.fType = MFT_STRING;

		sprintf_s(buf, sizeof(buf), "Examine this game");
		item.wID = MENUITEM_Game_Examine;
		item.dwTypeData =  buf;
		item.cch = (UINT)strlen(buf)+1;
		item.fState = MFS_ENABLED;
		InsertMenuItemA(menu, 70, FALSE, &item);

		item.fMask = MIIM_TYPE | MIIM_STATE;
		item.fType = MFT_SEPARATOR;
		item.fState = MFS_ENABLED;
		InsertMenuItemA(menu, 70, FALSE, &item);
	}

	if (flags & MENUTYPE_GameObserveable)
	{
		MENUITEMINFOA item;
		item.cbSize = sizeof(item);
		item.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
		item.fType = MFT_STRING;

		sprintf_s(buf, sizeof(buf), "Observe this game");
		item.wID = MENUITEM_Game_Observe;
		item.dwTypeData =  buf;
		item.cch = (UINT)strlen(buf)+1;
		item.fState = MFS_ENABLED;
		InsertMenuItemA(menu, 70, FALSE, &item);

		item.fMask = MIIM_TYPE | MIIM_STATE;
		item.fType = MFT_SEPARATOR;
		item.fState = MFS_ENABLED;
		InsertMenuItemA(menu, 70, FALSE, &item);
	}

	if (flags & MENUTYPE_Player)
	{
		MENUITEMINFOA item;
		item.cbSize = sizeof(item);
		item.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
		item.fType = MFT_STRING;

		if ((flags & MENUTYPE_PlayerObservable) == MENUTYPE_PlayerObservable)
		{
			sprintf_s(buf, sizeof(buf), "Observe %s", gameData->player1);
			item.wID = MENUITEM_Player_Observe;
			item.dwTypeData =  buf;
			item.cch = (UINT)strlen(buf)+1;
			item.fState = MFS_ENABLED;
			InsertMenuItemA(menu, 80, FALSE, &item);
		}

		sprintf_s(buf, sizeof(buf), "Follow %s", gameData->player1);
		item.wID = MENUITEM_Player_Follow;
		item.dwTypeData =  buf;
		item.cch = (UINT)strlen(buf)+1;
		item.fState = MFS_ENABLED;
		InsertMenuItemA(menu, 80, FALSE, &item);

		sprintf_s(buf, sizeof(buf), "Finger %s", gameData->player1);
		item.wID = MENUITEM_Player_Finger;
		item.dwTypeData =  buf;
		item.cch = (UINT)strlen(buf)+1;
		item.fState = MFS_ENABLED;
		InsertMenuItemA(menu, 80, FALSE, &item);

		sprintf_s(buf, sizeof(buf), "Add %s to notify list", gameData->player1);
		item.wID = MENUITEM_Player_Add_To_Notify;
		item.dwTypeData =  buf;
		item.cch = (UINT)strlen(buf)+1;
		item.fState = MFS_ENABLED;
		InsertMenuItemA(menu, 80, FALSE, &item);

		sprintf_s(buf, sizeof(buf), "Remove %s from notify list", gameData->player1);
		item.wID = MENUITEM_Player_Remove_From_Notify;
		item.dwTypeData =  buf;
		item.cch = (UINT)strlen(buf)+1;
		item.fState = MFS_ENABLED;
		InsertMenuItemA(menu, 80, FALSE, &item);

		item.fMask = MIIM_TYPE | MIIM_STATE;
		item.fType = MFT_SEPARATOR;
		item.fState = MFS_ENABLED;
		InsertMenuItemA(menu, 80, FALSE, &item);
	}

	if (flags & MENUTYPE_Team1)
	{
		MENUITEMINFOA item;
		item.cbSize = sizeof(item);
		item.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
		item.fType = MFT_STRING;

		sprintf_s(buf, sizeof(buf), "Add \"%s\" to favorites", gameData->team1);
		item.wID = MENUITEM_Add_Team_To_Favs;
		item.dwTypeData =  buf;
		item.cch = (UINT)strlen(buf)+1;
		item.fState = Settings::Get()->IsTeamFavorite(gameData->team1) ? MFS_CHECKED : MFS_UNCHECKED;
		InsertMenuItemA(menu, 90, FALSE, &item);

		sprintf_s(buf, sizeof(buf), "List team \"%s\"", gameData->team1);
		item.wID = MENUITEM_List_a_Team;
		item.dwTypeData =  buf;
		item.cch = (UINT)strlen(buf)+1;
		item.fState = MFS_ENABLED;
		InsertMenuItemA(menu, 90, FALSE, &item);

		item.fMask = MIIM_TYPE | MIIM_STATE;
		item.fType = MFT_SEPARATOR;
		item.fState = MFS_ENABLED;
		InsertMenuItemA(menu, 90, FALSE, &item);
	}

	if (flags & MENUTYPE_Team2)
	{
		MENUITEMINFOA item;
		item.cbSize = sizeof(item);
		item.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
		item.fType = MFT_STRING;

		sprintf_s(buf, sizeof(buf), "Add \"%s\" to favorites", gameData->team2);
		item.wID = MENUITEM_Add_Team2_To_Favs;
		item.dwTypeData =  buf;
		item.cch = (UINT)strlen(buf)+1;
		item.fState = Settings::Get()->IsTeamFavorite(gameData->team2) ? MFS_CHECKED : MFS_UNCHECKED;
		InsertMenuItemA(menu, 90, FALSE, &item);

		sprintf_s(buf, sizeof(buf), "List team \"%s\"", gameData->team2);
		item.wID = MENUITEM_List_a_Team2;
		item.dwTypeData =  buf;
		item.cch = (UINT)strlen(buf)+1;
		item.fState = MFS_ENABLED;
		InsertMenuItemA(menu, 90, FALSE, &item);

		item.fMask = MIIM_TYPE | MIIM_STATE;
		item.fType = MFT_SEPARATOR;
		item.fState = MFS_ENABLED;
		InsertMenuItemA(menu, 90, FALSE, &item);
	}

	item.wID = MENUITEM_Plugin_UpdateData;
	item.dwTypeData =  L"Update data";
	item.cch = (UINT)wcslen(item.dwTypeData)+1;
	item.fState = Dataset::Get().is_data_valid() ? MFS_ENABLED : MFS_GRAYED;
	InsertMenuItemW(menu, 100, FALSE, &item);

	item.wID = MENUITEM_Plugin_Settings;
	item.dwTypeData =  L"Plugin settings...";
	item.cch = (UINT)wcslen(item.dwTypeData)+1;
	item.fState = MFS_ENABLED;
	InsertMenuItemW(menu, 100, FALSE, &item);

	GetCursorPos(&point);
	selectedItem = (int) TrackPopupMenuEx(menu, TPM_LEFTALIGN| TPM_TOPALIGN | TPM_RETURNCMD, point.x, point.y, hwnd, 0);

	switch(selectedItem)
	{
	case MENUITEM_Plugin_UpdateData:
		Engine_RequestData();
		break;
	case MENUITEM_Plugin_Settings:
		Settings::Get()->DoDialog();
		break;
	case MENUITEM_Add_Team_To_Favs:
    Settings::Get()->ChangeTeamFavoriteStatus(gameData->team1, !Settings::Get()->IsTeamFavorite(gameData->team1));
		Tabset_Event(TABEVENT_CONFIG_CHANGED);
		break;
	case MENUITEM_List_a_Team:
		sprintf_s(buf, sizeof(buf), TEAMLEAGUE_CMD("=team %s"), gameData->team1);
		Engine_SendCommand(buf);
		break;
	case MENUITEM_Add_Team2_To_Favs:
    Settings::Get()->ChangeTeamFavoriteStatus(gameData->team2, !Settings::Get()->IsTeamFavorite(gameData->team2));
		Tabset_Event(TABEVENT_CONFIG_CHANGED);
		break;
	case MENUITEM_List_a_Team2:
		sprintf_s(buf, sizeof(buf), TEAMLEAGUE_CMD("=team %s"), gameData->team2);
		Engine_SendCommand(buf);
		break;
	case MENUITEM_Player_Follow:
		sprintf_s(buf, sizeof(buf), "follow %s", gameData->player1);
		Engine_SendCommand(buf);
		break;
	case MENUITEM_Player_Observe:
		sprintf_s(buf, sizeof(buf), "observe %s", gameData->player1);
		Engine_SendCommand(buf);
		break;
	case MENUITEM_Player_Finger:
		sprintf_s(buf, sizeof(buf), "finger %s", gameData->player1);
		Engine_SendCommand(buf);
		break;
	case MENUITEM_Player_Add_To_Notify:
		sprintf_s(buf, sizeof(buf), "+notify %s", gameData->player1);
		Engine_SendCommand(buf);
		break;
	case MENUITEM_Player_Remove_From_Notify:
		sprintf_s(buf, sizeof(buf), "-notify %s", gameData->player1);
		Engine_SendCommand(buf);
		break;
	case MENUITEM_Game_Start:
		sprintf_s(buf, sizeof(buf), TEAMLEAGUE_CMD(" play %d"), gameData->gameId);
		Engine_SendCommand(buf);
		break;
	case MENUITEM_Game_Examine:
		sprintf_s(buf, sizeof(buf), TEAMLEAGUE_CMD(" examine %d"), gameData->gameId);
		Engine_SendCommand(buf);
		break;
	case MENUITEM_Game_Observe:
		if (Settings::Get()->IsTeamFavorite(gameData->team2) && !Settings::Get()->IsTeamFavorite(gameData->team1)) {
			sprintf_s(buf, sizeof(buf), "observe %s", gameData->player2);
		}	else {
			sprintf_s(buf, sizeof(buf), "observe %s", gameData->player1);
		}
		Engine_SendCommand(buf);
		break;
	}
	
	DestroyMenu(menu);
	return STATUS_OK;
}
