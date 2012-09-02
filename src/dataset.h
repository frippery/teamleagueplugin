#ifndef __DATASET_INCLUDED__
#define __DATASET_INCLUDED__

#include <string>
#include <iostream>
#include "config.h"
#include "status.h"
#include "settings.h"
#include <windows.h>
#pragma warning( push, 3 )
#include "gamedata.pb.h"
#pragma warning( pop )

class Dataset {
public:
  Dataset();
  Status_t ProcessDataChunk(const std::string& chunk);
  bool ProcessHotData(const std::string& msg, const Settings& settings);
  Status_t Abort();
  bool is_data_valid() const { return data_is_valid_; }
  bool is_update_in_progress() const { return update_is_in_progress_; }
  const Season_t& get_season() const { return season_; }
  
  static void ReverseTeamGame(TeamGame_t* team_game);
  static std::string OutcomeToStr(Game_Status_t status);
  static bool IsStatusFinished(Game_Status_t status);
private:
  Status_t ProcessStart();
  Status_t ProcessUpdate(std::istream& stream);
  Status_t ProcessFinish();

  static Status_t SetOutcome(Game_t* game, const std::string& outcome);
  static Game_Status_t ReversedStatus(Game_Status_t status);
  static void ReverseGame(Game_t* game, bool reverse_color = true);
  static bool ReadPlayerAndTeam(const std::string& player_and_team, std::string* player, std::string* team);
  
  bool data_is_valid_;
  bool update_is_in_progress_;
  Season_t season_;
  Season_t updating_season_;
};

#endif