#include "dataset.h"
#include <stdio.h>
#include "settings.h"
#include "engine.h"

typedef struct  
{
	int  teamGameCount;
	TeamGame_t teamGames[CONFIG_MAX_TEAMGAMES_PER_SEASON];
} Work_Dataset_t;

static Dataset_t g_dataset;
static Work_Dataset_t g_workDataset;



const Dataset_t* Dataset_GetDataset()
{
	return &g_dataset;
}



Status_t Dataset_init()
{
	g_dataset.teamGameCount = 0;
	g_dataset.isDataValid = FALSE;
	g_dataset.isUpdateInProgress = FALSE;
	
	return STATUS_OK;
}

const char* Dataset_OutcomeToStr(Game_Status_t status)
{
	switch (status)
	{
	case GAMESTATUS_NotScheduled:
		return "-";
	case GAMESTATUS_Pending:
		return "+";
	case GAMESTATUS_InProgress:
		return "*";
	case GAMESTATUS_Adjourned:
		return "ADJ";
	case GAMESTATUS_Win_1_0:
		return "1-0";
	case GAMESTATUS_Win_i_o:
		return "i-o";
	case GAMESTATUS_Win_plus_minus:
		return "+:-";
	case GAMESTATUS_Lose_0_1:
		return "0-1";
	case GAMESTATUS_Lose_o_i:
		return "o-i";
	case GAMESTATUS_Lose_minus_plus:
		return "-:+";
	case GAMESTATUS_Draw_12_12:
		return "1/2-1/2";
	case GAMESTATUS_Draw_i2_i2:
		return "i/2-i/2";
	case GAMESTATUS_DoubleForfeit_o_o:
		return "o-o";
	case GAMESTATUS_DoubleForfeit_minus_minus:
		return "-:-";
	case GAMESTATUS_Error:
		return "?";
	}
	return "???";
}

static Status_t Dataset_SetOutcome(Game_t *game, const char* outcome)
{
#define OUTCOME_OPTION(d_strval, d_status) if (strcmp(outcome, d_strval) == 0) { game->status = d_status; return STATUS_OK; }

 		 OUTCOME_OPTION("-",GAMESTATUS_NotScheduled)
	else OUTCOME_OPTION("+",GAMESTATUS_Pending)
	else OUTCOME_OPTION("*",GAMESTATUS_InProgress)
	else OUTCOME_OPTION("ADJ", GAMESTATUS_Adjourned)
	else OUTCOME_OPTION("1-0",GAMESTATUS_Win_1_0)
	else OUTCOME_OPTION("i-o",GAMESTATUS_Win_i_o)
	else OUTCOME_OPTION("+:-",GAMESTATUS_Win_plus_minus)
	else OUTCOME_OPTION("0-1",GAMESTATUS_Lose_0_1)
	else OUTCOME_OPTION("o-i",GAMESTATUS_Lose_o_i)
	else OUTCOME_OPTION("-:+",GAMESTATUS_Lose_minus_plus)
	else OUTCOME_OPTION("1/2-1/2",GAMESTATUS_Draw_12_12)
	else OUTCOME_OPTION("i/2-i/2",GAMESTATUS_Draw_i2_i2)
	else OUTCOME_OPTION("o-o",GAMESTATUS_DoubleForfeit_o_o)
	else OUTCOME_OPTION("-:-",GAMESTATUS_DoubleForfeit_minus_minus)
	else
		return STATUS_ERR_BOT_SYNTAX_ERROR;

#undef OUTCOME_OPTION
}


void SwapData(char* buf1, char* buf2, int size)
{
	while (size--)
	{
		char c = *buf1;
		*buf1++ = *buf2;
		*buf2++ = c;
	}
}

 static void Dataset_ReverseStatus(Game_Status_t* status)
{
	switch(*status)
	{
	case GAMESTATUS_Lose_minus_plus:	*status = GAMESTATUS_Win_plus_minus; break;
	case GAMESTATUS_Lose_0_1:			*status = GAMESTATUS_Win_1_0; break;
	case GAMESTATUS_Lose_o_i:			*status = GAMESTATUS_Win_i_o; break;
	case GAMESTATUS_Win_plus_minus:		*status = GAMESTATUS_Lose_minus_plus; break;
	case GAMESTATUS_Win_1_0:			*status = GAMESTATUS_Lose_0_1; break;
	case GAMESTATUS_Win_i_o:			*status = GAMESTATUS_Lose_o_i; break;
	}
}

 static void Dataset_ReverseGame(Game_t* game)
 {
	 game->isPlayer1White = !game->isPlayer1White;
	 Dataset_ReverseStatus(&game->status);
	 SwapData(game->player1, game->player2, sizeof(game->player1));
 }

 void Dataset_ReverseTeamGame(TeamGame_t* teamGame)
 {
	 int i;
	 SwapData(teamGame->team1, teamGame->team2, sizeof(teamGame->team1));
	 for(i = 0; i < teamGame->gameCount; ++i)
	 {
		 Dataset_ReverseGame(&teamGame->teamGames[i]);
	 }
 }

static Status_t Dataset_ProcessStart()
{
	if (g_dataset.isUpdateInProgress)
	{
		return STATUS_ERR_GAME_UPDATE_IN_PROGRESS;
	}
	g_dataset.isUpdateInProgress = TRUE;
	g_workDataset.teamGameCount = 0;

	return STATUS_OK_GAME_UPDATE_STARTED;
}



static Status_t Dataset_ProcessUpdate(const char* chunk)
{
	int increment = 0;
	Game_t game;
	int res;
	char outcome[16];
	int boardnum;
	char season[16];
	TeamGame_t *teamGame;
	int unixTime;

	if (!g_dataset.isUpdateInProgress)
	{
		return STATUS_ERR_GAME_UPDATE_NOT_IN_PROGRESS;
	}

	if (g_workDataset.teamGameCount >= CONFIG_MAX_TEAMGAMES_PER_SEASON)
	{
		Dataset_Abort();
		return STATUS_ERR_NOT_ENOUGH_ROOM;
	}

	teamGame = &g_workDataset.teamGames[g_workDataset.teamGameCount];

	res = sscanf_s(chunk,  "%s %s %d %s %s%n",
						 season, sizeof(season),
						 teamGame->secton, CONFIG_MAX_SECTION_TITLE_LENGTH,
						 &teamGame->round, 
						 teamGame->team1, CONFIG_MAX_TEAM_TITLE_LENGTH,
						 teamGame->team2, CONFIG_MAX_TEAM_TITLE_LENGTH,
						 &increment);
	if (res < 4)
	{
		Dataset_Abort();
		return STATUS_ERR_BOT_SYNTAX_ERROR;
	}
	chunk += increment;
	++g_workDataset.teamGameCount;
	
	teamGame->gameCount = 0;

	while(6 <=
		sscanf_s(chunk,
		         "%d %d %s %s %s %d%n",
				 &boardnum, &game.gameId, 
				 game.player1, CONFIG_MAX_NICKNAME_LENGTH,
				 game.player2, CONFIG_MAX_NICKNAME_LENGTH,
				 outcome, 16,
				 &unixTime,
				 &increment)
		)
	{
		LONGLONG ll = Int32x32To64(unixTime, 10000000) + 116444736000000000;

		if (teamGame->gameCount >= CONFIG_MAX_GAMES_PER_TEAMGAME)
		{
			Dataset_Abort();
			return STATUS_ERR_NOT_ENOUGH_ROOM;
		}
		game.isPlayer1White = boardnum%2;

		if (Dataset_SetOutcome(&game, outcome) != STATUS_OK)
		{
			Dataset_Abort();
			return STATUS_ERR_BOT_SYNTAX_ERROR;
		}

		if (!game.isPlayer1White)
		{
			Dataset_ReverseStatus(&game.status);
			SwapData(game.player1, game.player2, sizeof(game.player1));
		}

		game.scheduledTime.dwLowDateTime = (DWORD)ll;
		game.scheduledTime.dwHighDateTime = (DWORD)(ll >> 32);

		memcpy(&teamGame->teamGames[teamGame->gameCount++], &game, sizeof(game));
		chunk += increment;
	}

	return STATUS_OK;
}



static Status_t Dataset_ProcessFinish()
{
	if (!g_dataset.isUpdateInProgress)
	{
		return STATUS_ERR_GAME_UPDATE_NOT_IN_PROGRESS;
	}
	if (g_workDataset.teamGameCount == 0)
	{
		g_dataset.isUpdateInProgress = FALSE;
		g_dataset.isDataValid = FALSE;
		g_dataset.teamGameCount = 0;
		return STATUS_OK_EMPTY_DATASET;
	}

	g_dataset.isUpdateInProgress = FALSE;
	g_dataset.teamGameCount = g_workDataset.teamGameCount;
	g_dataset.isDataValid = TRUE;
	memcpy(g_dataset.teamGames, g_workDataset.teamGames, sizeof(TeamGame_t)*g_workDataset.teamGameCount );

	
	return STATUS_OK_GAME_UPDATE_COMPLETED;
}



Status_t Dataset_Abort()
{
	if (!g_dataset.isUpdateInProgress)
	{
		return STATUS_ERR_GAME_UPDATE_NOT_IN_PROGRESS;
	}
	g_dataset.isUpdateInProgress = FALSE;
	return STATUS_OK;
}



Status_t Dataset_ProcessDataChunk(const char* chunk)
{
	int increment = 0;
	char tmpBuf[24];
	int res;

	res = sscanf_s(chunk, "%s%n", tmpBuf, sizeof(tmpBuf), &increment);
	if (res == 0 || strcmp(CONFIG_BOT_QTELL_IDENTIFIER, tmpBuf) != 0)
	{
		return STATUS_ERR_BOT_SYNTAX_ERROR;
	}

	chunk += increment;

	res = sscanf_s(chunk, "%s%n", tmpBuf, sizeof(tmpBuf), &increment);
	if (res == 0)
	{
		return STATUS_ERR_BOT_SYNTAX_ERROR;
	}

	if (strcmp(tmpBuf, CONFIG_BOT_QTELL_UPDATE) == 0)
	{
		chunk += increment;
		return Dataset_ProcessUpdate(chunk);
	}
	else if (strcmp(tmpBuf, CONFIG_BOT_QTELL_CONTROL) == 0)
	{
		chunk += increment;
		res = sscanf_s(chunk, "%s", tmpBuf, sizeof(tmpBuf));
		if (res == 0)
		{
			return STATUS_ERR_BOT_SYNTAX_ERROR;
		}

		if (strcmp(tmpBuf, CONFIG_BOT_QTELL_START) == 0)
		{
			return Dataset_ProcessStart();
		}
		else if (strcmp(tmpBuf, CONFIG_BOT_QTELL_FINISH) == 0)
		{
		return Dataset_ProcessFinish();
		}
	}

	return STATUS_ERR_BOT_SYNTAX_ERROR;
}

BOOL Dataset_IsFinishedStatus(Game_Status_t status)
{
	switch(status)
	{
	case GAMESTATUS_Win_1_0:
	case GAMESTATUS_Win_i_o:
	case GAMESTATUS_Win_plus_minus:
	case GAMESTATUS_Lose_0_1:
	case GAMESTATUS_Lose_o_i:
	case GAMESTATUS_Lose_minus_plus:
	case GAMESTATUS_Draw_12_12:
	case GAMESTATUS_Draw_i2_i2:
	case GAMESTATUS_DoubleForfeit_o_o:
	case GAMESTATUS_DoubleForfeit_minus_minus:
		return TRUE;
	}
	return FALSE;
}

BOOL Dataset_ProcessHotData(const char* msg)
{
	char team1[CONFIG_MAX_TEAM_TITLE_LENGTH];
	char team2[CONFIG_MAX_TEAM_TITLE_LENGTH];
	char player1[CONFIG_MAX_NICKNAME_LENGTH];
	char player2[CONFIG_MAX_NICKNAME_LENGTH];
	char section[CONFIG_MAX_SECTION_TITLE_LENGTH];
	int board;
	char outcome[16];
	Game_Status_t statusToSet = GAMESTATUS_Error;

	if (!g_dataset.isDataValid)
	{
		return FALSE;
	}

	// 	[15:39]TeamLeague(TD): Buriag(RainbowWarriors1800) vs. rederik(queens_crusaders1800) -- Section U1800a, board 3 has ended as: 1-0.
	if (sscanf_s(msg, "%[^(](%[^)]) vs. %[^(](%[^)]) -- Section %[^,], board %d has ended as: %[^.]",
		         player1, sizeof(player1), team1, sizeof(team1), player2, sizeof(player2), team2, sizeof(team2), 
				 section, sizeof(section), &board, outcome, sizeof(outcome)) == 7)
	{
		if (strcmp(outcome, "ADJ") == 0)
		{
			statusToSet = GAMESTATUS_Adjourned;
		}
		else if (strcmp(outcome, "1/2-1/2") == 0)
		{
			statusToSet = GAMESTATUS_Draw_12_12;
		}
		else if (strcmp(outcome, "1-0") == 0)
		{
			if (board % 2)
			{
				statusToSet = GAMESTATUS_Win_1_0;
			}
			else
			{
				statusToSet = GAMESTATUS_Lose_0_1;
			}
		}
		else if (strcmp(outcome, "0-1") == 0)
		{
			if (board % 2)
			{
				statusToSet = GAMESTATUS_Lose_0_1;

			}
			else
			{
				statusToSet = GAMESTATUS_Win_1_0;
			}
		}
	}
	// TeamLeague(TD): Game started: Mapleleaf(CurrentAffairs) vs. XFrame(MonkeyClub2000) -- Section U2000a, board 2. To watch it "observe 208".
	else if (sscanf_s(msg, "Game started: %[^(](%[^)]) vs. %[^(](%[^)]) -- Section %[^,], board %d",
		player1, sizeof(player1), team1, sizeof(team1), player2, sizeof(player2), team2, sizeof(team2), 
		section, sizeof(section), &board) == 6)
	{
		statusToSet = GAMESTATUS_InProgress;

#ifdef ENABLE_TELLING_TO_CHANNEL_177
		if (Settings_IsTeamFavorite(team1))
		{
			char buf[256];
			sprintf_s(buf, sizeof(buf), "xtell 177 Monkey teamleague game started! \"observe %s\"", player1);
			Engine_SendCommand(buf);
		}
		else if (Settings_IsTeamFavorite(team2))
		{
			char buf[256];
			sprintf_s(buf, sizeof(buf), "xtell 177 Monkey teamleague game started! \"observe %s\"", player2);
			Engine_SendCommand(buf);
		}
#endif
	}

	if (statusToSet != GAMESTATUS_Error)
	{
		int i;
		for(i = 0; i < g_dataset.teamGameCount; ++i)
		{
			TeamGame_t *teamGame = &g_dataset.teamGames[i];
			if (strcmp(section, teamGame->secton) != 0)
				continue;

			if (strcmp(team1, teamGame->team1) != 0 && strcmp(team1, teamGame->team2) != 0)
				continue;

			if (strcmp(team2, teamGame->team1) != 0 && strcmp(team2, teamGame->team2) != 0)
				continue;

			if (board > teamGame->gameCount)
				continue;

			{
				Game_t* game = &teamGame->teamGames[board-1];
				if (strcmp(player1, game->player1) != 0 && strcmp(player1, game->player2) != 0)
					continue;

				if (strcmp(player2, game->player1) != 0 && strcmp(player2, game->player2) != 0)
					continue;

				if (Dataset_IsFinishedStatus(game->status))
					continue;

				game->status = statusToSet;
				return TRUE;
			}
		}
	}
	return FALSE;
}