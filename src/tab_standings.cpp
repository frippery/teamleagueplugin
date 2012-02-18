#define _WIN32_WINNT 0x501
#include <Windows.h>
#include "tabset.h"
#include "status.h"
#include "tab_common.h"
#include "dataset.h"
#include <CommCtrl.h>
#include <stdio.h>
#include "settings.h"


typedef struct
{
	int round;
	int GP2;
	int oppGP2;
	int gameToGo;
	BOOL played;
	const TeamGame_t* teamGame;
} TeamGameStat_t;

typedef struct
{
	TeamGameStat_t teamGames[CONFIG_MAX_TEAMPAIR_GAMES_PER_SECTION];
	int teamGameCount;
} TeamPairGameSet_t;


typedef struct
{
	char name[CONFIG_MAX_TEAM_TITLE_LENGTH];
	int MP2;
	int GP2;
	int gamesToPlay;
	TeamPairGameSet_t teamGameSet[CONFIG_MAX_TEAMS_PER_SECTION];
} Tab_Standings_Team_t;

typedef struct
{
	char title[CONFIG_MAX_SECTION_TITLE_LENGTH];
	int maxTeampairRounds;
	int teamCount;
	Tab_Standings_Team_t teams[CONFIG_MAX_TEAMS_PER_SECTION];
} Tab_Standings_Section_t;

typedef struct
{
	Tab_Standings_Section_t sections[CONFIG_MAX_SECTIONS_PER_SEASON];
	int sectionCount;
	HWND hwnd;
	HWND dropDownHwnd;
	HWND toolTipHwnd;
	int curSection;
} Tab_Standings_t;

static Tab_Standings_t *g_tab = 0;

static Status_t Tab_Standings_AddToolTip(RECT rect, int uid, TeamGameStat_t* teamGameStat)
{
	TOOLINFO ti;
	wchar_t toolTipStr_[1024];
	wchar_t *toolTipStr = &toolTipStr_[0];
	int toolTipLen = 1024;
	int i;
	int len;
	const char* outcomes[] =
	{
		"---", // GAMESTATUS_NotScheduled,
		"---", // GAMESTATUS_Pending,
		"(In progress!)", // GAMESTATUS_InProgress,
		"(ADJ)", // GAMESTATUS_Adjourned,

		"1-0", // GAMESTATUS_Win_1_0,
		"i-o", // GAMESTATUS_Win_i_o,
		"+:-", // GAMESTATUS_Win_plus_minus,
		"0-1", // GAMESTATUS_Lose_0_1,
		"o-i", // GAMESTATUS_Lose_o_i,
		"-:+", // GAMESTATUS_Lose_minus_plus,
		"1/2-1/2", // GAMESTATUS_Draw_12_12,
		"i/2-i/2", // GAMESTATUS_Draw_i2_i2,
		"o-o", // GAMESTATUS_DoubleForfeit_o_o,
		"-:-" // GAMESTATUS_DoubleForfeit_minus_minus,
	};

	if (teamGameStat->teamGame == NULL)
	{
		return STATUS_OK;
	}

	ti.cbSize = sizeof(TOOLINFO);
	ti.uFlags = TTF_SUBCLASS;
	ti.hwnd = g_tab->hwnd;
	ti.hinst = Tabset_GetHInstance();
	ti.rect = rect;
	ti.uId = uid;
	ti.lpszText = toolTipStr;

	len = swprintf_s(toolTipStr, toolTipLen, L"%S\tvs\t%S\r\nRound: %d", teamGameStat->teamGame->team1, teamGameStat->teamGame->team2, teamGameStat->round);
	toolTipLen -= len;
	toolTipStr += len;

	for(i=0; i < teamGameStat->teamGame->gameCount; ++i)
	{
		const Game_t* game = &teamGameStat->teamGame->teamGames[i];
		len = swprintf_s(toolTipStr, toolTipLen, L"\r\n%S %S \t%S\t %S %S", 
			game->isPlayer1White ? "(W)" : "(B)",
			game->player1,
			outcomes[game->status],
			game->player2,
			game->isPlayer1White ? "(B)" : "(W)");
		toolTipLen -= len;
		toolTipStr += len;
	}

	SendMessage(g_tab->toolTipHwnd, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO) &ti);	

	return STATUS_OK;
}

static Status_t Tab_Standings_UpdateDropDown()
{
	int i;
	int curSel;

	Status_t status = Tab_Common_UpdateDropDown(&g_tab->dropDownHwnd, g_tab->hwnd, g_tab->sectionCount != 0,
		CONFIG_TABS_COMMON_DROPDOWN_WIDTH, CONFIG_TABS_COMMON_DROPDOWN_HEIGHT);

	if (status != STATUS_OK || g_tab->sectionCount == 0)
	{
		return status;
	}

	curSel = 0;
	for(i = 0; i < g_tab->sectionCount; ++i)
	{
		SendMessageA(g_tab->dropDownHwnd, CB_ADDSTRING, 0, (LPARAM)g_tab->sections[i].title);
		if (strcmp(g_tab->sections[i].title, Settings_GetSettings()->curSectionTitle) == 0)
		{
			curSel = i;
		}
	}

	SendMessage(g_tab->dropDownHwnd, CB_SETCURSEL, curSel, 0);
	g_tab->curSection = curSel;

	ComboBox_SetMinVisible(g_tab->dropDownHwnd, g_tab->sectionCount);
	
	return STATUS_OK;
}

typedef struct
{
	int numWidth;
	int titleWidth;
	int MPwidth;
	int GPwidth;
	int gamesToGoWidth;
	int gameWidth;
	int totalWidth;

	int lineHeight;
	int roundHeight;
	int headerHeight;
	int totalHeight;

	int xOffset;
	int yOffset;
} TablePaintData_t;

static int Tab_Standings_GetMinMP2(const TeamGameStat_t * gam)
{
	if (gam->GP2 == 0)
		return 0;
	if (gam->GP2 > gam->oppGP2 + 2*gam->gameToGo)
		return 2;
	if (gam->GP2 == gam->oppGP2 + 2*gam->gameToGo)
		return 1;
	return 0;
}

static int Tab_Standings_GetMaxMP2(const TeamGameStat_t * gam)
{
	if (gam->GP2 + 2*gam->gameToGo > gam->oppGP2)
		return 2;
	if (gam->GP2 + 2*gam->gameToGo == gam->oppGP2)
		return 1;
	return 0;
}


static void Tab_Standings_DrawGame(HDC hdc, const RECT* rect, const TeamGameStat_t* game)
{
	TabSet_ColorStyle_t oldStyle;
	wchar_t buf[32];
	if (!game->played)
	{
		Tabset_SelectStyle(hdc, &Tabset_GetGdiCollection()->headerCell, &oldStyle);
		Rectangle(hdc, rect->left, rect->top, rect->right+1, rect->bottom+1);

		wsprintf(buf, L"Round %d", game->round);

		DrawText(hdc,  buf, -1, (RECT*)rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		Tabset_SelectStyle(hdc, &oldStyle, 0);
	}
	else
	{
		int minScore = Tab_Standings_GetMinMP2(game);
		int maxScore = Tab_Standings_GetMaxMP2(game);
		Tabset_SelectStyle(hdc, Tab_Common_GetGameStyle(minScore, maxScore), &oldStyle);

		Rectangle(hdc, rect->left, rect->top, rect->right+1, rect->bottom+1);
		swprintf_s(buf, 32, L"%3.1f (R%d)", (game->GP2/2.0), game->round);
		DrawText(hdc,  buf, -1, (RECT*)rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		Tabset_SelectStyle(hdc, &oldStyle, 0);
	}
}

static void Tab_Standings_CalcSizes(int rounds, int teams, const RECT* rect, TablePaintData_t* sizes)
{
	int commonHeight = rect->bottom - rect->top - 1;
	int commonWidth = rect->right - rect->left - 1;

	sizes->roundHeight = commonHeight/(teams*rounds+1);
	if (sizes->roundHeight > CONFIG_TABS_STANDINGS_MAX_ROUNDHEIGHT)
	{
		sizes->roundHeight = CONFIG_TABS_STANDINGS_MAX_ROUNDHEIGHT;
	}
	sizes->lineHeight = sizes->roundHeight*rounds;
	sizes->headerHeight = commonHeight - sizes->lineHeight*teams;
	if (sizes->headerHeight > CONFIG_TABS_STANDINGS_MAX_HEADERHEIGHT)
	{
		sizes->headerHeight = CONFIG_TABS_STANDINGS_MAX_HEADERHEIGHT;
	}

	sizes->gameWidth = commonWidth / (teams + CONFIG_TABS_STANDINGS_HEADER_WEIGHT);

	if (sizes->gameWidth > CONFIG_TABS_STANDINGS_MAX_GAMEWIDTH)
	{
		sizes->gameWidth = CONFIG_TABS_STANDINGS_MAX_GAMEWIDTH;
	}

	commonWidth -= sizes->gameWidth*teams;

	sizes->numWidth = commonWidth/10;
	if (sizes->numWidth > CONFIG_TABS_STANDINGS_MAX_GAMENUMWIDTH)
		sizes->numWidth = CONFIG_TABS_STANDINGS_MAX_GAMENUMWIDTH;

	sizes->gamesToGoWidth = sizes->numWidth;
	sizes->GPwidth = sizes->numWidth*3/2;
	sizes->MPwidth = sizes->numWidth*3/2;

	sizes->titleWidth = commonWidth - sizes->GPwidth - sizes->MPwidth - sizes->numWidth - sizes->gamesToGoWidth;

	if (sizes->titleWidth > CONFIG_TABS_STANDINGS_MAX_TEAMTITLEWIDTH)
		sizes->titleWidth = CONFIG_TABS_STANDINGS_MAX_TEAMTITLEWIDTH;

	sizes->totalWidth = sizes->numWidth + sizes->titleWidth + sizes->MPwidth + sizes->GPwidth + sizes->gamesToGoWidth + teams*sizes->gameWidth;
	sizes->totalHeight= sizes->headerHeight + teams*sizes->lineHeight;

	sizes->xOffset = (rect->right - rect->left - sizes->totalWidth) / 2;
	sizes->yOffset = (rect->bottom - rect->top - sizes->totalHeight) / 2;
}

static Status_t Tab_Standings_UpdateTooltips()
{
	if (g_tab->toolTipHwnd)
	{
		DestroyWindow(g_tab->toolTipHwnd);
		g_tab->toolTipHwnd = 0;
	}

	if (g_tab->sectionCount == 0)
	{
		return STATUS_OK;
	}

	g_tab->toolTipHwnd = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, 
		CW_USEDEFAULT, CW_USEDEFAULT,CW_USEDEFAULT,	CW_USEDEFAULT, g_tab->hwnd, NULL, Tabset_GetHInstance(), NULL);

	SetWindowPos(g_tab->toolTipHwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	SendMessage(g_tab->toolTipHwnd, TTM_SETMAXTIPWIDTH, 0, (LPARAM)(INT) 1600);	
	SendMessage(g_tab->toolTipHwnd, TTM_SETDELAYTIME, (WPARAM) (DWORD) TTDT_AUTOPOP, (LPARAM) MAKELONG(20000, 0));

	{
		TablePaintData_t sizes;
		RECT rect;
		Tab_Standings_Section_t *section = &g_tab->sections[g_tab->curSection];
		int i, j;
		int saveoff;
		int uid = 0;
		GetClientRect(g_tab->hwnd, &rect);
		rect.top += CONFIG_TABS_COMMON_TOP_MARGIN;

		Tab_Standings_CalcSizes(section->maxTeampairRounds, section->teamCount, &rect, &sizes);

		rect.top += sizes.yOffset;

		rect.top += sizes.headerHeight;
		saveoff = rect.left + sizes.xOffset + sizes.numWidth + sizes.titleWidth + sizes.GPwidth + sizes.MPwidth + sizes.gamesToGoWidth;

		for(i = 0; i < section->teamCount; ++i)
		{

			for(j = 0; j < section->maxTeampairRounds; ++j)
			{
				int k;
				rect.left = saveoff;
				rect.bottom = rect.top+sizes.roundHeight;

				for(k = 0; k < section->teamCount; ++k)
				{
					rect.right = rect.left + sizes.gameWidth;

					if (i != k && (section->teams[i].teamGameSet[k].teamGameCount > j))
					{
						uid++;
						Tab_Standings_AddToolTip(rect, uid++, &section->teams[i].teamGameSet[k].teamGames[j]);
					}

					rect.left = rect.right;
				}
				rect.top = rect.bottom;
			}
		}
	}

	return STATUS_OK;
}


static void Tab_Standings_DoPaint()
{
	HWND hwnd = g_tab->hwnd;
	PAINTSTRUCT ps;
	HDC hdc;
	RECT clientRect;
	RECT rect;
	TablePaintData_t sizes;
	Tab_Standings_Section_t *section = &g_tab->sections[g_tab->curSection];
	int i;
	int j;
	int curY;
	int curX;

	if (g_tab->curSection < 0 || g_tab->curSection >= g_tab->sectionCount )
	{
		return;
	}

	BeginPaint(hwnd, &ps);
	hdc = ps.hdc;
	SelectObject(hdc, Tabset_GetGdiCollection()->normalFont);
	Tabset_SelectStyle(hdc, &Tabset_GetGdiCollection()->neutralCell, 0);
	GetClientRect(hwnd, &clientRect);
	clientRect.top += CONFIG_TABS_COMMON_TOP_MARGIN;
	Tab_Standings_CalcSizes(section->maxTeampairRounds, section->teamCount, &clientRect, &sizes);
	SetBkMode(hdc, TRANSPARENT);

	//////////////////////////////////////////////////////////////////////////
	// Draw header
	//////////////////////////////////////////////////////////////////////////
	
	rect.top    = clientRect.top + sizes.yOffset;
	rect.left   = clientRect.left + sizes.xOffset;

	Tabset_SelectStyle(hdc, &Tabset_GetGdiCollection()->headerCell, 0);

	Rectangle(hdc, rect.left, rect.top, rect.left + sizes.totalWidth + 1, rect.top + sizes.headerHeight + 1);

	rect.bottom = rect.top + sizes.headerHeight;

	rect.right  = rect.left + sizes.numWidth;
	DrawText(hdc,  L"¹", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	rect.left   = rect.right;

	rect.right  = rect.left + sizes.titleWidth;
	DrawText(hdc,  L"Team", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	rect.left   = rect.right;

	rect.right  = rect.left + sizes.GPwidth;
	DrawText(hdc,  L"GP", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	rect.left   = rect.right;

	rect.right  = rect.left + sizes.MPwidth;
	DrawText(hdc,  L"MP", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	rect.left   = rect.right;

	rect.right  = rect.left + sizes.gamesToGoWidth;
	DrawText(hdc,  L"-G", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	rect.left   = rect.right;

	for(i = 1; i <= section->teamCount; ++i)
	{
		wchar_t buf[16];
		wsprintf(buf, L"Team %d", i);
		rect.right  = rect.left + sizes.gameWidth;
		DrawText(hdc,  buf, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		rect.left   = rect.right;
	}

	Tabset_SelectStyle(hdc, &Tabset_GetGdiCollection()->neutralCell, 0);

	//////////////////////////////////////////////////////////////////////////
	// Draw lines
	//////////////////////////////////////////////////////////////////////////

	for(i = 0; i < section->teamCount; ++i)
	{
		wchar_t buf[16];
		int saveoff;

		rect.top    = rect.bottom;
		rect.left   = clientRect.left + sizes.xOffset;
		rect.bottom = rect.top + sizes.lineHeight;

		rect.right  = rect.left+sizes.numWidth;
		wsprintf(buf, L"%d", i+1);
		DrawText(hdc,  buf, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		rect.left   = rect.right;

		rect.right  = rect.left+sizes.titleWidth;
		DrawTextA(hdc,  section->teams[i].name, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
		rect.left   = rect.right;

		rect.right  = rect.left+sizes.GPwidth;
		swprintf_s(buf, sizeof(buf)/sizeof(wchar_t), L"%3.1f", section->teams[i].GP2/2.0);
		DrawText(hdc,  buf, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		rect.left   = rect.right;

		rect.right  = rect.left+sizes.MPwidth;
		swprintf_s(buf, sizeof(buf)/sizeof(wchar_t), L"%3.1f", section->teams[i].MP2/2.0);
		DrawText(hdc,  buf, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		rect.left   = rect.right;

		rect.right  = rect.left+sizes.gamesToGoWidth;
		wsprintf(buf, L"%d", section->teams[i].gamesToPlay);
		DrawText(hdc,  buf, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		rect.left   = rect.right;

		saveoff = rect.left;
		for(j = 0; j < section->maxTeampairRounds; ++j)
		{
			int k;
			rect.left = saveoff;
			rect.bottom = rect.top+sizes.roundHeight;

			for(k = 0; k < section->teamCount; ++k)
			{
				rect.right = rect.left + sizes.gameWidth;

				if (i != k)
				{
					if (section->teams[i].teamGameSet[k].teamGameCount <= j)
					{
						Tabset_SelectStyle(hdc, &Tabset_GetGdiCollection()->headerCell, 0);
						Rectangle(hdc, rect.left, rect.top, rect.right+1, rect.bottom+1);
					}
					else
					{
						Tab_Standings_DrawGame(hdc, &rect, &section->teams[i].teamGameSet[k].teamGames[j]);
					}
				}

				rect.left = rect.right;
			}
			rect.top = rect.bottom;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Draw grid
	//////////////////////////////////////////////////////////////////////////

	Tabset_SelectStyle(hdc, &Tabset_GetGdiCollection()->neutralCell, 0);

	curY = clientRect.top + sizes.yOffset;
	MoveToEx(hdc, clientRect.left+sizes.xOffset, curY, 0);
	LineTo(hdc, clientRect.left+sizes.xOffset+sizes.totalWidth, curY);
	curY += sizes.headerHeight;

	for(i = 0; i <= section->teamCount; ++i)
	{
		MoveToEx(hdc, clientRect.left+sizes.xOffset, curY, 0);
		LineTo(hdc, clientRect.left+sizes.xOffset+sizes.totalWidth, curY);
		if (i < section->teamCount)
			for(j = 0; j < section->maxTeampairRounds; ++j)
			{
				MoveToEx(hdc, clientRect.left+sizes.xOffset+sizes.totalWidth - section->teamCount*sizes.gameWidth, curY, 0);
				LineTo(hdc, clientRect.left+sizes.xOffset+sizes.totalWidth - (section->teamCount-i)*sizes.gameWidth, curY);
				MoveToEx(hdc, clientRect.left+sizes.xOffset+sizes.totalWidth - (section->teamCount-i-1)*sizes.gameWidth, curY, 0);
				LineTo(hdc, clientRect.left+sizes.xOffset+sizes.totalWidth, curY);
				curY += sizes.roundHeight;
			}
	}

	curX = clientRect.left + sizes.xOffset;
	MoveToEx(hdc, curX, clientRect.top+sizes.yOffset, 0);
	LineTo(hdc, curX, clientRect.top+sizes.yOffset+sizes.totalHeight);
	curX += sizes.numWidth;

	MoveToEx(hdc, curX, clientRect.top+sizes.yOffset, 0);
	LineTo(hdc, curX, clientRect.top+sizes.yOffset+sizes.totalHeight);
	curX += sizes.titleWidth;

	MoveToEx(hdc, curX, clientRect.top+sizes.yOffset, 0);
	LineTo(hdc, curX, clientRect.top+sizes.yOffset+sizes.totalHeight);
	curX += sizes.GPwidth;

	MoveToEx(hdc, curX, clientRect.top+sizes.yOffset, 0);
	LineTo(hdc, curX, clientRect.top+sizes.yOffset+sizes.totalHeight);
	curX += sizes.MPwidth;

	MoveToEx(hdc, curX, clientRect.top+sizes.yOffset, 0);
	LineTo(hdc, curX, clientRect.top+sizes.yOffset+sizes.totalHeight);
	curX += sizes.gamesToGoWidth;

	for(i = 0; i <= section->teamCount; ++i)
	{
		MoveToEx(hdc, curX, clientRect.top+sizes.yOffset, 0);
		LineTo(hdc, curX, clientRect.top+sizes.yOffset+sizes.totalHeight+1);
		curX += sizes.gameWidth;
	}

	rect.left = clientRect.left + sizes.xOffset + sizes.totalWidth - sizes.gameWidth * section->teamCount;
	rect.top = clientRect.top + sizes.yOffset + sizes.headerHeight;

	for(i = 0; i < section->teamCount; ++i)
	{
		LOGBRUSH lbrush;
		HBRUSH brush;

		rect.right = rect.left + sizes.gameWidth;
		rect.bottom = rect.top + sizes.lineHeight;

		lbrush.lbColor = Tabset_GetGdiCollection()->fgColor;
		lbrush.lbStyle = BS_HATCHED;
		lbrush.lbHatch = HS_DIAGCROSS;
		brush = CreateBrushIndirect(&lbrush);
		SelectObject(hdc, brush);

		Rectangle(hdc, rect.left, rect.top, rect.right + 1, rect.bottom + 1);

		SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
		DeleteObject(brush);

		rect.left = rect.right;
		rect.top = rect.bottom;
	}


	EndPaint(hwnd, &ps);
}

static LRESULT CALLBACK Tab_Standings_WndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
	case WM_COMMAND:
		{
			HWND sender = (HWND)(lParam);
			if (sender == g_tab->dropDownHwnd && HIWORD(wParam) == CBN_SELCHANGE)
			{
				int newSel = (int)SendMessage(sender, CB_GETCURSEL, 0, 0);
				RECT rect;
				g_tab->curSection = newSel;
				strcpy_s(Settings_GetSettings()->curSectionTitle, sizeof(Settings_GetSettings()->curSectionTitle), g_tab->sections[newSel].title);
				GetClientRect(hwnd, &rect);
				rect.top += CONFIG_TABS_COMMON_TOP_MARGIN;
				Tab_Standings_UpdateTooltips();
				InvalidateRect(hwnd, &rect, TRUE);
				return 0;
			}
		}
		break;
	case WM_PAINT:
		{
			Tab_Standings_DoPaint();
			return 0;
		}
	case WM_CREATE:
		{
			/* CREATESTRUCT *createStruct = (CREATESTRUCT*)lParam; */
			g_tab->hwnd = hwnd;
			Tab_Standings_UpdateDropDown();
			return 0;
		}
	case WM_SIZE:
		{
			int width = LOWORD(lParam);
			int height = HIWORD(lParam);
			(void)width;
			(void)height;
			Tab_Standings_UpdateTooltips();

			return 0;
		}
	case WM_DESTROY:
		{
			free(g_tab);
			g_tab = 0;
			return 0;
		}
	case WM_LBUTTONDOWN:
		{
			if (g_tab->dropDownHwnd)
			{
				SetFocus(g_tab->dropDownHwnd);
			}
			return 0;
		}
	case WM_RBUTTONUP:
		{
			Tab_Common_SimpleGameData_t gameData;
			int flags = 0;
			Tab_Common_DoMenu(g_tab->hwnd, flags, &gameData);
			return 0;
		}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);

}


static Status_t Tab_Standings_Init()
{
	WNDCLASS wndclass;  
	wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS ; 
	wndclass.lpfnWndProc = Tab_Standings_WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = Tabset_GetHInstance();
	wndclass.hIcon = NULL;
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW); 
	wndclass.hbrBackground = CreateSolidBrush(Tabset_GetGdiCollection()->bgColor); // TODO fix it
	wndclass.lpszMenuName =  NULL;
	wndclass.lpszClassName = L"moomoo_StandingsTab";
	if (!RegisterClass(&wndclass))
	{
		return STATUS_ERR_WINDOW_CANNOT_REGISTER;
	}

	return STATUS_OK;
}

static int Tab_Standings_CmpTeams(Tab_Standings_Section_t* section, const int* team1idx, const int* team2idx)
{
	Tab_Standings_Team_t *team1 = &section->teams[*team1idx];
	Tab_Standings_Team_t *team2 = &section->teams[*team2idx];

	if (team1->MP2 != team2->MP2)
	{
		return team2->MP2 - team1->MP2;
	}

	if (team1->GP2 != team2->GP2)
	{
		return team2->GP2 - team1->GP2;
	}

	return team2->gamesToPlay - team1->gamesToPlay;

}

static Status_t Tab_Standings_UpdateData()
{
	const Dataset_t *dataSet = Dataset_GetDataset();
	Tab_Standings_Section_t *sections; // [CONFIG_MAX_SECTIONS_PER_SEASON];
	int sectionCount = 0;
	int i;
	int j;

	g_tab->sectionCount = 0;

	if (!dataSet->isDataValid)
	{
		return STATUS_OK;
	}

	sections = (Tab_Standings_Section_t *)malloc(sizeof(g_tab->sections));
	memset(sections, 0, sizeof(g_tab->sections));

	for(i = 0; i < dataSet->teamGameCount; ++i)
	{
		const TeamGame_t *teamGame = &dataSet->teamGames[i];
		Tab_Standings_Section_t *section;
		int team1index = 0;
		int team2index = 0;

		//////////////////////////////////////////////////////////////////////////
		// Searching for section
		//////////////////////////////////////////////////////////////////////////

		for(j = 0; j < sectionCount; ++j)
			if (strcmp(teamGame->secton, sections[j].title) == 0)
				break;

		if (j == sectionCount)
		{
			if (sectionCount == CONFIG_MAX_SECTIONS_PER_SEASON)
			{
				free(sections);
				return STATUS_ERR_NOT_ENOUGH_ROOM;
			}

			strcpy_s(sections[j].title, CONFIG_MAX_SECTION_TITLE_LENGTH, teamGame->secton);
			++sectionCount;
		}

		section = &sections[j];

		//////////////////////////////////////////////////////////////////////////
		// searching for team 1
		//////////////////////////////////////////////////////////////////////////

		for(team1index = 0; team1index < section->teamCount; ++team1index)
			if (strcmp(section->teams[team1index].name, teamGame->team1) == 0)
				break;

		if (team1index == section->teamCount)
		{
			if (section->teamCount == CONFIG_MAX_TEAMS_PER_SECTION)
			{
				free(sections);
				return STATUS_ERR_NOT_ENOUGH_ROOM;
			}

			strcpy_s(section->teams[team1index].name, CONFIG_MAX_TEAM_TITLE_LENGTH, teamGame->team1);
			++section->teamCount;
		}

		//////////////////////////////////////////////////////////////////////////
		// searching for team 2
		//////////////////////////////////////////////////////////////////////////

		for(team2index = 0; team2index < section->teamCount; ++team2index)
			if (strcmp(section->teams[team2index].name, teamGame->team2) == 0)
				break;

		if (team2index == section->teamCount)
		{
			if (section->teamCount == CONFIG_MAX_TEAMS_PER_SECTION)
			{
				free(sections);
				return STATUS_ERR_NOT_ENOUGH_ROOM;
			}

			strcpy_s(section->teams[team2index].name, CONFIG_MAX_TEAM_TITLE_LENGTH, teamGame->team2);
			++section->teamCount;
		}

		//////////////////////////////////////////////////////////////////////////
		// Building teamGames
		//////////////////////////////////////////////////////////////////////////

		{
			Tab_Standings_Team_t *team1 = &section->teams[team1index];
			Tab_Standings_Team_t *team2 = &section->teams[team2index];
			TeamGameStat_t *team1game;
			TeamGameStat_t *team2game;

			if (team1->teamGameSet[team2index].teamGameCount == CONFIG_MAX_TEAMPAIR_GAMES_PER_SECTION ||
				team2->teamGameSet[team1index].teamGameCount == CONFIG_MAX_TEAMPAIR_GAMES_PER_SECTION)
			{
				free(sections);
				return STATUS_ERR_NOT_ENOUGH_ROOM;
			}

			team1game = &team1->teamGameSet[team2index].teamGames[team1->teamGameSet[team2index].teamGameCount];
			team2game = &team2->teamGameSet[team1index].teamGames[team2->teamGameSet[team1index].teamGameCount];
			team1game->teamGame = teamGame;
			team2game->teamGame = teamGame;
			++team1->teamGameSet[team2index].teamGameCount;
			++team2->teamGameSet[team1index].teamGameCount;

			if (section->maxTeampairRounds < team1->teamGameSet->teamGameCount)
				section->maxTeampairRounds = team1->teamGameSet->teamGameCount;

			if (section->maxTeampairRounds < team2->teamGameSet->teamGameCount)
				section->maxTeampairRounds = team2->teamGameSet->teamGameCount;

			//////////////////////////////////////////////////////////////////////////
			// Populating with games
			//////////////////////////////////////////////////////////////////////////

			team1game->round = teamGame->round;
			team2game->round = teamGame->round;

			for(j = 0; j < teamGame->gameCount; ++j)
			{
				const Game_t *game = &teamGame->teamGames[j];
				team1game->played = TRUE;
				team2game->played = TRUE;

				switch(game->status)
				{
				case GAMESTATUS_NotScheduled:
				case GAMESTATUS_Pending:
				case GAMESTATUS_InProgress:
				case GAMESTATUS_Adjourned:
					++team1->gamesToPlay;
					++team2->gamesToPlay;
					++team1game->gameToGo;
					++team2game->gameToGo;
					break;
				case GAMESTATUS_Win_1_0:
				case GAMESTATUS_Win_i_o:
				case GAMESTATUS_Win_plus_minus:
					team1game->GP2 += 2;
					team2game->oppGP2 += 2;
					break;
				case GAMESTATUS_Lose_0_1:
				case GAMESTATUS_Lose_o_i:
				case GAMESTATUS_Lose_minus_plus:
					team2game->GP2 += 2;
					team1game->oppGP2 += 2;
					break;
				case GAMESTATUS_Draw_12_12:
				case GAMESTATUS_Draw_i2_i2:
					team1game->GP2 += 1;
					team2game->GP2 += 1;
					team1game->oppGP2 += 1;
					team2game->oppGP2 += 1;
					break;
				}
			}

			team1->GP2 += team1game->GP2;
			team2->GP2 += team2game->GP2;
			team1->MP2 += Tab_Standings_GetMinMP2(team1game);
			team2->MP2 += Tab_Standings_GetMinMP2(team2game);
		}
	}


	g_tab->sectionCount = sectionCount;

	// Sorting

	for(i = 0; i < sectionCount; ++i)
	{
		Tab_Standings_Section_t *srcSection = &sections[i];
		Tab_Standings_Section_t *dstSection = &g_tab->sections[i];
		int indices[CONFIG_MAX_TEAMS_PER_SECTION];
		int j;

		dstSection->teamCount = srcSection->teamCount;
		dstSection->maxTeampairRounds = srcSection->maxTeampairRounds;
		strcpy_s(dstSection->title, sizeof(dstSection->title), srcSection->title);

		for(j = 0; j < srcSection->teamCount; ++j)
		{
			indices[j] = j;
		}

        qsort_s(indices, srcSection->teamCount, sizeof(indices[0]), (int (__cdecl *)(void *,const void *,const void *))Tab_Standings_CmpTeams, srcSection);

		for(j = 0; j < srcSection->teamCount; ++j)
		{
			Tab_Standings_Team_t *srcTeam = &srcSection->teams[indices[j]];
			Tab_Standings_Team_t *dstTeam = &dstSection->teams[j];
			int k;

			dstTeam->gamesToPlay = srcTeam->gamesToPlay;
			dstTeam->GP2 = srcTeam->GP2;
			dstTeam->MP2 = srcTeam->MP2;
			strcpy_s(dstTeam->name, sizeof(dstTeam->name), srcTeam->name);

			for(k = 0; k < srcSection->teamCount; ++k)
			{
				memcpy_s(&dstTeam->teamGameSet[k], sizeof(dstTeam->teamGameSet[k]), &srcTeam->teamGameSet[indices[k]], sizeof(srcTeam->teamGameSet[indices[k]]));
			}

		}

	}

	free(sections);
	return STATUS_OK;
}


static Status_t Tab_Standings_Event(TabEvent_t event)
{
	switch (event)
	{
	case TABEVENT_DATA_UPDATED:
		{
			Status_t status = Tab_Standings_UpdateData();
			if (status != STATUS_OK)
			{
				return status;
			}
        }
	case TABEVENT_CONFIG_CHANGED: // Fall through
        {
			RECT rect;
			GetClientRect(g_tab->hwnd, &rect); 
			InvalidateRect(g_tab->hwnd, &rect, TRUE); // TODO make in false
			Tab_Standings_UpdateDropDown();
			Tab_Standings_UpdateTooltips();
			return STATUS_OK;
		}
	case TABEVENT_INIT:
		return Tab_Standings_Init();
	}
	return STATUS_OK_NOT_PROCESSED;
}

static Status_t Tab_Standings_Create(HWND parent, HWND* child)
{
	g_tab = (Tab_Standings_t*)malloc(sizeof(Tab_Standings_t));
	if (!g_tab)
	{
		return STATUS_ERR_NOT_ENOUGH_ROOM;
	}
	memset(g_tab, 0, sizeof(*g_tab));

	Tab_Standings_UpdateData();
	g_tab->hwnd = CreateWindow(L"moomoo_StandingsTab", L"", WS_CHILD|WS_VISIBLE, 0, 0, 0, 0, parent, 0, Tabset_GetHInstance(), 0);
	if (!g_tab->hwnd)
	{
		return STATUS_ERR_WINDOW_CANNOT_CREATE;
	}
	*child = g_tab->hwnd;
	return STATUS_OK;
}

static Status_t Tab_Standings_GetTabTitle(char* title, int length)
{
	sprintf_s(title, length, "Standings");
	return STATUS_OK;
}

Status_t Tab_Standings_GetHandlers(TabHandlers_t* handlers)
{
	handlers->onCreate = Tab_Standings_Create;
	handlers->onEvent = Tab_Standings_Event;
	handlers->onGetTabTitle = Tab_Standings_GetTabTitle;
	return STATUS_OK;
}