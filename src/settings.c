#include "settings.h"
#include <Windows.h>
#include "tabset.h"
#include "../resource.h"
#include <CommCtrl.h>
#include "dataset.h"
#include <stdio.h>

typedef struct 
{
	char header[16]; // "moooOoOooOOoOo\0"
	int size;
} Settings_header_t;


typedef struct  
{
	struct 
	{
		char title[CONFIG_MAX_SECTION_TITLE_LENGTH];
		HTREEITEM treeItem;
	} sections[CONFIG_MAX_SECTIONS_PER_SEASON];
	int sectionCount;

	struct 
	{
		char title[CONFIG_MAX_TEAM_TITLE_LENGTH];
		HTREEITEM treeItem;
	} teams[CONFIG_MAX_TEAMS_PER_SEASON];
	int teamsCount;

	BOOL wasValid;
	wchar_t confFileName[CONFIG_MAX_CONFIG_PATHNAME_LENGTH];
} Settings_Internal_t;

static Settings_t g_settings;
static Settings_Internal_t g_setInternal;

Status_t Settings_MakeTeamNotFavorite(const char* team)
{
	int i;
	for(i = 0; i < g_settings.favoriteTeamsCount; ++i)
	{
		if (strcmp(team, g_settings.favoriteTeams[i]) == 0)
		{
			if (i != g_settings.favoriteTeamsCount-1)
			{
				strcpy_s(g_settings.favoriteTeams[i], sizeof(g_settings.favoriteTeams[i]), g_settings.favoriteTeams[g_settings.favoriteTeamsCount-1]);
				--g_settings.favoriteTeamsCount;
				return STATUS_OK;
			}
		}
	}
	return STATUS_ERR_NO_SUCH_TEAM;
}

Status_t Settings_MakeTeamFavorite(const char* team)
{
	int i;
	for(i = 0; i < g_settings.favoriteTeamsCount; ++i)
	{
		if (strcmp(team, g_settings.favoriteTeams[i]) == 0)
		{
			return STATUS_ERR_TEAM_ALREADY_EXISTS;
		}
	}
	if (g_settings.favoriteTeamsCount == CONFIG_MAX_TEAMS_PER_SEASON)
	{
		return STATUS_ERR_NOT_ENOUGH_ROOM;
	}

	strcpy_s(g_settings.favoriteTeams[g_settings.favoriteTeamsCount++], sizeof(g_settings.favoriteTeams[0]), team);
	return STATUS_OK;
}

Settings_t* Settings_GetSettings()
{
	return &g_settings;
}

static void Settings_PopulateDefault()
{
	memset(&g_settings, 0, sizeof(g_settings));
	g_settings.favoriteTeamsCount = 0;
	g_settings.gameTime = SETTINGS_TIME_LOCAL;
	g_settings.updateOnLogon = TRUE;
	g_settings.updatePeriod = 0;
	g_settings.showFavoritesOnly = TRUE;
}

static Status_t Settings_Load()
{
	FILE* file;
	Settings_header_t header;
	int err = _wfopen_s(&file, g_setInternal.confFileName, L"rb");
	if (err != 0)
	{
		return STATUS_ERR_FILE_CANNOT_OPEN;
	}

	if (fread(&header, sizeof(header), 1, file) != 1)
	{
		fclose(file);
		return STATUS_ERR_FILE_CANNOT_READ;
	}

	if (strcmp(header.header, "moooOoOooOOoOo") != 0)
	{
		return STATUS_ERR_FILE_WRONG_FORMAT;
	}

	if (header.size > sizeof(g_settings))
		header.size = sizeof(g_settings);

	if (header.size < sizeof(g_settings))
		Settings_PopulateDefault();

	if (fread(&g_settings, header.size, 1, file) != 1)
	{
		fclose(file);
		return STATUS_ERR_FILE_CANNOT_READ;
	}

	fclose(file);
	return STATUS_OK;
}

Status_t Settings_Store()
{
	FILE* file;
	int err = _wfopen_s(&file, g_setInternal.confFileName, L"wb");
	Settings_header_t header = 
	{
		"moooOoOooOOoOo",
		sizeof(Settings_t)
	};

	if (err != 0)
	{
		return STATUS_ERR_FILE_CANNOT_OPEN;
	}

	fwrite(&header, sizeof(header), 1, file);
	fwrite(&g_settings, sizeof(g_settings), 1, file);
	fclose(file);

	return STATUS_OK;
}

BOOL Settings_IsTeamFavorite(const char* team)
{
	int i;
	for(i = 0; i < g_settings.favoriteTeamsCount; ++i)
	{
		if (strcmp(g_settings.favoriteTeams[i], team) == 0)
		{
			return TRUE;
		}
	}
	return FALSE;
}

Status_t Settings_Init(HINSTANCE instance)
{
	wchar_t* pos;
	GetModuleFileName(instance, g_setInternal.confFileName, (DWORD)(sizeof(g_setInternal.confFileName)/sizeof(wchar_t) - wcslen(CONFIG_CONFIG_FILENAME) - 2));
	pos = wcsrchr(g_setInternal.confFileName, L'\\');

	if (pos == NULL)
		pos = g_setInternal.confFileName + wcslen(g_setInternal.confFileName);

	*pos = L'\0';

	wcscpy_s(pos, sizeof(g_setInternal.confFileName)/sizeof(wchar_t)-(pos-g_setInternal.confFileName) , L"\\" CONFIG_CONFIG_FILENAME);


	if (Settings_Load() != STATUS_OK)
	{
		Settings_PopulateDefault();
	}

	return STATUS_OK;
}

static void Settings_Internal_InsertTeam(HWND hwnd, const char* team, const char* section)
{
	int i;
	HTREEITEM parent = 0;


	for(i = 0; i < g_setInternal.teamsCount; ++i)
	{
		if (strcmp(team, g_setInternal.teams[i].title) == 0)
			return;
	}

	if (i >= CONFIG_MAX_TEAMS_PER_SEASON)
		return;

	for(i = 0; i < g_setInternal.sectionCount; ++i)
	{
		if (strcmp(section, g_setInternal.sections[i].title) == 0)
		{
			parent = g_setInternal.sections[i].treeItem;
			break;
		}
	}

	if (i == g_setInternal.sectionCount)
	{
		wchar_t buf[CONFIG_MAX_SECTION_TITLE_LENGTH];
		size_t tmp;
		TVINSERTSTRUCT tvi;

		if (i >= CONFIG_MAX_SECTIONS_PER_SEASON)
			return;

		strcpy_s(g_setInternal.sections[i].title, sizeof(g_setInternal.sections[i].title), section);

		tvi.hParent = 0;
		tvi.hInsertAfter = TVI_LAST;
		tvi.itemex.mask = TVIF_TEXT;

		mbstowcs_s(&tmp, buf, sizeof(buf)/sizeof(wchar_t), section, CONFIG_MAX_SECTION_TITLE_LENGTH);
		tvi.itemex.pszText = buf;
		parent = g_setInternal.sections[i].treeItem = TreeView_InsertItem(hwnd, &tvi);

		++g_setInternal.sectionCount;
	}


	{
		wchar_t buf[CONFIG_MAX_TEAM_TITLE_LENGTH];
		size_t tmp;

		TVINSERTSTRUCT tvi;
		tvi.hParent = parent;
		tvi.hInsertAfter = TVI_LAST;
		tvi.itemex.mask = TVIF_TEXT  | TVIF_STATE;

		tvi.itemex.state =  INDEXTOSTATEIMAGEMASK((Settings_IsTeamFavorite(team) ? 2 : 1));
		tvi.itemex.stateMask = TVIS_STATEIMAGEMASK;

		mbstowcs_s(&tmp, buf, sizeof(buf)/sizeof(wchar_t), team, CONFIG_MAX_TEAM_TITLE_LENGTH);
		tvi.itemex.pszText = buf;

		strcpy_s(g_setInternal.teams[g_setInternal.teamsCount].title, sizeof(g_setInternal.teams[g_setInternal.teamsCount].title), team);
		g_setInternal.teams[g_setInternal.teamsCount].treeItem = TreeView_InsertItem(hwnd, &tvi);
		TreeView_Expand(hwnd, parent, TVE_EXPAND);

		++g_setInternal.teamsCount;
	}
}

static void Settings_UnpopulateDlg(HWND hwnd)
{
	HWND favoritesHwnd = GetDlgItem(hwnd, IDC_TREE_FAVORITES);
	int i;

	g_settings.gameTime = SETTINGS_TIME_LOCAL;
	if (IsDlgButtonChecked(hwnd, IDC_RADIO_TIME_FICS))
		g_settings.gameTime = SETTINGS_TIME_FICS;
	if (IsDlgButtonChecked(hwnd, IDC_RADIO_TIME_RELATIVE))
		g_settings.gameTime = SETTINGS_TIME_RELATIVE;

	g_settings.updateOnLogon = IsDlgButtonChecked(hwnd, IDC_CHECK_UPDATE_LOGON);
	if (IsDlgButtonChecked(hwnd, IDC_CHECK_UPDATE_EVERY))
	{
		g_settings.updatePeriod = GetDlgItemInt(hwnd, IDC_EDIT_UPDATE_INTERVAL, 0, FALSE);
	}
	else
	{
		g_settings.updatePeriod = 0;
	}

	if (g_setInternal.wasValid)
	{
		g_settings.favoriteTeamsCount = 0;
		for(i = 0; i < g_setInternal.teamsCount; ++i)
		{
			if (TreeView_GetCheckState(favoritesHwnd, g_setInternal.teams[i].treeItem))
			{
				strcpy_s(g_settings.favoriteTeams[g_settings.favoriteTeamsCount++], sizeof(g_settings.favoriteTeams[0]), g_setInternal.teams[i].title);
			}
		}
	}
}

static void Settings_PopulateDlg(HWND hwnd)
{
	int timeButton = IDC_RADIO_TIME_LOCAL;
	const Dataset_t *data = Dataset_GetDataset();
	HWND favoritesHwnd = GetDlgItem(hwnd, IDC_TREE_FAVORITES);

	SetWindowLong(favoritesHwnd, GWL_STYLE, GetWindowLong(favoritesHwnd, GWL_STYLE) | TVS_CHECKBOXES);

	switch(g_settings.gameTime)
	{
	case SETTINGS_TIME_LOCAL:
		timeButton = IDC_RADIO_TIME_LOCAL;
		break;
	case SETTINGS_TIME_FICS:
		timeButton = IDC_RADIO_TIME_FICS;
		break;
	case SETTINGS_TIME_RELATIVE:
		timeButton = IDC_RADIO_TIME_RELATIVE;
		break;
	}

	CheckRadioButton(hwnd, IDC_RADIO_TIME_LOCAL, IDC_RADIO_TIME_RELATIVE, timeButton);
	CheckDlgButton(hwnd, IDC_CHECK_UPDATE_LOGON, g_settings.updateOnLogon);
	CheckDlgButton(hwnd, IDC_CHECK_UPDATE_EVERY, g_settings.updatePeriod != 0);
	SendDlgItemMessage(hwnd, IDC_EDIT_UPDATE_INTERVAL, WM_ENABLE, (WPARAM) (g_settings.updatePeriod != 0), 0);
	if (g_settings.updatePeriod)
	{
		SetDlgItemInt(hwnd, IDC_EDIT_UPDATE_INTERVAL, g_settings.updatePeriod, FALSE);
	}
	else
	{
		SetDlgItemInt(hwnd, IDC_EDIT_UPDATE_INTERVAL, 14, FALSE);
	}

	SendDlgItemMessage(hwnd, IDC_EDIT_UPDATE_INTERVAL, WM_ENABLE, (WPARAM) (FALSE), 0);

	g_setInternal.wasValid = data->isDataValid;
	g_setInternal.teamsCount = 0;
	g_setInternal.sectionCount = 0;

	if (g_setInternal.wasValid)
	{
		int i;
		for(i = 0; i < data->teamGameCount; ++i)
		{
			Settings_Internal_InsertTeam(favoritesHwnd, data->teamGames[i].team1, data->teamGames[i].secton);
			Settings_Internal_InsertTeam(favoritesHwnd, data->teamGames[i].team2, data->teamGames[i].secton);
		}
	}
	
}

static INT_PTR CALLBACK Settings_DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	(void)lParam;
	(void)wParam;
	(void)uMsg;
	(void)hwndDlg;

	switch(uMsg)
	{
	case WM_INITDIALOG:
		Settings_PopulateDlg(hwndDlg);
		return TRUE;
	case WM_COMMAND:
		{
			int id = LOWORD(wParam);
			switch(id)
			{
			case IDOK:
				Settings_UnpopulateDlg(hwndDlg);
			case IDCANCEL:   // Fall through
				EndDialog(hwndDlg, id);
				return TRUE;
			case IDC_CHECK_UPDATE_EVERY:
				SendDlgItemMessage(hwndDlg, IDC_EDIT_UPDATE_INTERVAL, WM_ENABLE, (WPARAM)IsDlgButtonChecked(hwndDlg, IDC_CHECK_UPDATE_EVERY), 0);
				return TRUE;
			}
			return FALSE;
		}
	}

	return FALSE;
}

void Settings_DoDialog()
{
	if (IDOK == DialogBox(Tabset_GetHInstance(), MAKEINTRESOURCE(IDD_DIALOG1), Tabset_GetHWND(), Settings_DialogProc))
	{
		Settings_Store();
		Tabset_Event(TABEVENT_CONFIG_CHANGED);
	}
}