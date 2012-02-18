#include "tab_common.h"
#include "dataset.h"
#include "settings.h"
#include <stdio.h>
#include <assert.h>

typedef struct  
{
	int Round;
	char team1[CONFIG_MAX_TEAM_TITLE_LENGTH];
	char team2[CONFIG_MAX_TEAM_TITLE_LENGTH];
	char player1[CONFIG_MAX_NICKNAME_LENGTH];
	char player2[CONFIG_MAX_NICKNAME_LENGTH];
	char secton[CONFIG_MAX_SECTION_TITLE_LENGTH];
	FILETIME scheduledTime;
	int gameId;
} Upcoming_Game_t;

typedef struct 
{
	Upcoming_Game_t games[CONFIG_MAX_GAMES_PER_ROUND];
	int gameCount;
	HWND hwnd;
} Tab_Upcoming_t;

static Tab_Upcoming_t *g_tab = 0;

static const char* DaysOfWeek[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const char* Months[] = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

typedef struct  
{
	int xOffset;
	int yOffset;

	int headerHeight;
	int rowHeight;

	int roundWidth;
	int sectionWidth;
	int playerWidth;
	int timeWidth;

	int totalWidth;

} TablePaintData_t;

static void Tab_Upcoming_CalcSizes(const RECT* rect, TablePaintData_t* sizes)
{
	int baseWidth;
	sizes->yOffset = 0;
	sizes->totalWidth = rect->right - rect->left - 2;

	if (sizes->totalWidth > CONFIG_TABS_UPCOMING_MAX_TABLEWIDTH)
	{
		sizes->totalWidth = CONFIG_TABS_UPCOMING_MAX_TABLEWIDTH;
	}

	baseWidth =  sizes->totalWidth / 12;
	sizes->totalWidth = baseWidth * 12;

	sizes->xOffset = ((rect->right - rect->left) - sizes->totalWidth) / 2;
	sizes->headerHeight = CONFIG_TABS_UPCOMING_HEADERHEIGHT;
	sizes->rowHeight = CONFIG_TABS_UPCOMING_GAMEHEIGHT;

	sizes->roundWidth = baseWidth;
	sizes->sectionWidth = baseWidth * 2;
	sizes->playerWidth = baseWidth * 3;
	sizes->timeWidth = baseWidth * 3;
}

static void Tab_Upcoming_UpdateScrollBar()
{
	TablePaintData_t sizes;
	RECT rect;
	int totalHeight = 0;
	SCROLLINFO scrollinfo;

	if (g_tab->gameCount <= 0)
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
	Tab_Upcoming_CalcSizes(&rect, &sizes);
	totalHeight = sizes.headerHeight + sizes.rowHeight*g_tab->gameCount;

	scrollinfo.cbSize = sizeof(scrollinfo);
	scrollinfo.fMask = /*SIF_DISABLENOSCROLL | */  SIF_PAGE | SIF_RANGE;
	scrollinfo.nMin = 0;
	scrollinfo.nMax = totalHeight;
	scrollinfo.nPage = rect.bottom - rect.top;

	SetScrollInfo(g_tab->hwnd, SB_VERT, &scrollinfo, TRUE);
}

static void SecondsToStr(int seconds, char* buf, size_t size)
{
	int absSeconds = seconds;
	int minutes;
	int hours;
	int days;

	if (seconds <= 0)
	{
		strcpy_s(buf, size, "in ");
		size -= strlen(buf);
		buf += strlen(buf);
		absSeconds = -seconds;
	}

	minutes = absSeconds / 60;
	hours = minutes / 60;
	days = hours / 24;
	hours %= 24;
	minutes %= 60;

	if (days > 1)
	{
		sprintf_s(buf, size, "%d days ", days);
		size -= strlen(buf);
		buf += strlen(buf);
	}
	else if (days)
	{
		strcpy_s(buf, size, "1 day ");
		size -= strlen(buf);
		buf += strlen(buf);
	}

	if (hours > 1)
	{
		sprintf_s(buf, size, "%d hrs ", hours);
		size -= strlen(buf);
		buf += strlen(buf);

	}
	else if (hours)
	{
		strcpy_s(buf, size, "1 hr ");
		size -= strlen(buf);
		buf += strlen(buf);
	}

	if (minutes > 1)
	{
		sprintf_s(buf, size, "%d mins ", minutes);
		size -= strlen(buf);
		buf += strlen(buf);

	}
	else if (minutes)
	{
		strcpy_s(buf, size, "1 min ");
		size -= strlen(buf);
		buf += strlen(buf);
	}

	if (absSeconds < 60)
	{
		strcpy_s(buf, size, "less than a minute ");
		size -= strlen(buf);
		buf += strlen(buf);
	}

	if (seconds > 0)
	{
		strcpy_s(buf, size, "ago ");
		size -= strlen(buf);
		buf += strlen(buf);
	}

	--buf;
	*buf = '\0';
}

static void Tab_Upcoming_PaintDC(HDC hdc, int offset, RECT clientRect)
{
	RECT rect;
	TablePaintData_t sizes;
	SYSTEMTIME time;
	FILETIME now;
	int i;
	ULARGE_INTEGER now1;

	SelectObject(hdc, Tabset_GetGdiCollection()->normalFont);
	SetBkMode(hdc, TRANSPARENT);

	if (g_tab->gameCount <= 0)
	{
		DrawText(hdc, L"\n\n\nThere're no scheduled games at the moment.", -1, &clientRect, DT_CENTER);
		return;
	}

	Tab_Upcoming_CalcSizes(&clientRect, &sizes);
	clientRect.top -= offset;
	rect = clientRect;

	//////////////////////////////////////////////////////////////////////////
	/// Drawing Header
	//////////////////////////////////////////////////////////////////////////

	rect.top    = clientRect.top + sizes.yOffset;
	rect.left   = clientRect.left + sizes.xOffset;
	rect.bottom = rect.top + sizes.headerHeight;

	rect.right = rect.left + sizes.roundWidth;
	DrawText(hdc,  L"Round", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	rect.left   = rect.right;

	rect.right = rect.left + sizes.sectionWidth;
	DrawText(hdc,  L"Section", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	rect.left   = rect.right;

	rect.right = rect.left + sizes.playerWidth;
	DrawText(hdc,  L"White", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	rect.left   = rect.right;

	rect.right = rect.left + sizes.playerWidth;
	DrawText(hdc,  L"Black", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	rect.left   = rect.right;

	rect.right = rect.left + sizes.timeWidth;
	switch(Settings_GetSettings()->gameTime)
	{
	case SETTINGS_TIME_LOCAL:
		DrawText(hdc,  L"Time (local)", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		break;
	case SETTINGS_TIME_RELATIVE:
		DrawText(hdc,  L"Time", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		break;
	case SETTINGS_TIME_FICS:
		DrawText(hdc,  L"Time (FICS)", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		break;
	}
	rect.left   = rect.right;

	rect.top = rect.bottom;

	//////////////////////////////////////////////////////////////////////////
	// Drawing data
	//////////////////////////////////////////////////////////////////////////

	GetSystemTime(&time);
	SystemTimeToFileTime(&time, &now);
	now1.HighPart = now.dwHighDateTime;
	now1.LowPart = now.dwLowDateTime;

	for(i = 0; i < g_tab->gameCount; ++i)
	{
		const Upcoming_Game_t *game = &g_tab->games[i];
		ULARGE_INTEGER gameTime1;
		int difference;
		signed long long lldif;

		gameTime1.HighPart = game->scheduledTime.dwHighDateTime;
		gameTime1.LowPart = game->scheduledTime.dwLowDateTime;

		lldif = now1.QuadPart - gameTime1.QuadPart;
		difference = (int)(lldif / 10000000);

		rect.left = clientRect.left + sizes.xOffset;
		rect.right = rect.left + sizes.totalWidth;
		rect.bottom = rect.top + sizes.rowHeight;

		if (difference >= CONFIG_TABS_UPCOMING_RED)
		{
			Tabset_SelectStyle(hdc, &Tabset_GetGdiCollection()->badCell, 0);
		}
		else if (difference >= CONFIG_TABS_UPCOMING_YELLOW)
		{
			Tabset_SelectStyle(hdc, &Tabset_GetGdiCollection()->pourCell, 0);
		}
		else if (difference >= CONFIG_TABS_UPCOMING_GREEN)
		{
			Tabset_SelectStyle(hdc, &Tabset_GetGdiCollection()->goodCell, 0);
		}
		else
		{
			Tabset_SelectStyle(hdc, &Tabset_GetGdiCollection()->neutralCell, 0);
		}

		Rectangle(hdc, rect.left, rect.top, rect.right+1, rect.bottom+1);
		
		// Drawing actual data

		{
			int top = rect.top;
			char buf[256];
			char relTime[256];
			SecondsToStr(difference, relTime, sizeof(relTime));

			rect.right = rect.left + sizes.roundWidth;
			sprintf_s(buf, sizeof(buf), "%d", game->Round);
			DrawTextA(hdc, buf, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE /*| DT_END_ELLIPSIS*/);
			rect.left = rect.right;

			rect.right = rect.left + sizes.sectionWidth;
			sprintf_s(buf, sizeof(buf), "%s", game->secton);
			DrawTextA(hdc, buf, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
			rect.left = rect.right;

			rect.right = rect.left + sizes.playerWidth;
			rect.bottom = rect.top + CONFIG_TABS_UPCOMING_LINEHEIGHT;
			sprintf_s(buf, sizeof(buf), "%s", game->player1);
			DrawTextA(hdc, buf, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
			rect.top = rect.bottom;
			rect.bottom = rect.top + CONFIG_TABS_UPCOMING_LINEHEIGHT;
			sprintf_s(buf, sizeof(buf), "(%s)", game->team1);
			DrawTextA(hdc, buf, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
			rect.left = rect.right;

			rect.top = top;
			rect.right = rect.left + sizes.playerWidth;
			rect.bottom = rect.top + CONFIG_TABS_UPCOMING_LINEHEIGHT;
			sprintf_s(buf, sizeof(buf), "%s", game->player2);
			DrawTextA(hdc, buf, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
			rect.top = rect.bottom;
			rect.bottom = rect.top + CONFIG_TABS_UPCOMING_LINEHEIGHT;
			sprintf_s(buf, sizeof(buf), "(%s)", game->team2);
			DrawTextA(hdc, buf, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
			rect.left = rect.right;

			rect.top = top;
			rect.right = rect.left + sizes.timeWidth;
			if (Settings_GetSettings()->gameTime == SETTINGS_TIME_RELATIVE)
			{
				DrawTextA(hdc, relTime, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
			}
			else
			{
				SYSTEMTIME systime;
				SYSTEMTIME localTime;
				TIME_ZONE_INFORMATION FICStzInfo =	{	480,
														L"Pacific Standard Time",
														{0, 11, 0, 1, 2, 0, 0, 0},
														0,
														L"Pacific Daylight Time",
														{0, 3, 0, 2, 2, 0, 0, 0},
														-60	};

				FileTimeToSystemTime(&game->scheduledTime, &systime);

				if (Settings_GetSettings()->gameTime == SETTINGS_TIME_LOCAL)
				{
					SystemTimeToTzSpecificLocalTime(0, &systime, &localTime);
				}
				else
				{
					SystemTimeToTzSpecificLocalTime(&FICStzInfo, &systime, &localTime);
				}
				rect.bottom = rect.top + CONFIG_TABS_UPCOMING_LINEHEIGHT;
				sprintf_s(buf, sizeof(buf), "%s, %s %d, %02d:%02d", DaysOfWeek[localTime.wDayOfWeek], Months[localTime.wMonth],
												localTime.wDay, localTime.wHour, localTime.wMinute);
				DrawTextA(hdc, buf, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
				rect.top = rect.bottom;
				rect.bottom = rect.top + CONFIG_TABS_UPCOMING_LINEHEIGHT;
				sprintf_s(buf, sizeof(buf), "(%s)", relTime);
				DrawTextA(hdc, buf, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
				rect.top = top;
			}

			rect.left = rect.right;
		}

		//////////////////////////////////////////////////////////////////////////
		// Drawing vertical lines
		//////////////////////////////////////////////////////////////////////////

		{
			int top = rect.top;
			int bottom = rect.bottom;
			int left = clientRect.left + sizes.xOffset;

			left += sizes.roundWidth;
			MoveToEx(hdc, left, top, 0);
			LineTo(hdc, left, bottom);

			left += sizes.sectionWidth;
			MoveToEx(hdc, left, top, 0);
			LineTo(hdc, left, bottom);

			left += sizes.playerWidth;
			MoveToEx(hdc, left, top, 0);
			LineTo(hdc, left, bottom);

			left += sizes.playerWidth;
			MoveToEx(hdc, left, top, 0);
			LineTo(hdc, left, bottom);
		}

		rect.top = rect.bottom;
	}

}

static int Tab_Upcoming_CmpTime(const void* context, const Upcoming_Game_t* team1idx, const Upcoming_Game_t* team2idx)
{
	(void)context;
	if (team2idx->scheduledTime.dwHighDateTime > team1idx->scheduledTime.dwHighDateTime)
		return -1;
	if (team2idx->scheduledTime.dwHighDateTime < team1idx->scheduledTime.dwHighDateTime)
		return 1;

	if (team2idx->scheduledTime.dwLowDateTime > team1idx->scheduledTime.dwLowDateTime)
		return -1;
	if (team2idx->scheduledTime.dwLowDateTime < team1idx->scheduledTime.dwLowDateTime)
		return 1;

	return 0;
}

static Status_t Tab_Upcoming_UpdateCheckBox()
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

static Status_t Tab_Upcoming_UpdateData()
{
	const Dataset_t* data = Dataset_GetDataset();
	BOOL onlyFavorites = Settings_GetSettings()->showFavoritesOnly;
	int i;

	g_tab->gameCount = 0;

	if (!data->isDataValid)
	{
		g_tab->gameCount = 0;
		return STATUS_OK;
	}

	for(i = 0; i < data->teamGameCount; ++i)
	{
		const TeamGame_t *teamGame = &data->teamGames[i];
		BOOL processThisTeamGame = TRUE;

		BOOL team1IsFavorite = FALSE;
		BOOL team2IsFavorite = TRUE;
		int j;

		if (onlyFavorites)
		{
			team1IsFavorite = Settings_IsTeamFavorite(teamGame->team1);
			team2IsFavorite = Settings_IsTeamFavorite(teamGame->team2);
			processThisTeamGame = team1IsFavorite || team2IsFavorite;
		}

		if (!processThisTeamGame)
		{
			continue;
		}

		for(j = 0; j < teamGame->gameCount; ++j)
		{
			const Game_t* game = &teamGame->teamGames[j];
			if (game->status != GAMESTATUS_Pending)
			{
				continue;
			}

			if (g_tab->gameCount >= CONFIG_MAX_GAMES_PER_ROUND)
			{
				g_tab->gameCount = 0;
				return STATUS_ERR_NOT_ENOUGH_ROOM;
			}

			{
				Upcoming_Game_t* newEntry = &g_tab->games[g_tab->gameCount++];
				newEntry->gameId = game->gameId;
				newEntry->Round = teamGame->round;
				newEntry->scheduledTime = game->scheduledTime;

				strcpy_s(newEntry->secton, sizeof(newEntry->secton), teamGame->secton);

				if (game->isPlayer1White)
				{
					strcpy_s(newEntry->team1, sizeof(newEntry->team1), teamGame->team1);
					strcpy_s(newEntry->team2, sizeof(newEntry->team2), teamGame->team2);
					strcpy_s(newEntry->player1, sizeof(newEntry->player1), game->player1);
					strcpy_s(newEntry->player2, sizeof(newEntry->player1), game->player2);
				}
				else
				{
					strcpy_s(newEntry->team1, sizeof(newEntry->team1), teamGame->team2);
					strcpy_s(newEntry->team2, sizeof(newEntry->team2), teamGame->team1);
					strcpy_s(newEntry->player1, sizeof(newEntry->player1), game->player2);
					strcpy_s(newEntry->player2, sizeof(newEntry->player1), game->player1);
				}
			}
		}
	}

	qsort_s(g_tab->games, g_tab->gameCount, sizeof(g_tab->games[0]), (int (__cdecl *)(void *,const void *,const void *))Tab_Upcoming_CmpTime, 0);
	return STATUS_OK;
}

static Tab_HitPoint_t Tab_Upcoming_HitTest(Tab_Common_SimpleGameData_t *gameData, POINT point)
{
	TablePaintData_t sizes;
	RECT rect;
	int index;

	if (g_tab->gameCount == 0)
	{
		return HITPOINT_Milk;
	}

	GetClientRect(g_tab->hwnd, &rect);
	Tab_Upcoming_CalcSizes(&rect, &sizes);
	point.x -= rect.left + sizes.xOffset;
	point.y -= rect.top + sizes.yOffset + CONFIG_TABS_COMMON_TOP_MARGIN + sizes.headerHeight;
	
	if (point.x < 0 || point.x >= sizes.totalWidth)
	{
		return HITPOINT_Milk;
	}

	if (point.y < 0 || point.y >= sizes.rowHeight*g_tab->gameCount)
	{
		return HITPOINT_Milk;
	}

	index = point.y / sizes.rowHeight;

	if (index >= g_tab->gameCount)
	{
		assert(FALSE);
		return HITPOINT_Milk;
	}

	gameData->gameId = g_tab->games[index].gameId;
	strcpy_s(gameData->player1, sizeof(gameData->player1), g_tab->games[index].player1);
	strcpy_s(gameData->player2, sizeof(gameData->player2), g_tab->games[index].player2);
	strcpy_s(gameData->team1, sizeof(gameData->team1), g_tab->games[index].team1);
	strcpy_s(gameData->team2, sizeof(gameData->team2), g_tab->games[index].team2);

	if (point.x < sizes.roundWidth)
	{
		return HITPOINT_Round;
	}
	point.x -= sizes.roundWidth;

	if (point.x < sizes.sectionWidth)
	{
		return HITPOINT_Section;
	}
	point.x -= sizes.sectionWidth;

	if (point.x < sizes.playerWidth)
	{
		return HITPOINT_Player1;
	}
	point.x -= sizes.playerWidth;

	if (point.x < sizes.playerWidth)
	{
		SwapData(gameData->player1, gameData->player2, CONFIG_MAX_NICKNAME_LENGTH);
		SwapData(gameData->team1, gameData->team2, CONFIG_MAX_TEAM_TITLE_LENGTH);
		return HITPOINT_Player2;
	}
	point.x -= sizes.playerWidth;

	if (point.x < sizes.timeWidth)
	{
		return HITPOINT_Time;
	}

	assert(FALSE);
	return HITPOINT_Milk;
}

static LRESULT CALLBACK Tab_Upcoming_WndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
			Tab_Common_DoPaint(hwnd, Tab_Upcoming_PaintDC);
			return 0;
		}
	case WM_CREATE:
		{
			g_tab->hwnd = hwnd;
			Tab_Upcoming_UpdateCheckBox();
			Tab_Upcoming_UpdateScrollBar();
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
			Tab_Upcoming_UpdateScrollBar();
			Tab_Upcoming_UpdateCheckBox();
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
			point.x = x;
			point.y = y + Tab_Common_GetScrollOffset(hwnd);
			hitPoint = Tab_Upcoming_HitTest(&gameData, point);
			switch (hitPoint)
			{
			case HITPOINT_Player1:
			case HITPOINT_Player2:
				Tab_Common_DoMenu(g_tab->hwnd, MENUTYPE_Team | MENUTYPE_Player | MENUTYPE_Plugin| MENUTYPE_GameStartable, &gameData);
				break;
			case HITPOINT_Round:
			case HITPOINT_Section:
			case HITPOINT_Time:
				Tab_Common_DoMenu(g_tab->hwnd, MENUTYPE_Plugin | MENUTYPE_GameStartable, &gameData);
				break;
			default:
				Tab_Common_DoMenu(g_tab->hwnd, MENUTYPE_Plugin, &gameData);
				break;
			}
			return 0;
		}

	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

static Status_t Tab_Upcoming_Init()
{
	WNDCLASS wndclass;  
 	wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS; 
 	wndclass.lpfnWndProc = Tab_Upcoming_WndProc;
 	wndclass.cbClsExtra = 0;
 	wndclass.cbWndExtra = 0;
 	wndclass.hInstance = Tabset_GetHInstance();
 	wndclass.hIcon = NULL;
 	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW); 
 	wndclass.hbrBackground = CreateSolidBrush(Tabset_GetGdiCollection()->bgColor); // TODO fix it
 	wndclass.lpszMenuName =  NULL;
 	wndclass.lpszClassName = L"moomoo_UpcomingTab";
 	if (!RegisterClass(&wndclass))
 	{
 		return STATUS_ERR_WINDOW_CANNOT_REGISTER;
 	}
 
 	return STATUS_OK;
}
 
static Status_t Tab_Upcoming_Event(TabEvent_t event)
{
 	switch(event)
 	{
	case TABEVENT_CONFIG_CHANGED:
 	case TABEVENT_DATA_UPDATED:
 		{
 			Status_t status = Tab_Upcoming_UpdateData();
 			
 			if (status != STATUS_OK)
 			{
 				return status;
 			}

			Tab_Upcoming_UpdateScrollBar();
			Tab_Upcoming_UpdateCheckBox();
        }

	case TABEVENT_MINUTE_CHANGED: // Fall though
        {
			{
				RECT rect;
 				GetClientRect(g_tab->hwnd, &rect); 
 				InvalidateRect(g_tab->hwnd, &rect, FALSE);
			}
 			return STATUS_OK;
 		}
 
 	case TABEVENT_INIT:
 		return Tab_Upcoming_Init();
 	}
 	return STATUS_OK_NOT_PROCESSED;
}
 
static Status_t Tab_Upcoming_Create(HWND parent, HWND* child)
{
 	g_tab = (Tab_Upcoming_t *)malloc(sizeof(Tab_Upcoming_t));
 	if (!g_tab)
 	{
 		return STATUS_ERR_NOT_ENOUGH_ROOM;
 	}
 
 	Tab_Upcoming_UpdateData();
 	g_tab->hwnd = CreateWindow(L"moomoo_UpcomingTab", L"", WS_CHILD|WS_VISIBLE|WS_VSCROLL, 0, 0, 0, 0, parent, 0, Tabset_GetHInstance(), 0);
 	if (!g_tab->hwnd)
 	{
 		return STATUS_ERR_WINDOW_CANNOT_CREATE;
 	}
 	*child = g_tab->hwnd;
 	return STATUS_OK;
}
 
static Status_t Tab_Upcoming_GetTabTitle(char* title, int length)
{
	int favCount = 0;
	int allCount = 0;
	Status_t status = Tab_Common_PopulateStdData(TRUE, NULL, &favCount, NOT_VALID31, GAMESTATUS_Pending);

	if (status == STATUS_OK)
	{
		status = Tab_Common_PopulateStdData(FALSE, NULL, &allCount, NOT_VALID31, GAMESTATUS_Pending);
	}

	if (status == STATUS_OK)
	{
		sprintf_s(title, length, "Upcoming (%d/%d)", favCount, allCount);
	}
	else
	{
		sprintf_s(title, length, "Upcoming");
	}

	return STATUS_OK;
}
 
Status_t Tab_Upcoming_GetHandlers(TabHandlers_t* handlers)
{
 	handlers->onCreate = Tab_Upcoming_Create;
 	handlers->onEvent = Tab_Upcoming_Event;
	handlers->onGetTabTitle = Tab_Upcoming_GetTabTitle;
 	return STATUS_OK;
}