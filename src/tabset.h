#ifndef __TABSET_INCLUDED__
#define __TABSET_INCLUDED__

#include <Windows.h>
#include "status.h"
#include "config.h"

typedef enum _TabEvent_t
{
	TABEVENT_INIT,
	TABEVENT_DESTROY,
	TABEVENT_ONLINE,
	TABEVENT_OFFLINE,
	TABEVENT_DATA_UPDATING,
	TABEVENT_DATA_UPDATED,
	TABEVENT_DATA_UPDATE_REQUESTED,
	TABEVENT_DATA_UPDATE_ERROR,
	TABEVENT_CONFIG_CHANGED,
	TABEVENT_MINUTE_CHANGED,
} TabEvent_t;

typedef Status_t (*Tab_Handler_Create_t)(HWND parent, HWND* child);
typedef Status_t (*Tab_Handler_Event_t)(TabEvent_t);
typedef Status_t (*Tab_Handler_GetTabTitle_t)(char* title, int length);

typedef struct
{
	Tab_Handler_Create_t      onCreate;
	Tab_Handler_Event_t       onEvent;
	Tab_Handler_GetTabTitle_t onGetTabTitle;
} TabHandlers_t;


typedef struct
{
	HBRUSH   brush;
	HPEN	 border;
	COLORREF fontColor;
} TabSet_ColorStyle_t;

typedef struct  
{
	TabSet_ColorStyle_t neutralCell;
	TabSet_ColorStyle_t goodCell;
	TabSet_ColorStyle_t badCell;
	TabSet_ColorStyle_t pourCell;
	TabSet_ColorStyle_t pourOrBadCell;
	TabSet_ColorStyle_t pourOrGoodCell;
	TabSet_ColorStyle_t headerCell;
	COLORREF bgColor;
	COLORREF fgColor;
	HFONT    normalFont;
	HFONT    headerFont;
} TabSet_GdiCollection_t;


const GUID* Tabset_GetGUID();
const char* Tabset_GetTitle();
HBITMAP Tabset_GetBitmap();
HWND Tabset_GetHWND();
HINSTANCE Tabset_GetHInstance();
const TabSet_GdiCollection_t * Tabset_GetGdiCollection();
void Tabset_Destroy();
void Tabset_SelectStyle(HDC hdc, const TabSet_ColorStyle_t* style, TabSet_ColorStyle_t* styleToStore);

Status_t Tabset_init(HWND parent, HINSTANCE hInstance, COLORREF bgColor, COLORREF testColor);
Status_t Tabset_Event(TabEvent_t event);

#endif