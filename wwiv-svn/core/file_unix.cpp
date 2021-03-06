/**************************************************************************/
/*                                                                        */
/*                              WWIV Version 5.0x                         */
/*             Copyright (C)1998-2015,WWIV Software Services             */
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
#include "core/file.h"

#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/mount.h>

#ifdef __linux
#include <sys/statfs.h>
#include <sys/vfs.h> 
#endif  // __linux

#include "core/wfndfile.h"
#include "core/wwivassert.h"

using std::string;

/////////////////////////////////////////////////////////////////////////////
// Constants

const int File::shareDenyReadWrite = S_IWRITE;
const int File::shareDenyWrite     = 0;
const int File::shareDenyRead      = S_IREAD;
const int File::shareDenyNone      = 0;

const int File::permReadWrite      = O_RDWR;

const char File::pathSeparatorChar = '/';
const char File::pathSeparatorString[] = "/";

const char File::separatorChar     = ':';


bool File::IsDirectory() const {
  struct stat statbuf;
  stat(full_path_name_.c_str(), &statbuf);
  return S_ISDIR(statbuf.st_mode);
}

/////////////////////////////////////////////////////////////////////////////
// Static functions

bool File::Copy(const std::string& sourceFileName, const std::string& destFileName) {
  if (sourceFileName != destFileName && File::Exists(sourceFileName) && !File::Exists(destFileName)) {
    char *pBuffer = static_cast<char *>(malloc(16400));
    if (pBuffer == nullptr) {
      return false;
    }
    int hSourceFile = open(sourceFileName.c_str(), O_RDONLY | O_BINARY);
    if (!hSourceFile) {
      free(pBuffer);
      return false;
    }

    int hDestFile = open(destFileName.c_str(), O_RDWR | O_BINARY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
    if (!hDestFile) {
      free(pBuffer);
      close(hSourceFile);
      return false;
    }

    int i = read(hSourceFile, pBuffer, 16384);

    while (i > 0) {
      write(hDestFile, pBuffer, i);
      i = read(hSourceFile, pBuffer, 16384);
    }

    hSourceFile = close(hSourceFile);
    hDestFile = close(hDestFile);
    free(pBuffer);
  }

  // I'm not sure about the logic here since you would think we should return true
  // in the last block, and false here.  This seems fishy
  return true;
}

bool File::Move(const std::string& sourceFileName, const std::string& destFileName) {
  //TODO: Atani needs to see if Rushfan buggered up this implementation
  if (Copy(sourceFileName, destFileName)) {
    return Remove(sourceFileName);
  }
  return false;
}

bool File::RealPath(const std::string& path, std::string* resolved) {
  char* result = realpath(path.c_str(), NULL);
  if (resolved == NULL) {
    resolved->assign(path);
    return false;
  }

  resolved->assign(result);
  free(result);
  return true;
}
long File::GetFreeSpaceForPath(const string& path) {
  struct statfs fs;
  if (statfs(path.c_str(), &fs)) {
    perror("freek1()");
    return 0;
  }
  return ((long) fs.f_bsize * (double) fs.f_bavail) / 1024;
}
