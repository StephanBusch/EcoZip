#include "StdAfx.h"
#include "7zeGroup.h"

#include <string>
#include <sstream>
#include <fstream>
#include <regex>
#include <cctype>
#include <algorithm>
#include <locale>
#include <codecvt>

#include "../../Common/RegisterCodec.h"
#include "../../../Common/StringConvert.h"
#include "../../Common/CreateCoder.h"

extern unsigned int g_NumCodecs;
extern const CCodecInfo *g_Codecs[];

namespace NArchive {
  namespace N7ze {

    static const char *k_GLOBAL_Name = "global";
    static const char *k_DELTA_Name = "DELTA";
    static const char *k_algorithm_suffix[] = {
      "LIBBSC", "LIBBSC",
      "CSC32",  "CSC",
      NULL,     NULL,
    };
    static const char *k_Default_Group = "default";

    static
#if 0
    inline std::string trim(const std::string &s)
    {
      auto wsfront = std::find_if_not(s.begin(), s.end(), [](int c){return std::isspace(c); });
      auto wsback = std::find_if_not(s.rbegin(), s.rend(), [](int c){return std::isspace(c); }).base();
      return (wsback <= wsfront ? std::string() : std::string(wsfront, wsback));
    }
#else
    inline std::string trim(const std::string &s)
    {
      auto  wsfront = std::find_if_not(s.begin(), s.end(), [](int c){return std::isspace(c); });
      return std::string(wsfront, std::find_if_not(s.rbegin(), std::string::const_reverse_iterator(wsfront), [](int c){return std::isspace(c); }).base());
    }
#endif

    static inline std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
      std::stringstream ss(s);
      std::string item;
      while (std::getline(ss, item, delim)) {
        elems.push_back(item);
      }
      return elems;
    }

    static inline std::vector<std::string> split(const std::string &s, char delim) {
      std::vector<std::string> elems;
      split(s, delim, elems);
      return elems;
    }

//     static void replaceAll(std::string &s, const std::string &search, const std::string &replace)
//     {
// #if 0
//       for (size_t pos = 0;; pos += replace.length()) {
//         // Locate the substring to replace
//         pos = s.find(search, pos);
//         if (pos == std::string::npos) break;
//         // Replace by erasing and inserting
//         s.erase(pos, search.length());
//         s.insert(pos, replace);
//       }
// #else
//       size_t len = search.size();
//       size_t index = 0;
//       while ((index = s.find(search, index)) != std::string::npos) {
//         /* Make the replacement. */
//         s.replace(index, len, replace);
// 
//         /* Advance index forward so the next iteration doesn't pick it up as well. */
//         index += len;
//       }
// #endif
//     }

    static void cvtString2UString(std::string str, UString &ustr)
    {
#if 0
      std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
// 	      std::string narrow = converter.to_bytes(L"foo");
      std::wstring wstr = converter.from_bytes(str);
#else
      std::wstring wstr(str.begin(), str.end());
#endif
      ustr = wstr.c_str();
    }

    void Config::load(const char *pszConfigFile)
    {
      groups.Clear();

      std::smatch m;

      std::ifstream infile(pszConfigFile);
      std::string line;
      while (std::getline(infile, line))
      {
        std::istringstream iss(line);
        line = trim(line);
        char chBegin = line.at(0);
        if (chBegin == ';')
          continue;
        if (chBegin == '-') {
          // ----$exe - use BCJ2 + LZMA2 mf=bt4 fb=273 lc=4

          // Name
          if (!std::regex_match(line, m, std::regex("-+\\$([^ \t-]+)(.*)")))
            continue;
          groups.Add(Group());
          Group &curGroup = groups.Back();
          curGroup.name = (*(m.begin() + 1)).str().c_str();
          line = trim(*(m.begin() + 2));

          // Check default
          if (std::regex_match(line, m, std::regex("default([ \t-]+.*)"))) {
            curGroup.isDefault = true;
            line = trim(*(m.begin() + 1));
          }

          // parameter list
          if (!std::regex_match(line, m, std::regex("-[ \t]*use[ \t]+(.*)")))
            continue;
          line = *(m.begin() + 1);

          std::vector<std::string> codecs;
          split(line, '+', codecs);
          for (unsigned int i = 0; i < codecs.size(); i++) {
            std::vector<std::string> args;
            line = trim(codecs[i]);
            bool isFirst = true;
            bool isGlobal = false;
            curGroup.codecs.Add(Group::Codec());
            Group::Codec &curCodec = curGroup.codecs.Back();
            std::string strName;
            std::string strArgName;
            std::string strArgValue;
            while (!line.empty()) {
              std::regex_match(line, m, std::regex("([^ \t]+)(.*)"));
              std::string token = *(m.begin() + 1);
              for (;;) {
                if (isFirst) {
                  int nColon = (int)token.find(':');
                  if (nColon >= 0)
                    strName = token.substr(0, nColon);
                  else
                    strName = token;
                  strArgName = std::to_string((_Longlong)curGroup.codecs.Size() - 1);
                  if (stricmp(strName.c_str(), k_DELTA_Name) == 0 && nColon < 0)
                    strArgValue = strName + ":1";
                  else
                    strArgValue = strName;
                  isGlobal = (stricmp(strName.c_str(), k_GLOBAL_Name) == 0);
                  if (nColon < 0)
                    break;
                  token = token.substr(nColon + 1);
                }
                else if (isGlobal) {
                  curGroup.globals.Add(token.c_str());
                  break;
                }
                const char ** ppszMethod = k_algorithm_suffix;
                for (; *ppszMethod != NULL; ppszMethod++) {
                  if (stricmp(strName.c_str(), *ppszMethod++) == 0)
                    break;
                }
                if (*ppszMethod != NULL) {
                  std::vector<std::string> args;
                  split(token, ':', args);
                  for (std::vector<std::string>::const_iterator it = args.cbegin(); it != args.cend(); it++) {
                    strArgValue += ':';
                    strArgValue += *ppszMethod;
                    strArgValue += *it;
                  }
                }
                else {
                  strArgValue += ':';
                  strArgValue += token;
                }
                break;
              }
              line = trim(*(m.begin() + 2));
              isFirst = false;
            }
            if (isGlobal)
              curGroup.codecs.DeleteBack();
            else {
              curCodec.name = strName.c_str();
              curCodec.argName = strArgName.c_str();
              curCodec.argValue = strArgValue.c_str();
              for (unsigned i = 0; i < g_ExternalCodecs.Codecs.Size(); i++) {
                CCodecInfoEx &info = g_ExternalCodecs.Codecs[i];
                if (curCodec.name.IsEqualTo_Ascii_NoCase(info.Name)) {
                  curCodec.id = info.Id;
                  break;
                }
              }
              if (curGroup.name.IsEqualTo_Ascii_NoCase(k_Default_Group))
                defaultGroup = groups.Size() - 1;
            }
          }
        }
        else if (!groups.IsEmpty()) {
          Group &curGroup = groups.Back();
          if (curGroup.includes.FindInSorted(line.c_str()) < 0) {
            curGroup.includes.Add(line.c_str());
            curGroup.includes.Sort();
          }
        }
      }
    }

    int Config::getGroupIndex(const char *filename)
    {
      int nDefault = -1;
      FOR_VECTOR(i, groups) {
        const Group &group = groups[i];
        if (nDefault < 0 && group.name.IsEqualTo_Ascii_NoCase(k_Default_Group))
          nDefault = i;
        for (unsigned j = 0; j < group.includes.Size(); j++) {
          if (wildcmp((const char *)group.includes[j], filename)) {
            for (unsigned k = 0; k < group.codecs.Size(); k++) {
              if (group.codecs[k].id == 0)
                return nDefault;
            }
            return i;
          }
        }
      }
      if (nDefault < 0)
        nDefault = 0;
      return nDefault;
    }

    int Config::getGroupIndex(const wchar_t *filename)
    {
      int nDefault = -1;
      FOR_VECTOR(i, groups) {
        const Group &group = groups[i];
        if (nDefault < 0 && group.name.IsEqualTo_Ascii_NoCase(k_Default_Group))
          nDefault = i;
        for (unsigned j = 0; j < group.includes.Size(); j++) {
          if (wildcmp((const wchar_t *)GetUnicodeString(group.includes[j]), filename)) {
            for (unsigned k = 0; k < group.codecs.Size(); k++) {
              if (group.codecs[k].id == 0)
                return nDefault;
            }
            return i;
          }
        }
      }
      if (nDefault < 0)
        nDefault = 0;
      return nDefault;
    }

    int Config::getGroupIndex(const CObjectVector<CCoderInfo> *coders)
    {
      int nDefault = -1;
      FOR_VECTOR(i, groups) {
        const Group &group = groups[i];
        if (nDefault < 0 && group.name.IsEqualTo_Ascii_NoCase(k_Default_Group))
          nDefault = i;
        if (group.codecs.Size() == coders->Size()) {
          unsigned j;
          for (j = 0; j < group.codecs.Size(); j++) {
            if (group.codecs[j].id != (*coders)[j].MethodID)
              break;
          }
          if (j == group.codecs.Size())
            return i;
        }
      }
      if (nDefault < 0)
        nDefault = 0;
      return nDefault;
    }

    int Config::getGroupIndex(CMethodId methodId)
    {
      int nDefault = -1;
      FOR_VECTOR(i, groups) {
        const Group &group = groups[i];
        if (nDefault < 0 && group.name.IsEqualTo_Ascii_NoCase(k_Default_Group))
          nDefault = i;
        if (group.codecs.Size() == 1 && group.codecs[0].id == methodId) {
          return i;
        }
      }
      if (nDefault < 0)
        nDefault = 0;
      return nDefault;
    }

    int Config::getDefaultGroupIndex(const char *grpName)
    {
      FOR_VECTOR(i, groups) {
        const Group &group = groups[i];
        if (group.name.IsEqualTo_Ascii_NoCase(grpName)) {
          if (group.isDefault)
            return i;
          return -1;
        }
      }
      return -1;
    }
  }
}
