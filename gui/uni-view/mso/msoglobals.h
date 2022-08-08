#pragma once

#include <Windows.h>
#include <assert.h>

#define MSO_WORD 0x0002L
#define MSO_EXCEL 0x0004L
#define MSO_READONLY 0x0010L

#define VIEWFORM_TYPE_MSWORD 4 // это стиль        6
#define VIEWFORM_TYPE_MSEXCEL 8 // это стиль

#define WORDOLEOPEN_DOTFILE 1
#define WORDOLEOPEN_WORDFILE 2
#define WORDOLEOPEN_WORDDOC 3
#define WORDOLE_MAXDOCNAMELEN 512
#define WORDOLE_MAXFILEPATHLEN 512

#define WORDOLE_PRINT_VIEW 1
#define WORDOLE_WEB_VIEW 2
#define WORDOLE_PRINT_PREVIEW 3

#define SIZEOFARRAY(a) _countof(a)
#define KB (1024)
#define MB (0x100000l)

#define QSTR_TO_WSTR(_var_) reinterpret_cast<wchar_t*>(const_cast<ushort*>(_var_.utf16()))
#define QSTR_TO_BSTR(_var_) SysAllocStringLen(reinterpret_cast<const OLECHAR*>(_var_.unicode()), UINT(_var_.length()))

// The maximum possible size_t value has all bits set
#define MAX_SIZE_T ((size_t)(~size_t(0)))
#define DebugTrap() ((void)0)
#define RELEASE(ptr)                                                                                                   \
    {                                                                                                                  \
        if (ptr != nullptr) {                                                                                          \
            (ptr)->Release();                                                                                          \
            ptr = nullptr;                                                                                             \
        }                                                                                                              \
    }
#define WM_COMMAND_NOTIFYID(wParam, lParam) LOWORD(wParam)

#define BEGIN_MSG_MAP(window, ObjectType)                                                                              \
    ObjectType* _pObject = reinterpret_cast<ObjectType*>(GetWindowLong(window, 0 /*GWL_USERDATA*/));                   \
    if (!_pObject)                                                                                                     \
        return DebugTrap(), DefWindowProc(hWindow, message, wParam, lParam);                                           \
    switch (message) {
#define MSG_HANDLER(message, handler)                                                                                  \
    case message:                                                                                                      \
        return _pObject->handler(hWindow, message, wParam, lParam);
#define END_MSG_MAP() }

#define _LOGERR(fmt, ...) ((void)0)
#define _LOGINFO(fmt, ...) ((void)0)

////////////////////////////////////////////////////////////////////
// Global Variables
//

extern HANDLE v_hPrivateHeap;
extern CRITICAL_SECTION v_csecThreadSynch;
extern HICON v_icoOffDocIcon;
extern ULONG v_cLocks;
extern BOOL v_fUnicodeAPI;
extern BOOL v_fWindows2KPlus;

////////////////////////////////////////////////////////////////////
// Custom Errors - we support a very limited set of custom error messages
//
#define DSO_E_ERR_BASE              0x80041100
#define DSO_E_UNKNOWN               0x80041101   // "An unknown problem has occurred."
#define DSO_E_INVALIDPROGID         0x80041102   // "The ProgID/Template could not be found or is not associated with a COM server."
#define DSO_E_INVALIDSERVER         0x80041103   // "The associated COM server does not support ActiveX Document embedding."
#define DSO_E_COMMANDNOTSUPPORTED   0x80041104   // "The command is not supported by the document server."
#define DSO_E_DOCUMENTREADONLY      0x80041105   // "Unable to perform action because document was opened in read-only mode."
#define DSO_E_REQUIRESMSDAIPP       0x80041106   // "The Microsoft Internet Publishing Provider is not installed, so the URL document cannot be open for write access."
#define DSO_E_DOCUMENTNOTOPEN       0x80041107   // "No document is open to perform the operation requested."
#define DSO_E_INMODALSTATE          0x80041108   // "Cannot access document when in modal condition."
#define DSO_E_NOTBEENSAVED          0x80041109   // "Cannot Save file without a file path."
#define DSO_E_FRAMEHOOKFAILED       0x8004110A   // "Unable to set frame hook for the parent window."
#define DSO_E_ERR_MAX               0x8004110B

////////////////////////////////////////////////////////////////////
// Custom OLE Command IDs - we use for special tasks
//
#define OLECMDID_GETDATAFORMAT      0x7001  // 28673
#define OLECMDID_SETDATAFORMAT      0x7002  // 28674
#define OLECMDID_LOCKSERVER         0x7003  // 28675
#define OLECMDID_RESETFRAMEHOOK     0x7009  // 28681
#define OLECMDID_NOTIFYACTIVE       0x700A  // 28682

////////////////////////////////////////////////////////////////////
// Custom Window Messages (only apply to CDsoFramerControl window proc)
//
#define DSO_WM_ASYNCH_OLECOMMAND         (WM_USER + 300)
#define DSO_WM_ASYNCH_STATECHANGE        (WM_USER + 301)

#define DSO_WM_HOOK_NOTIFY_COMPACTIVE    (WM_USER + 400)
#define DSO_WM_HOOK_NOTIFY_APPACTIVATE   (WM_USER + 401)
#define DSO_WM_HOOK_NOTIFY_FOCUSCHANGE   (WM_USER + 402)
#define DSO_WM_HOOK_NOTIFY_SYNCPAINT     (WM_USER + 403)
#define DSO_WM_HOOK_NOTIFY_PALETTECHANGE (WM_USER + 404)

// State Flags for DSO_WM_ASYNCH_STATECHANGE:
#define DSO_STATE_MODAL            1
#define DSO_STATE_ACTIVATION       2
#define DSO_STATE_INTERACTIVE      3
#define DSO_STATE_RETURNFROMMODAL  4


////////////////////////////////////////////////////////////////////
// Menu Bar Items
//
#define DSO_MAX_MENUITEMS         16
#define DSO_MAX_MENUNAME_LENGTH   32

#ifndef DT_HIDEPREFIX
#define DT_HIDEPREFIX             0x00100000
#define DT_PREFIXONLY             0x00200000
#endif

#define SYNCPAINT_TIMER_ID         4
