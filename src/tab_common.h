#ifndef __TAB_COMMON_INCLUDED__
#define __TAB_COMMON_INCLUDED__

#include "status.h"
#include <Windows.h>
#include "tabset.h"
#include "dataset.h"

typedef struct  
{
	int Round;
	char team1[CONFIG_MAX_TEAM_TITLE_LENGTH];
	char team2[CONFIG_MAX_TEAM_TITLE_LENGTH];
	char player1[CONFIG_MAX_NICKNAME_LENGTH];
	char player2[CONFIG_MAX_NICKNAME_LENGTH];
	char secton[CONFIG_MAX_SECTION_TITLE_LENGTH];
	int gameId;
} Basic_GameData_t;

typedef struct  
{
	int xOffset;
	int yOffset;

	int headerHeight;
	int rowHeight;

	int roundWidth;
	int sectionWidth;
	int playerWidth;

	int totalWidth;
} Tab_Common_StdTablePaintData_t;

typedef struct  
{
	char team1[CONFIG_MAX_TEAM_TITLE_LENGTH];
	char player1[CONFIG_MAX_TEAM_TITLE_LENGTH];
	char team2[CONFIG_MAX_NICKNAME_LENGTH];
	char player2[CONFIG_MAX_NICKNAME_LENGTH];
	int gameId;
	int round;
} Tab_Common_SimpleGameData_t;

typedef enum 
{
	MENUTYPE_Team             = 0x00000001,
	MENUTYPE_Team1            = MENUTYPE_Team,
	MENUTYPE_Team2            = 0x00000002,
	MENUTYPE_Game             = 0x00000004,
	MENUTYPE_GameObserveable  = 0x0000000c,
	MENUTYPE_GameStartable    = 0x00000010,
	// Player
	MENUTYPE_Player           = 0x00000020,
	MENUTYPE_PlayerObservable = 0x00000060,
	MENUTYPE_Plugin           = 0x00000080,
	MENUTYPE_ObserveAll       = 0x00000100,

	MENUTYPE_GameExaminable   = 0x00000200,
} Tab_Common_Menutype_t;

typedef enum
{
	HITPOINT_Milk,
	HITPOINT_Round,
	HITPOINT_Section,
	HITPOINT_Player1,
	HITPOINT_Player2,
	HITPOINT_Time,
	HITPOINT_TeamGame,
	HITPOINT_Outcome,
} Tab_HitPoint_t;

typedef void (*PaintDCFunc_t)(HDC hdc, int offset, RECT clientRect);

Status_t Tab_Common_Init();
Status_t Tab_Common_WndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *res);
Status_t Tab_Common_Event(TabEvent_t event);
Status_t Tab_Common_UpdateDropDown(HWND *dropDownHwnd, HWND parent, BOOL hasData, int width, int height);
const TabSet_ColorStyle_t* Tab_Common_GetGameStyle(int minScore, int maxScore);
const TabSet_ColorStyle_t* Tab_Common_GetTeamGameStyle(int minScore, int maxScore);
const TabSet_ColorStyle_t* Tab_Common_GetTeamGameStyleByStatus(Game_Status_t status);
const TabSet_ColorStyle_t* Tab_Common_GetTeamGametStyle(const TeamGame_t *teamGame);
void Tab_Common_DoPaint(HWND hwnd, PaintDCFunc_t func);
void Tab_Common_CalcStdSizes(const RECT* rect, Tab_Common_StdTablePaintData_t* sizes);
void Tab_Common_DrawSimpleTable(HDC hdc, RECT *clientRect, const Tab_Common_StdTablePaintData_t *sizes, const Basic_GameData_t* data, int count);
Status_t Tab_Common_PopulateStdData( BOOL onlyFavorites, Basic_GameData_t* newEntry, int *count, int maxCount, Game_Status_t status );
Status_t Tab_Common_UpdateCheckBox(HWND parent, const RECT* rect, BOOL show);
Status_t Tab_Common_DoMenu(HWND hwnd, unsigned int flags, const Tab_Common_SimpleGameData_t* gameData);
int Tab_Common_GetScrollOffset(HWND hwnd);
Tab_HitPoint_t Tab_Common_StdTableHitTest(POINT point, const Tab_Common_StdTablePaintData_t *sizes, const Basic_GameData_t* data, 
										  int count, Tab_Common_SimpleGameData_t *gameData);


#endif