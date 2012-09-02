#include <utility>
#include <sstream>
#include "dataset.h"
#include <stdio.h>
#include "settings.h"
#include "engine.h"

namespace {
static const std::string kBotQtellIdentifier = ":*TeamLeague";
static const std::string kBotQtellUpdate     = "pairing:";
static const std::string kBotQtellControl    = "pairings";
static const std::string kBotQtellStart      = "start.";
static const std::string kBotQtellFinish     = "done.";
}  // Anonymous namespace

Dataset::Dataset()
  : data_is_valid_(false), update_is_in_progress_(false) {
}

std::string Dataset::OutcomeToStr(Game_Status_t status) {
  switch (status) {
	case GAMESTATUS_NotScheduled:              return "-";
	case GAMESTATUS_Pending:                   return "+";
	case GAMESTATUS_InProgress:                return "*";
	case GAMESTATUS_Adjourned:                 return "ADJ";
	case GAMESTATUS_Win_1_0:                   return "1-0";
	case GAMESTATUS_Win_i_o:                   return "i-o";
	case GAMESTATUS_Win_plus_minus:		         return "+:-";
	case GAMESTATUS_Lose_0_1:                  return "0-1";
	case GAMESTATUS_Lose_o_i:                  return "o-i";
	case GAMESTATUS_Lose_minus_plus:           return "-:+";
	case GAMESTATUS_Draw_12_12:                return "1/2-1/2";
	case GAMESTATUS_Draw_i2_i2:                return "i/2-i/2";
	case GAMESTATUS_DoubleForfeit_o_o:         return "o-o";
	case GAMESTATUS_DoubleForfeit_minus_minus: return "-:-";
	case GAMESTATUS_Error:                     return "?";
	}
	return "???";
}

Status_t Dataset::SetOutcome(Game_t *game, const std::string& outcome) {
#define OUTCOME_OPTION(d_strval, d_status) if (outcome == d_strval) { game->set_status(d_status); return STATUS_OK; }

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

 Game_Status_t Dataset::ReversedStatus(Game_Status_t status) {
  switch(status) {
    case GAMESTATUS_Lose_minus_plus: return GAMESTATUS_Win_plus_minus;
    case GAMESTATUS_Lose_0_1:		 return GAMESTATUS_Win_1_0;
    case GAMESTATUS_Lose_o_i:        return GAMESTATUS_Win_i_o;
    case GAMESTATUS_Win_plus_minus:	 return	GAMESTATUS_Lose_minus_plus;
    case GAMESTATUS_Win_1_0:         return GAMESTATUS_Lose_0_1;
    case GAMESTATUS_Win_i_o:         return GAMESTATUS_Lose_o_i;
  }
  return status;
}

 void Dataset::ReverseGame(Game_t* game, bool reverse_color) {
   if (reverse_color)
	 game->set_is_player1_white(!game->is_player1_white());
    game->set_status(ReversedStatus(game->status()));
    std::swap((*game->mutable_player1()), (*game->mutable_player2()));
 }

 void Dataset::ReverseTeamGame(TeamGame_t* teamGame) {
   std::swap((*teamGame->mutable_team1()), (*teamGame->mutable_team2()));
   for(int i = 0; i < teamGame->team_games_size(); ++i) {
	 ReverseGame(teamGame->mutable_team_games(i));
   }
 }

Status_t Dataset::ProcessStart() {
	if (update_is_in_progress_) {
		return STATUS_ERR_GAME_UPDATE_IN_PROGRESS;
	}
    update_is_in_progress_ = true;
    updating_season_.Clear();
	return STATUS_OK_GAME_UPDATE_STARTED;
}

Status_t Dataset::ProcessUpdate(std::istream& stream) {
	if (!update_is_in_progress_) {
		return STATUS_ERR_GAME_UPDATE_NOT_IN_PROGRESS;
	}

	TeamGame_t* team_game = updating_season_.add_team_games();

  {
    std::string season;
    std::string section;
    int round;
    std::string team1;
    std::string team2;
    if (!(stream >> season >> section >> round >> team1 >> team2))
      return STATUS_ERR_BOT_SYNTAX_ERROR;
    // Ignoring season for now.
    team_game->set_section(section);
    team_game->set_round(round);
    team_game->set_team1(team1);
    team_game->set_team2(team2);
  }

  int boardnum;
  int game_id;
  std::string player1;
  std::string player2;
  std::string outcome;
  unsigned int unix_time;
	while(stream >> boardnum >> game_id >> player1 >> player2 >> outcome >> unix_time) {
		long long ll = unix_time * 10000000LL + 116444736000000000LL;

    Game_t* game = team_game->add_team_games();
    game->set_game_id(game_id);
    game->set_player1(player1);
    game->set_player2(player2);
    game->set_scheduled_time(ll);

    game->set_is_player1_white(boardnum%2 != 0);
		if (!game->is_player1_white()) {
      ReverseGame(game, false);
		}
    if (SetOutcome(game, outcome) != STATUS_OK) {
      Abort();
      return STATUS_ERR_BOT_SYNTAX_ERROR;
    }
	}
	return STATUS_OK;
}

Status_t Dataset::ProcessFinish() {
	if (!update_is_in_progress_) {
		return STATUS_ERR_GAME_UPDATE_NOT_IN_PROGRESS;
	}
	if (updating_season_.team_games_size() == 0) {
    update_is_in_progress_ = false;
    data_is_valid_ = false;
		return STATUS_OK_EMPTY_DATASET;
	}

  update_is_in_progress_ = false;
  data_is_valid_ = true;
  season_ = updating_season_;
  updating_season_.Clear();

	return STATUS_OK_GAME_UPDATE_COMPLETED;
}

Status_t Dataset::Abort() {
	if (!update_is_in_progress_)	{
		return STATUS_ERR_GAME_UPDATE_NOT_IN_PROGRESS;
	}
  update_is_in_progress_ = false;
  updating_season_.Clear();
	return STATUS_OK;
}

Status_t Dataset::ProcessDataChunk(const std::string& chunk) {
  std::istringstream stream(chunk);
  std::string identifier;
  std::string command;

  if (!(stream >> identifier >> command) || identifier != kBotQtellIdentifier) {
    return STATUS_ERR_BOT_SYNTAX_ERROR;
  }

	if (command == kBotQtellUpdate)	{
		return ProcessUpdate(stream);
	}	else if (command == kBotQtellControl)	{
    std::string subcommand;
    if (!(stream >> subcommand)) {
      return STATUS_ERR_BOT_SYNTAX_ERROR;
    }

    if (subcommand == kBotQtellStart) {
      return ProcessStart();
    }
    if (subcommand == kBotQtellFinish) {
      return ProcessFinish();
    }
	}

	return STATUS_ERR_BOT_SYNTAX_ERROR;
}

bool Dataset::IsStatusFinished(Game_Status_t status) {
	switch(status)	{
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

bool Dataset::ReadPlayerAndTeam(const std::string& player_and_team, std::string* player, std::string* team) {
  size_t opening_par_pos = player_and_team.find('(');
  size_t closing_par_pos = player_and_team.find(')');
  if (closing_par_pos != player_and_team.size() - 1)
    return false;
  if (opening_par_pos == player_and_team.npos)
    return false;
  player->assign(player_and_team.begin(), player_and_team.begin()+opening_par_pos);
  team->assign(player_and_team.begin()+opening_par_pos+1, player_and_team.begin()+closing_par_pos);
  return true;
}

bool Dataset::ProcessHotData(const std::string& msg, const Settings& settings) {
  const int kMaxStringSize = 512;
	char team1[kMaxStringSize];
	char team2[kMaxStringSize];
	char player1[kMaxStringSize];
	char player2[kMaxStringSize];
	char section[kMaxStringSize];
	int board;
	char outcome[16];
	Game_Status_t statusToSet = GAMESTATUS_Error; 

	if (data_is_valid_)	{
		return FALSE;
	}

	// 	[15:39]TeamLeague(TD): Buriag(RainbowWarriors1800) vs. rederik(queens_crusaders1800) -- Section U1800a, board 3 has ended as: 1-0.
	if (sscanf_s(msg.c_str(), "%[^(](%[^)]) vs. %[^(](%[^)]) -- Section %[^,], board %d has ended as: %[^.]",
		         player1, sizeof(player1), team1, sizeof(team1), player2, sizeof(player2), team2, sizeof(team2), 
				 section, sizeof(section), &board, outcome, sizeof(outcome)) == 7) {
		if (strcmp(outcome, "ADJ") == 0) {
			statusToSet = GAMESTATUS_Adjourned;
		}	else if (strcmp(outcome, "1/2-1/2") == 0)	{
			statusToSet = GAMESTATUS_Draw_12_12;
		}	else if (strcmp(outcome, "1-0") == 0)	{
			if (board % 2) {
				statusToSet = GAMESTATUS_Win_1_0;
			}	else {
				statusToSet = GAMESTATUS_Lose_0_1;
			}
		}	else if (strcmp(outcome, "0-1") == 0)	{
			if (board % 2) {
				statusToSet = GAMESTATUS_Lose_0_1;
			}	else {
				statusToSet = GAMESTATUS_Win_1_0;
			}
		}
	} else if (sscanf_s(msg.c_str(), "Game started: %[^(](%[^)]) vs. %[^(](%[^)]) -- Section %[^,], board %d",
    // TeamLeague(TD): Game started: Mapleleaf(CurrentAffairs) vs. XFrame(MonkeyClub2000) -- Section U2000a, board 2. To watch it "observe 208".
		player1, sizeof(player1), team1, sizeof(team1), player2, sizeof(player2), team2, sizeof(team2), 
		section, sizeof(section), &board) == 6) {
		statusToSet = GAMESTATUS_InProgress;

		if (settings.settings().playsoundwhengamestarts() && 
			(!settings.settings().playsoundonlyforfavorites()
          || settings.IsTeamFavorite(team1)
          || settings.IsTeamFavorite(team2)))	{
// MOOMOOMOOMOOMOO Level TODO. DOn't release with this commented out!
			PlaySoundA(settings.settings().playsoundfilename().c_str(), NULL, SND_ASYNC | SND_FILENAME);
		}
	}

	if (statusToSet != GAMESTATUS_Error) {
		for(int i = 0; i < season_.team_games_size(); ++i) {
			TeamGame_t *teamGame = season_.mutable_team_games(i);
			if (teamGame->section() != section)
				continue;

			if (teamGame->team1() != team1 && teamGame->team2() != team1)
				continue;

      if (teamGame->team1() != team2 && teamGame->team2() != team2)
				continue;

			if (board > teamGame->team_games_size())
				continue;

			{
				Game_t* game = teamGame->mutable_team_games(board-1);
				if (game->player1() != player1 && game->player2() != player1)
					continue;

        if (game->player1() != player2 && game->player2() != player2)
					continue;

				if (IsStatusFinished(game->status()))
					continue;

				game->set_status(statusToSet);
				return TRUE;
			}
		}
	}
	return FALSE;
}