// Messages.h

#ifndef _MESSAGES_H_
#define _MESSAGES_H_

enum MyMessages
{
	// we can use WM_USER, since we have defined new window class.
	// so we don't need WM_APP.
	kShiftSelectMessage = WM_USER + 1,
	kReLoadMessage,
	kSetFocusToListView,
	kOpenItemChanged,
	kRefresh_StatusBar,
#ifdef UNDER_CE
	kRefresh_HeaderComboBox,
#endif
	kFolderChanged,
};

#endif	// _MESSAGES_H_
