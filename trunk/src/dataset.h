#ifndef __DATASET_INCLUDED__
#define __DATASET_INCLUDED__

#include "config.h"
#include "status.h"
#include <windows.h>

typedef enum
{
	GAMESTATUS_NotScheduled,
	GAMESTATUS_Pending,
	GAMESTATUS_InProgress,
	GAMESTATUS_Adjourned,

	GAMESTATUS_Win_1_0,
	GAMESTATUS_Win_i_o,
	GAMESTATUS_Win_plus_minus,
	GAMESTATUS_Lose_0_1,
	GAMESTATUS_Lose_o_i,
	GAMESTATUS_Lose_minus_plus,
	GAMESTATUS_Draw_12_12,
	GAMESTATUS_Draw_i2_i2,
	GAMESTATUS_DoubleForfeit_o_o,
	GAMESTATUS_DoubleForfeit_minus_minus,
	GAMESTATUS_Error
} Game_Status_t;

typedef struct 
{
	int gameId;
	char player1[CONFIG_MAX_NICKNAME_LENGTH];
	char player2[CONFIG_MAX_NICKNAME_LENGTH];
	Game_Status_t status;
	FILETIME scheduledTime;
	BOOL isPlayer1White;
} Game_t;

typedef struct 
{
	int round;
	char secton[CONFIG_MAX_SECTION_TITLE_LENGTH];
	char team1[CONFIG_MAX_TEAM_TITLE_LENGTH];
	char team2[CONFIG_MAX_TEAM_TITLE_LENGTH];
	Game_t teamGames[CONFIG_MAX_GAMES_PER_TEAMGAME];
	int gameCount;
} TeamGame_t;

typedef struct 
{
	// TODO add support of timestamp
	// TODO add support of errors
	BOOL isDataValid;
	BOOL isUpdateInProgress;
	int  teamGameCount;
	TeamGame_t teamGames[CONFIG_MAX_TEAMGAMES_PER_SEASON];
} Dataset_t;

Status_t Dataset_init();
Status_t Dataset_ProcessDataChunk(const char* chunk);
Status_t Dataset_Abort();
const Dataset_t* Dataset_GetDataset();
void Dataset_ReverseTeamGame(TeamGame_t* teamGame);
const char* Dataset_OutcomeToStr(Game_Status_t status);
void SwapData(char* buf1, char* buf2, int size);
BOOL Dataset_ProcessHotData(const char* msg);
BOOL Dataset_IsFinishedStatus(Game_Status_t status);

#endif