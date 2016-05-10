#pragma once

#include "../../../Common/MyString.h"
#include "../../Common/MethodId.h"
#include "../../UI/Common/Property.h"

#include "7zeItem.h"

namespace NArchive {
  namespace N7ze {

    struct Group
    {
      AString name;
      struct Codec {
        AString name;
        CMethodId id;
        AString argName;
        AString argValue;

        Codec() {
          id = 0;
        }
      };
      CObjectVector<Codec> codecs;
      AStringVector globals;
      AStringVector includes;
      bool isDefault;

      Group() : isDefault(false) {}
      bool isValid() const {
        FOR_VECTOR(i, codecs)
          if (codecs[i].id == 0)
            return false;
        return true;
      }
    };

    class Config {
    public:
      int defaultGroup;
      CObjectVector<Group> groups;

      void load(const char *pszConfigFile);
      int getGroupIndex(const char *filename);
      int getGroupIndex(const wchar_t *filename);
      int getGroupIndex(const CObjectVector<CCoderInfo> *coders);
      int getGroupIndex(CMethodId methodId);
      int getDefaultGroupIndex(const char *grpName);
    };

    template <typename T>
    bool wildcmp(const T *wild, const T *str)
    {
      // Written by Jack Handy - <A href="mailto:jakkhandy@hotmail.com">jakkhandy@hotmail.com</A>
      const T *cp = NULL, *mp = NULL;

      while ((*str) && (*wild != '*')) {
        if ((toupper(*wild) != toupper(*str)) && (*wild != '?')) {
          return 0;
        }
        wild++;
        str++;
      }

      while (*str) {
        if (*wild == '*') {
          if (!*++wild) {
            return 1;
          }
          mp = wild;
          cp = str + 1;
        }
        else if ((toupper(*wild) == toupper(*str)) || (*wild == '?')) {
          wild++;
          str++;
        }
        else {
          wild = mp;
          str = cp++;
        }
      }

      while (*wild == '*') {
        wild++;
      }
      return !*wild;
    }
  }
}
