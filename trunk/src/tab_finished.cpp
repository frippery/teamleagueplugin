#define _WIN32_WINNT 0x0501
#include <windows.h>
#include "status.h"
#include "tab_common.h"
#include "dataset.h"
#include "settings.h"
#include <CommCtrl.h>
#include <stdio.h>

typedef struct
{
	int teamGameCount;
	TeamGame_t teamGame[CONFIG_MAX_TEAMGAMES_PER_ROUND];
} Round_t;

typedef struct 
{
	HWND hwnd;
	int roundCount;
	int curRound;
	HWND dropDownHwnd;
	Round_t round[CONFIX_MAX_ROUNDS_PER_SEASON];
} Tab_Finished_t;

static Tab_Finished_t *g_tab = 0;

typedef struct  
{
	int xOffset;
	int yOffset;

	int teamGameWidth;
	int team1Width;
	int team1TextWidth;
	int team1PawnWidth;
	int team2Width;
	int outcomeWidth;

	int totalWidth;
	
	int headerHeight;
	int teamGameHeight[CONFIG_MAX_GAMES_PER_TEAMGAME];
	int gameHeight[CONFIG_MAX_GAMES_PER_TEAMGAME];
	int teamGameStep[CONFIG_MAX_GAMES_PER_TEAMGAME];
} TablePaintData_t;

static void Tab_Finished_CalcSizes(const RECT* rect, TablePaintData_t* sizes)
{
	int widthTogo;
	int i;
	sizes->headerHeight = CONFIG_TABS_FINISHED_HEADERHEIGHT;

	sizes->teamGameWidth = (rect->right - rect->left)-2;
	if (sizes->teamGameWidth > CONFIG_TABS_FINISHED_MAX_TABLEWIDTH)
	{
		sizes->teamGameWidth = CONFIG_TABS_FINISHED_MAX_TABLEWIDTH;
	}
	sizes->teamGameWidth /= 3;
	sizes->xOffset = ((rect->right - rect->left)-(sizes->teamGameWidth*3))/2;
	sizes->yOffset = 0;

	widthTogo = sizes->teamGameWidth*2;
	sizes->team2Width = (widthTogo-CONFIG_TABS_FINISHED_PAWNWIDTH)/5*2;
	sizes->team1PawnWidth = CONFIG_TABS_FINISHED_PAWNWIDTH;
	sizes->team1TextWidth = sizes->team2Width;
	sizes->team1Width = sizes->team2Width+CONFIG_TABS_FINISHED_PAWNWIDTH;
	sizes->outcomeWidth = widthTogo - sizes->team2Width - sizes->team1Width;
	sizes->totalWidth = sizes->team1Width + sizes->team2Width + sizes->outcomeWidth + sizes->teamGameWidth;

	for(i = 0; i < CONFIG_MAX_GAMES_PER_TEAMGAME; ++i)
	{
		int heigth = CONFIG_TABS_FINISHED_MIN_GAMEHEIGHT;
		if (heigth * (i+1) < CONFIG_TABS_FINISHED_MIN_TEAMGAMEHEIGHT)
		{
			heigth = (CONFIG_TABS_FINISHED_MIN_TEAMGAMEHEIGHT+i) / (i+1);
		}
		sizes->teamGameHeight[i] = heigth*(i+1);
		sizes->gameHeight[i] = heigth;
		sizes->teamGameStep[i] = sizes->teamGameHeight[i] + CONFIG_TABS_FINISHED_INTERTEAMGAME_SPACE;
	}
}

static void Tab_Finished_UpdateScrollBar()
{
	TablePaintData_t sizes;
	RECT rect;
	int totalHeight = 0;
	const Round_t* round;
	int i;
	SCROLLINFO scrollinfo;

	if (g_tab->curRound < 0 || g_tab->curRound >= g_tab->roundCount)
	{
		scrollinfo.cbSize = sizeof(scrollinfo);
		scrollinfo.fMask = /*SIF_DISABLENOSCROLL | */  SIF_PAGE | SIF_RANGE;
		scrollinfo.nMin = 0;
		scrollinfo.nMax = 0;
		scrollinfo.nPage = 1;
		SetScrollInfo(g_tab->hwnd, SB_VERT, &scrollinfo, TRUE);
		return;
	}

	GetClientRect(g_tab->hwnd, &rect);
	rect.top += CONFIG_TABS_COMMON_TOP_MARGIN;
	Tab_Finished_CalcSizes(&rect, &sizes);
	totalHeight += sizes.headerHeight;
	round = &g_tab->round[g_tab->curRound];

	for (i = 0; i < round->teamGameCount; ++i)
	{
		if (round->teamGame[i].gameCount == 0)
			continue;
		totalHeight += sizes.teamGameStep[round->teamGame[i].gameCount-1];
	}

	scrollinfo.cbSize = sizeof(scrollinfo);
	scrollinfo.fMask = /*SIF_DISABLENOSCROLL | */  SIF_PAGE | SIF_RANGE;
	scrollinfo.nMin = 0;
	scrollinfo.nMax = totalHeight;
	scrollinfo.nPage = rect.bottom - rect.top;

	SetScrollInfo(g_tab->hwnd, SB_VERT, &scrollinfo, TRUE);
}

static void Tab_Finished_DrawTeamGame(HDC hdc, RECT rect, const TablePaintData_t* sizes, const TeamGame_t* teamGame)
{
	RECT myRect = rect;
	int i;
	TabSet_ColorStyle_t oldStyle;

	Tabset_SelectStyle(hdc, Tab_Common_GetTeamGametStyle(teamGame), &oldStyle);

	myRect.right = myRect.left + sizes->teamGameWidth;
	myRect.bottom = myRect.top + sizes->teamGameHeight[teamGame->gameCount-1];

	Rectangle(hdc, myRect.left, myRect.top, myRect.right+1, myRect.bottom+1);
	
	{
		char buf[256];
		RECT textRect = myRect;
		textRect.top += (textRect.bottom - textRect.top - CONFIG_TABS_FINISHED_MIN_TEAMGAMEHEIGHT) / 2;
		sprintf_s(buf, sizeof(buf), "%s: %s", teamGame->secton, teamGame->team1);
		DrawTextA(hdc, buf, -1, &textRect, DT_CENTER | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
		textRect.top += CONFIG_TABS_FINISHED_LINEHEIGHT;
		sprintf_s(buf, sizeof(buf), "(vs %s)", teamGame->team2);
		DrawTextA(hdc, buf, -1, &textRect, DT_CENTER | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
	}

	Tabset_SelectStyle(hdc, &oldStyle, 0);

	myRect.left = myRect.right;
	myRect.right = myRect.left+sizes->totalWidth - sizes->teamGameWidth;

	for(i = 0; i < teamGame->gameCount; ++i)
	{
		const Game_t* game = &teamGame->teamGames[i];
		Tabset_SelectStyle(hdc, Tab_Common_GetTeamGameStyleByStatus(game->status), &oldStyle);
		myRect.bottom = myRect.top + sizes->gameHeight[teamGame->gameCount-1];

		Rectangle(hdc, myRect.left, myRect.top, myRect.right+1, myRect.bottom+1);

		{
			RECT textRect = myRect;
			
			textRect.right = textRect.left + CONFIG_TABS_FINISHED_PAWNWIDTH;
			DrawTextA(hdc, game->isPlayer1White ? "(W)" : "(B)", -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE );
			textRect.left = textRect.right;

			textRect.right = textRect.left + sizes->team1TextWidth;
			DrawTextA(hdc, game->player1, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
			textRect.left = textRect.right;

			textRect.right = textRect.left + sizes->outcomeWidth;
			DrawTextA(hdc, Dataset_OutcomeToStr(game->status), -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
			textRect.left = textRect.right;

			textRect.right = textRect.left + sizes->team2Width;
			DrawTextA(hdc, game->player2, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
			textRect.left = textRect.right;
		}

		MoveToEx(hdc, rect.left + sizes->teamGameWidth + sizes->team1Width, myRect.top, 0);
		LineTo(hdc, rect.left + sizes->teamGameWidth + sizes->team1Width, myRect.bottom);

		MoveToEx(hdc, rect.left + sizes->teamGameWidth + sizes->team1Width+sizes->outcomeWidth, myRect.top, 0);
		LineTo(hdc, rect.left + sizes->teamGameWidth + sizes->team1Width+sizes->outcomeWidth, myRect.bottom);

		myRect.top = myRect.bottom;
		Tabset_SelectStyle(hdc, &oldStyle, 0);
	}

}

static void Tab_Finished_PaintDC(HDC hdc, int offset, RECT clientRect)
{
	RECT rect;
	TablePaintData_t sizes;
	int i;
	const Round_t *round;


	if (g_tab->curRound < 0 || g_tab->curRound >= g_tab->roundCount)
	{
		return;
	}

	SelectObject(hdc, Tabset_GetGdiCollection()->normalFont);
	Tabset_SelectStyle(hdc, &Tabset_GetGdiCollection()->neutralCell, 0);
	SetBkMode(hdc, TRANSPARENT);

	round = &g_tab->round[g_tab->curRound];

	{
		for(i = 0; i < round->teamGameCount; ++i)
		{
			if (round->teamGame[i].gameCount != 0)
				break;
		}
		if (i == round->teamGameCount)
		{
			DrawText(hdc, L"\n\nNo finished games in this round yet.", -1, &clientRect, DT_CENTER);
			return;
		}
	}

	Tab_Finished_CalcSizes(&clientRect, &sizes);

	clientRect.top -= offset;

	//////////////////////////////////////////////////////////////////////////
	/// Drawing Header
	//////////////////////////////////////////////////////////////////////////

	rect.top    = clientRect.top + sizes.yOffset;
	rect.left   = clientRect.left + sizes.xOffset;
	rect.bottom = rect.top + sizes.headerHeight;

	rect.right = rect.left + sizes.teamGameWidth;
	DrawText(hdc,  L"Team", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	rect.left   = rect.right;

	rect.right = rect.left + sizes.team1Width;
	DrawText(hdc,  L"We", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE); // TODO add favorites icon here
	rect.left   = rect.right;

	rect.right = rect.left + sizes.outcomeWidth;
	DrawText(hdc,  L"Outcome", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	rect.left   = rect.right;

	rect.right = rect.left + sizes.team2Width;
	DrawText(hdc,  L"Opponents", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	rect.left   = rect.right;

	//////////////////////////////////////////////////////////////////////////
	// Drawing teamgames
	//////////////////////////////////////////////////////////////////////////

	rect.left   = clientRect.left + sizes.xOffset;
	rect.top    += sizes.headerHeight;
	rect.bottom = clientRect.bottom;

	for(i = 0; i < round->teamGameCount; ++i)
	{
		if (rect.bottom <= rect.top)
		{
			break;
		}
		if (round->teamGame[i].gameCount == 0)
		{
			continue;
		}

		Tab_Finished_DrawTeamGame(hdc, rect, &sizes, &round->teamGame[i]);
		rect.top += sizes.teamGameStep[round->teamGame[i].gameCount-1];
	}
}

static Tab_HitPoint_t Tab_Finished_HitTest(POINT point, Tab_Common_SimpleGameData_t *gameData)
{
	TablePaintData_t sizes;
	const Round_t *round;
	RECT rect;
	int i;

	GetClientRect(g_tab->hwnd, &rect);
	Tab_Finished_CalcSizes(&rect, &sizes);
	round = &g_tab->round[g_tab->curRound];

	rect.top += CONFIG_TABS_COMMON_TOP_MARGIN;
	rect.top += sizes.yOffset + sizes.headerHeight;
	rect.left += sizes.xOffset;

	if (point.x < rect.left || point.x >= rect.left + sizes.totalWidth )
	{
		return HITPOINT_Milk;
	}

	for(i = 0; i < round->teamGameCount; ++i)
	{
		const TeamGame_t* teamGame = &round->teamGame[i];

		if (teamGame->gameCount == 0)
		{
			continue;
		}

		if (point.y < rect.top)
		{
			return HITPOINT_Milk;
		}

		if (point.y >= rect.top + sizes.teamGameStep[teamGame->gameCount-1])
		{
			rect.top += sizes.teamGameStep[teamGame->gameCount-1];
			continue;
		}

		if (point.y >= rect.top + sizes.teamGameHeight[teamGame->gameCount-1])
		{
			return HITPOINT_Milk;
		}

		gameData->round = g_tab->curRound;
		strcpy_s(gameData->team1, sizeof(gameData->team1), teamGame->team1);
		strcpy_s(gameData->team2, sizeof(gameData->team2), teamGame->team2);

		rect.left += sizes.teamGameWidth;
		if (point.x < rect.left)
		{
			return HITPOINT_TeamGame;
		}

		{
			int gameNo = (point.y - rect.top) / sizes.gameHeight[teamGame->gameCount-1];
			const Game_t* game = &teamGame->teamGames[gameNo];

			strcpy_s(gameData->player1, sizeof(gameData->player1), game->player1);
			strcpy_s(gameData->player2, sizeof(gameData->player2), game->player2);
			gameData->gameId = game->gameId;

			rect.left += sizes.team1Width;
			if (point.x < rect.left)
			{
				return HITPOINT_Player1;
			}

			rect.left += sizes.outcomeWidth;
			if (point.x < rect.left)
			{
				return HITPOINT_Outcome;
			}

			rect.left += sizes.team2Width;
			if (point.x < rect.left)
			{
				SwapData(gameData->player1, gameData->player2, CONFIG_MAX_NICKNAME_LENGTH);
				SwapData(gameData->team1, gameData->team2, CONFIG_MAX_TEAM_TITLE_LENGTH);
				return HITPOINT_Player2;
			}

			return HITPOINT_Milk;
		}
	}

	return HITPOINT_Milk;
}

static Status_t Tab_Finished_UpdateDropDown()
{
	int i;

	Status_t status = Tab_Common_UpdateDropDown(&g_tab->dropDownHwnd, g_tab->hwnd, g_tab->roundCount != 0,
		                      CONFIG_TABS_COMMON_DROPDOWN_WIDTH, CONFIG_TABS_COMMON_DROPDOWN_HEIGHT);

	if (status != STATUS_OK || g_tab->roundCount == 0)
	{
		return status;
	}


	if (Settings_GetSettings()->curRoundsNum < g_tab->roundCount)
	{
		Settings_GetSettings()->curRoundsNum = g_tab->roundCount;
		Settings_GetSettings()->curFinishedDropDownRoundNum = g_tab->roundCount-1;
	}

	if (Settings_GetSettings()->curFinishedDropDownRoundNum < 0 || Settings_GetSettings()->curFinishedDropDownRoundNum >= g_tab->roundCount)
	{
		Settings_GetSettings()->curFinishedDropDownRoundNum = g_tab->roundCount-1;
	}

	for(i = 0; i < g_tab->roundCount; ++i)
	{
		char buf[128];
		sprintf_s(buf, sizeof(buf), "Round %d", i+1);
		SendMessageA(g_tab->dropDownHwnd, CB_ADDSTRING, 0, (LPARAM)buf);
	}

	SendMessage(g_tab->dropDownHwnd, CB_SETCURSEL, Settings_GetSettings()->curFinishedDropDownRoundNum, 0);
	g_tab->curRound = Settings_GetSettings()->curFinishedDropDownRoundNum;

	ComboBox_SetMinVisible(g_tab->dropDownHwnd, g_tab->roundCount);

	Tab_Finished_UpdateScrollBar();

	return STATUS_OK;
}

static Status_t Tab_Finished_UpdateCheckBox()
{
	RECT rect;
	GetClientRect(g_tab->hwnd, &rect);
	rect.left += CONFIG_TABS_COMMON_DROPDOWN_WIDTH + CONFIG_TABS_COMMON_CHECKBOXLEFTPAD;
	rect.right -= CONFIG_TABS_COMMON_DROPDOWN_WIDTH;
	if (rect.left > rect.right)
		rect.right = rect.left;

	rect.bottom = rect.top + CONFIG_TABS_COMMON_DROPDOWN_HEIGHT;

	return Tab_Common_UpdateCheckBox(g_tab->hwnd, &rect, Dataset_GetDataset()->isDataValid);
}

static Status_t Tab_Finished_UpdateData()
{
	const Dataset_t* data = Dataset_GetDataset();
	BOOL onlyFavorites = Settings_GetSettings()->showFavoritesOnly;

	int i;

	g_tab->roundCount = 0;

	if (!data->isDataValid)
	{
		g_tab->roundCount = 0;
		return STATUS_OK;
	}

	for(i = 0; i < data->teamGameCount; ++i)
	{
		const TeamGame_t *teamGame = &data->teamGames[i];
		BOOL processThisTeamGame = TRUE;

		BOOL team1IsFavorite = Settings_IsTeamFavorite(teamGame->team1);
		BOOL team2IsFavorite = Settings_IsTeamFavorite(teamGame->team2);

		if (onlyFavorites)
		{
			processThisTeamGame = team1IsFavorite || team2IsFavorite;
		}

		if (!processThisTeamGame)
		{
			continue;
		}

		if (teamGame->gameCount == 0)
		{
			continue;
		}

		if (teamGame->round > CONFIX_MAX_ROUNDS_PER_SEASON || teamGame->round < 1)
		{
			g_tab->roundCount = 0;
			return STATUS_ERR_NOT_ENOUGH_ROOM;
		}

		if (g_tab->roundCount < teamGame->round)
		{
			int i;
			for(i = g_tab->roundCount; i < teamGame->round; ++i)
			{
				g_tab->round[i].teamGameCount = 0;
			}
			g_tab->roundCount = teamGame->round;
		}

		{
			TeamGame_t *destination;
			Round_t *round = &g_tab->round[teamGame->round-1];
			int i;
			Game_t *srcGame;
			Game_t *dstGame;

			if (round->teamGameCount >= CONFIG_MAX_TEAMGAMES_PER_ROUND)
			{
				g_tab->roundCount = 0;
				return STATUS_ERR_NOT_ENOUGH_ROOM;
			}
			destination = &round->teamGame[round->teamGameCount++];

			memcpy_s(destination, sizeof(*destination), teamGame, sizeof(*teamGame));

			if (team2IsFavorite && !team1IsFavorite)
			{
				Dataset_ReverseTeamGame(destination);
			}

			srcGame = dstGame = destination->teamGames;

			for(i = 0; i < teamGame->gameCount; ++i)
			{
				if (Dataset_IsFinishedStatus(srcGame->status))
				{
					if (dstGame != srcGame)
					{
						memcpy_s(dstGame, sizeof(*dstGame), srcGame, sizeof(*srcGame));
					}
					++dstGame;
					++srcGame;
				}
				else
				{
					++srcGame;
					--destination->gameCount;
				}
			}

			if (destination->gameCount == 0)
			{
				--round->teamGameCount;
			}
		}
	}
	return STATUS_OK;
}

static LRESULT CALLBACK Tab_Finished_WndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT res = 0;
	Status_t status = Tab_Common_WndProc(hwnd, uMsg, wParam, lParam, &res);

	if (status == STATUS_OK_PROCESSED)
	{
		return res;
	}
	else if (status != STATUS_OK && status != STATUS_OK_NOT_PROCESSED)
	{
		DestroyWindow(hwnd); // TODO make something more sensefull here
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
			Tab_Common_DoPaint(hwnd, Tab_Finished_PaintDC);
			return 0;
		}
	case WM_COMMAND:
		{
			HWND sender = (HWND)(lParam);
			if (sender == g_tab->dropDownHwnd && HIWORD(wParam) == CBN_SELCHANGE)
			{
				int newSel = (int)SendMessage(sender, CB_GETCURSEL, 0, 0);
				RECT rect;
				g_tab->curRound = newSel;
				Settings_GetSettings()->curFinishedDropDownRoundNum = newSel;
				GetClientRect(hwnd, &rect);
				Tab_Finished_UpdateScrollBar();
				rect.top += CONFIG_TABS_COMMON_TOP_MARGIN;
				InvalidateRect(hwnd, &rect, FALSE);
				return 0;
			}
		}
		break;
	case WM_CREATE:
		{
			/* CREATESTRUCT *createStruct = (CREATESTRUCT*)lParam; */
			g_tab->hwnd = hwnd;
			Tab_Finished_UpdateCheckBox();
			Tab_Finished_UpdateDropDown();
			Tab_Finished_UpdateScrollBar();
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
			Tab_Finished_UpdateScrollBar();
			Tab_Finished_UpdateCheckBox();
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

			hitPoint = Tab_Finished_HitTest(point, &gameData);

			switch (hitPoint)
			{
			case HITPOINT_TeamGame:
				flags |= MENUTYPE_Team1 | MENUTYPE_Team2;
				break;
			case HITPOINT_Player1:
			case HITPOINT_Player2:
				flags |= MENUTYPE_Team | MENUTYPE_Player | MENUTYPE_Plugin | MENUTYPE_GameExaminable;
				break;
			case HITPOINT_Outcome:
				flags |= MENUTYPE_Plugin | MENUTYPE_GameExaminable;
			}
			flags |= MENUTYPE_Plugin;
			Tab_Common_DoMenu(g_tab->hwnd, flags, &gameData);
			return 0;
		}

	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

static Status_t Tab_Finished_Init()
{
	WNDCLASS wndclass;  
	wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS; 
	wndclass.lpfnWndProc = Tab_Finished_WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = Tabset_GetHInstance();
	wndclass.hIcon = NULL;
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW); 
	wndclass.hbrBackground = CreateSolidBrush(Tabset_GetGdiCollection()->bgColor); // TODO fix it
	wndclass.lpszMenuName =  NULL;
	wndclass.lpszClassName = L"moomoo_FinishedTab";
	if (!RegisterClass(&wndclass))
	{
		return STATUS_ERR_WINDOW_CANNOT_REGISTER;
	}

	return STATUS_OK;
}

static Status_t Tab_Finished_Event(TabEvent_t event)
{
	switch(event)
	{
	case TABEVENT_DATA_UPDATED:
	case TABEVENT_CONFIG_CHANGED:
		{
			Status_t status = Tab_Finished_UpdateData();
			RECT rect;
			if (status != STATUS_OK)
			{
				return status;
			}
			GetClientRect(g_tab->hwnd, &rect); 
			InvalidateRect(g_tab->hwnd, &rect, FALSE);
			Tab_Finished_UpdateCheckBox();
			Tab_Finished_UpdateDropDown();
			Tab_Finished_UpdateScrollBar();

			return STATUS_OK;
		}

	case TABEVENT_INIT:
		return Tab_Finished_Init();
	}
	return STATUS_OK_NOT_PROCESSED;
}

static Status_t Tab_Finished_Create(HWND parent, HWND* child)
{
	g_tab = (Tab_Finished_t *)malloc(sizeof(Tab_Finished_t));
	if (!g_tab)
	{
		return STATUS_ERR_NOT_ENOUGH_ROOM;
	}

	g_tab->dropDownHwnd = 0;
	
	Tab_Finished_UpdateData();
	g_tab->hwnd = CreateWindow(L"moomoo_FinishedTab", L"", WS_CHILD|WS_VISIBLE|WS_VSCROLL, 0, 0, 0, 0, parent, 0, Tabset_GetHInstance(), 0);
	if (!g_tab->hwnd)
	{
		return STATUS_ERR_WINDOW_CANNOT_CREATE;
	}
	*child = g_tab->hwnd;
	return STATUS_OK;
}

static Status_t Tab_Finished_GetTabTitle(char* title, int length)
{
	sprintf_s(title, length, "Finished");
	return STATUS_OK;
}

Status_t Tab_Finished_GetHandlers(TabHandlers_t* handlers)
{
	handlers->onCreate = Tab_Finished_Create;
	handlers->onEvent = Tab_Finished_Event;
	handlers->onGetTabTitle = Tab_Finished_GetTabTitle;
	return STATUS_OK;
}