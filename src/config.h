#ifndef __CONFIG_INCLUDED__
#define __CONFIG_INCLUDED__

// Configuration defines

// #define ENABLE_TELLING_TO_CHANNEL_177
// #define SHOW_SPECIFIC_SEASON "T37"

// Version constants

#define STRINGIFY(s) #s
#define XSTRINGIFY(s) STRINGIFY(s)

#if defined(PLUGIN_VERSION)
#define PLUGIN_REVISION XSTRINGIFY(Revision PLUGIN_VERSION)
#else
#define PLUGIN_REVISION XSTRINGIFY(Revision PLUGIN_VERSION)
#endif PLUGIN_REVISION "Debug revision"

// Config constants

#define NOT_VALID31 0x7fffffff
#define CONFIG_MAX_CONFIG_PATHNAME_LENGTH 2048
#define CONFIG_CONFIG_FILENAME L"TeamLeaguePlugin.meow-meow"
#define TEAMLEAGUE_CMD(str) "xtell teamleague " str
#define CONFIG_TIMER_POLLING_INTERVAL 1000
#define CONFIG_MAX_USERNAME_LENGTH 128

// GameSet constants
#define CONFIG_MAX_SECTION_TITLE_LENGTH 64
#define CONFIG_MAX_TEAM_TITLE_LENGTH 64
#define CONFIG_MAX_NICKNAME_LENGTH 32
#define CONFIG_MAX_TEAMGAMES_PER_SEASON 768
#define CONFIG_MAX_TEAM_PAIRINGS 128
#define CONFIG_MAX_GAMES_PER_TEAMGAME 4
#define CONFIX_MAX_ROUNDS_PER_SEASON 16
#define CONFIG_MAX_TEAMGAMES_PER_ROUND 64
#define CONFIG_MAX_GAMES_PER_ROUND (CONFIG_MAX_TEAMGAMES_PER_ROUND * CONFIG_MAX_GAMES_PER_TEAMGAME)

#define CONFIG_MAX_SECTIONS_PER_SEASON 16
#define CONFIG_MAX_TEAMS_PER_SECTION 16
#define CONFIG_MAX_TEAMPAIR_GAMES_PER_SECTION 4

#define CONFIG_MAX_TEAMS_PER_SEASON (CONFIG_MAX_TEAMS_PER_SECTION*CONFIG_MAX_SECTIONS_PER_SEASON)

// DataSet constants

#define CONFIG_BOT_QTELL_IDENTIFIER			":*TeamLeague"


// #define CONFIG_BOT_QTELL_IDENTIFIER			":*TeamLeague"
// #define CONFIG_BOT_QTELL_UPDATE				"pairing:"
// #define CONFIG_BOT_QTELL_CONTROL			"pairings"
// #define CONFIG_BOT_QTELL_START				"start."
// #define CONFIG_BOT_QTELL_FINISH				"done."


// TabSet constants
#define CONFIG_PLUGIN_TAB_TITLE "TeamLeague"
#define CONFIG_MAX_TABS_COUNT 10
#define CONFIG_MAX_TAB_TITLE_SIZE 32

// Colors

#define CONFIG_COLOR_GOOD			RGB(0x99, 0xff, 0xaa)
#define CONFIG_COLOR_GOODORPOUR		RGB(0xcc, 0xff, 0x55)
#define CONFIG_COLOR_POUR			RGB(0xff, 0xff, 0xcc)
#define CONFIG_COLOR_POURORBAD		RGB(0xff, 0xdd, 0xaa)
#define CONFIG_COLOR_BAD			RGB(0xff, 0xbb, 0xbb)
#define CONFIG_COLOR_PEN            RGB(0, 0, 0)
#define CONFIG_COLOR_HEADER         RGB(0xbb, 0xbb, 0xbb)

// Tabs

#define CONFIG_TABS_SCROLLBAR_STEP 24

#define CONFIG_TABS_UPDATE_BUTTON_WIDTH 80
#define CONFIG_TABS_UPDATE_BUTTON_HEIGHT 20

#define CONFIG_TABS_COMMON_DROPDOWN_WIDTH 100
#define CONFIG_TABS_COMMON_DROPDOWN_HEIGHT 20
#define CONFIG_TABS_COMMON_TOP_MARGIN 24
#define CONFIG_TABS_COMMON_CHECKBOXLEFTPAD 6

#define CONFIG_TABS_STANDINGS_MAX_ROUNDHEIGHT 24
#define CONFIG_TABS_STANDINGS_MAX_HEADERHEIGHT 36
#define CONFIG_TABS_STANDINGS_MAX_GAMEWIDTH 60
#define CONFIG_TABS_STANDINGS_MAX_GAMENUMWIDTH 24
#define CONFIG_TABS_STANDINGS_MAX_TEAMTITLEWIDTH 300
#define CONFIG_TABS_STANDINGS_HEADER_WEIGHT 3

#define CONFIG_TABS_FINISHED_HEADERHEIGHT 36
#define CONFIG_TABS_FINISHED_PAWNWIDTH 24
#define CONFIG_TABS_FINISHED_MIN_GAMEHEIGHT 24

#define CONFIG_TABS_FINISHED_LINEHEIGHT 16
#define CONFIG_TABS_FINISHED_MIN_TEAMGAMEHEIGHT (CONFIG_TABS_FINISHED_LINEHEIGHT*2)

#define CONFIG_TABS_FINISHED_MAX_TABLEWIDTH 600
#define CONFIG_TABS_FINISHED_INTERTEAMGAME_SPACE 8


#define CONFIG_TABS_UPCOMING_HEADERHEIGHT 36
#define CONFIG_TABS_UPCOMING_LINEHEIGHT 16
#define CONFIG_TABS_UPCOMING_GAMEHEIGHT (CONFIG_TABS_UPCOMING_LINEHEIGHT*2)
#define CONFIG_TABS_UPCOMING_MAX_TABLEWIDTH 600

#define CONFIG_TABS_UPCOMING_RED		(60*30)
#define CONFIG_TABS_UPCOMING_YELLOW		0
#define CONFIG_TABS_UPCOMING_GREEN		(-60*60*3)

#define CONFIG_TABS_STD_HEADERHEIGHT 36
#define CONFIG_TABS_STD_LINEHEIGHT 16
#define CONFIG_TABS_STD_GAMEHEIGHT (CONFIG_TABS_UPCOMING_LINEHEIGHT*2)
#define CONFIG_TABS_STD_MAX_TABLEWIDTH 600

#endif