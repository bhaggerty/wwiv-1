/**************************************************************************/
/*                                                                        */
/*                              WWIV Version 5.0x                         */
/*             Copyright (C)1998-2007, WWIV Software Services             */
/*                                                                        */
/*    Licensed  under the  Apache License, Version  2.0 (the "License");  */
/*    you may not use this  file  except in compliance with the License.  */
/*    You may obtain a copy of the License at                             */
/*                                                                        */
/*                http://www.apache.org/licenses/LICENSE-2.0              */
/*                                                                        */
/*    Unless  required  by  applicable  law  or agreed to  in  writing,   */
/*    software  distributed  under  the  License  is  distributed on an   */
/*    "AS IS"  BASIS, WITHOUT  WARRANTIES  OR  CONDITIONS OF ANY  KIND,   */
/*    either  express  or implied.  See  the  License for  the specific   */
/*    language governing permissions and limitations under the License.   */
/*                                                                        */
/**************************************************************************/
#include "core/wfndfile.h"

#include <string>

#include <dirent.h>
#include <limits.h>
#include <iostream>
#include "core/strings.h"
#include "core/wwivassert.h"

static int dos_flag = false;
static long lTypeMask;
static const char* filespec_ptr;

#define TYPE_DIRECTORY  DT_DIR
#define TYPE_FILE DT_BLK

using std::string;

//////////////////////////////////////////////////////////////////////////////
// Local function prototypes
char *getdir_from_file(const char *pszFileName);
int fname_ok(const struct dirent *ent);
char *strip_filename(const char *pszFileName);


bool WFindFile::open(const string& filespec, unsigned int nTypeMask) {
  char szFileName[PATH_MAX];
  char szDirectoryName[PATH_MAX];
  char szFileSpec[PATH_MAX+1];
  unsigned int i, f, laststar;
  memset(szFileSpec, 0, PATH_MAX + 1);

  __open(filespec, nTypeMask);
  dos_flag = 0;

  strcpy(szDirectoryName, getdir_from_file(filespec.c_str()));
  strcpy(szFileName, strip_filename(filespec.c_str()));

  if (wwiv::strings::IsEquals(szFileName, "*.*")  ||
      wwiv::strings::IsEquals(szFileName, "*")) {
    memset(szFileSpec, '?', PATH_MAX);
  } else {
    f = laststar = szFileSpec[0] = 0;
    for (i = 0; i < strlen(szFileName); i++) {
      if (szFileName[i] == '*') {
        if (i < 8) {
          if (strchr(szFileName, '.') != NULL) {
            dos_flag = 1;
            memset(&szFileSpec[f], '?', 8 - i);
            f += 8 - i;
            while (szFileName[++i] != '.')
              ;
            i--;

            continue;
          }
        }

        do {
          if (szFileName[i] == '.' && i < strlen(szFileName)) {
            szFileSpec[f++] = '.';
            break;
          }
          szFileSpec[f++] = '?';
        } while (++i < PATH_MAX);

      } else {
        szFileSpec[f++] = szFileName[i];
      }
    }

    if (strchr(szFileSpec, '.') == NULL && f < PATH_MAX) {
      memset(&szFileSpec[f], '?', PATH_MAX - f);
    }

    if (strstr(szFileName, ".*") != NULL && dos_flag) {
      memset(&szFileSpec[9], '?', 3);
    }

    if (strlen(szFileSpec) < PATH_MAX && !dos_flag) {
      memset(&szFileSpec[f], 32, PATH_MAX - f);
    }

  }
  szFileSpec[PATH_MAX] = 0;

  if (dos_flag) {
    szFileSpec[12] = 0;
  }

  filespec_.assign(szFileSpec);
  filespec_ptr = filespec_.c_str();
  filename_.assign(szFileName);

  nMatches = scandir(szDirectoryName, &entries, fname_ok, alphasort);
  if (nMatches < 0) {
    std::cout << "could not open dir '" << szDirectoryName << "'\r\n";
    perror("scandir");
    return false;
  }
  nCurrentEntry = 0;

  next();
  return (nMatches > 0);
}

bool WFindFile::next() {
  if (nCurrentEntry >= nMatches) {
    return false;
  }
  struct dirent *entry = entries[nCurrentEntry++];

  filename_.assign(entry->d_name);
  lFileSize = entry->d_reclen;
  nFileType = entry->d_type;

  return true;
}

bool WFindFile::close() {
  __close();
  return true;
}

bool WFindFile::IsDirectory() {
  if (nCurrentEntry > nMatches) {
    return false;
  }

  return (nFileType & DT_DIR);
}

bool WFindFile::IsFile() {
  if (nCurrentEntry > nMatches) {
    return false;
  }

  return (nFileType & DT_REG);
}


//////////////////////////////////////////////////////////////////////////////
//
// Local functions
//
char *getdir_from_file(const char *pszFileName) {
  static char s[256];
  int i;

  s[0] = '\0';
  for (i = strlen(pszFileName); i > -1; i--) {
    if (pszFileName[i] == '/') {
      strcpy(s, pszFileName);
      s[i] = '\0';
      break;
    }
  }

  if (!s[0]) {
    strcpy(s, "./");
  }
  return (s);
}

int fname_ok(const struct dirent *ent) {
  int ok, i;
  char f[13], *ptr = NULL, s3[13];
  // kinda a hack but there's no way to pass parameters into this easily.
  const char *s1 = filespec_ptr;
  const char *s2 = ent->d_name;

  if (wwiv::strings::IsEquals(s2, ".") ||
      wwiv::strings::IsEquals(s2, "..")) {
    return 0;
  }

  if (lTypeMask) {
    if (ent->d_type & TYPE_DIRECTORY && !(lTypeMask & WFINDFILE_DIRS)) {
      return 0;
    } else if (ent->d_type & TYPE_FILE && !(lTypeMask & WFINDFILE_FILES)) {
      return 0;
    }
  }

  ok = 1;

  f[0] = '\0';
  if (dos_flag) {
    if (strlen(s2) > 12 || s2[0] == '.') {
      return 0;
    }

    strcpy(s3, s2);
    if (strlen(s3) < 12 && (ptr = strchr(s3, '.')) != NULL) {
      *ptr = '\0';
      strcpy(f, s3);
      for (i = strlen(f); i < 8; i++) {
        f[i] = '?';
      }

      f[i] = '.';
      f[++i] = '\0';
      strcat(f, ptr + 1);

      if (strlen(f) < 12) {
        memset(&f[strlen(f)], 32, 12 - strlen(f));
      }

      f[12] = '\0';
    } else {
      if (ptr == NULL) {
        return 0;
      }
    }
  }

  if (!dos_flag) {
    for (i = 0; i < PATH_MAX && ok; i++) {
      if ((s1[i] != s2[i]) && (s1[i] != '?') && (s2[i] != '?')) {
        ok = 0;
      }
    }
  } else {
    for (i = 0; i < 12 && ok; i++) {
      if (s1[i] != f[i]) {
        if (s1[i] != '?') {
          ok = 0;
        }
      }
    }
  }

  return ok;
}

char *strip_filename(const char *pszFileName) {
  WWIV_ASSERT(pszFileName);
  static char szStaticFileName[15];
  char szTempFileName[PATH_MAX];

  int nSepIndex = -1;
  for (int i = 0; i < strlen(pszFileName); i++) {
    if (pszFileName[i] == '\\' || pszFileName[i] == ':' || pszFileName[i] == '/') {
      nSepIndex = i;
    }
  }
  if (nSepIndex != -1) {
    strcpy(szTempFileName, &(pszFileName[nSepIndex + 1]));
  } else {
    strcpy(szTempFileName, pszFileName);
  }
  for (int i1 = 0; i1 < strlen(szTempFileName); i1++) {
    if (szTempFileName[i1] >= 'A' && szTempFileName[i1] <= 'Z') {
      szTempFileName[i1] = szTempFileName[i1] - 'A' + 'a';
    }
  }
  int j = 0;
  while (szTempFileName[j] != 0) {
    if (szTempFileName[j] == 32) {
      strcpy(&szTempFileName[j], &szTempFileName[j + 1]);
    } else {
      ++j;
    }
  }
  strcpy(szStaticFileName, szTempFileName);
  return szStaticFileName;
}
