#ifndef __SETTINGS_INCLUDED__
#define __SETTINGS_INCLUDED__

#include "config.h"
#include <windows.h>
#include "status.h"

typedef enum _Settings_Time_t
{
	SETTINGS_TIME_LOCAL,
	SETTINGS_TIME_FICS,
	SETTINGS_TIME_RELATIVE,
}Settings_Time_t;

typedef struct 
{
	char favoriteTeams[CONFIG_MAX_TEAMS_PER_SEASON][CONFIG_MAX_TEAM_TITLE_LENGTH];
	int favoriteTeamsCount;
	BOOL reserved1;
	Settings_Time_t gameTime;
	BOOL updateOnLogon;
	int updatePeriod;
	char curSectionTitle[CONFIG_MAX_SECTION_TITLE_LENGTH];

	int curFinishedDropDownRoundNum;
	int curRoundsNum;

	BOOL showFavoritesOnly;
} Settings_t;

void Settings_DoDialog();
Status_t Settings_Init(HINSTANCE instance);
Settings_t* Settings_GetSettings();
Status_t Settings_Store();
BOOL Settings_IsTeamFavorite(const char* team);
Status_t Settings_MakeTeamFavorite(const char* team);
Status_t Settings_MakeTeamNotFavorite(const char* team);

#endif