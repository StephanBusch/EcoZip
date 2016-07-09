// MyLoadMenu.h

#ifndef __MY_LOAD_MENU_H
#define __MY_LOAD_MENU_H

void MyChangeMenu(HMENU hMenu, int level, int menuIndex);

struct CFileMenu
{
  bool programMenu;
  bool readOnly;
  bool isFsFolder;
  bool allAreFiles;
  bool isAltStreamsSupported;
  int numItems;
  
  CFileMenu():
      programMenu(false),
      readOnly(false),
      isFsFolder(false),
      allAreFiles(false),
      isAltStreamsSupported(true),
      numItems(0)
    {}

  void Load(HMENU hMenu, unsigned startPos);
};

#endif
