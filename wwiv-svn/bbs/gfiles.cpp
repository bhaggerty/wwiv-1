/**************************************************************************/
/*                                                                        */
/*                              WWIV Version 5.0x                         */
/*             Copyright (C)1998-2015, WWIV Software Services             */
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
#include <string>

#include "bbs/wwiv.h"
#include "bbs/datetime.h"
#include "bbs/instmsg.h"
#include "bbs/printfile.h"
#include "core/strings.h"
#include "core/wwivassert.h"

using std::string;
using namespace wwiv::strings;

void gfl_hdr(int which);
void list_sec(int *map, int nmap);
void list_gfiles(gfilerec * g, int nf, int sn);
void gfile_sec(int sn);
void gfiles2();
void gfiles3(int n);

char *get_file(const string& filename, long *len) {
  File file(filename);
  if (!file.Open(File::modeBinary | File::modeReadOnly)) {
    *len = 0L;
    return nullptr;
  }

  long lFileSize = file.GetLength();
  char* pszFileText = static_cast< char *>(BbsAllocA(lFileSize + 50));
  if (pszFileText == nullptr) {
    *len = 0L;
    return nullptr;
  }
  *len = static_cast< long >(file.Read(pszFileText, lFileSize));
  return pszFileText;
}

gfilerec *read_sec(int sn, int *nf) {
  gfilerec *pRecord;
  *nf = 0;

  int nSectionSize = sizeof(gfilerec) * gfilesec[sn].maxfiles;
  if ((pRecord = static_cast<gfilerec *>(BbsAllocA(nSectionSize))) == nullptr) {
    return nullptr;
  }

  const string filename = StringPrintf("%s%s.gfl", syscfg.datadir, gfilesec[sn].filename);
  File file(filename);
  if (file.Open(File::modeBinary | File::modeReadOnly)) {
    *nf = file.Read(pRecord, nSectionSize) / sizeof(gfilerec);
  }
  return pRecord;
}

void gfl_hdr(int which) {
  char s[255], s1[81], s2[81], s3[81];

  if (okansi()) {
    strcpy(s2, charstr(29, '\xC4'));
  } else {
    strcpy(s2, charstr(29, '-'));
  }
  if (which) {
    strcpy(s1, charstr(12, ' '));
    strcpy(s3, charstr(11, ' '));
  } else {
    strcpy(s1, charstr(12, ' '));
    strcpy(s3, charstr(17, ' '));
  }
  bool abort = false;
  if (okansi()) {
    if (which) {
      sprintf(s, "|#7\xDA\xC4\xC4\xC4\xC2%s\xC2\xC4\xC4\xC4\xC4\xC2\xC4\xC4\xC4\xC2%s\xC2\xC4\xC4\xC4\xC4\xBF", s2, s2);
    } else {
      sprintf(s, "|#7\xDA\xC4\xC4\xC4\xC2%s\xC4\xC4\xC4\xC4\xC4\xC2\xC4\xC4\xC4\xC2%s\xC4\xC4\xC4\xC4\xBF", s2, s2);
    }
  } else {
    if (which) {
      sprintf(s, "+---+%s+----+---+%s+----+", s2, s2);
    } else {
      sprintf(s, "+---+%s-----+---+%s----+", s2, s2);
    }
  }
  pla(s, &abort);
  bout.Color(0);
  if (okansi()) {
    if (which) {
      sprintf(s, "|#7\xB3|#2 # |#7\xB3%s|#1 Name %s|#7\xB3|#9Size|#7\xB3|#2 # |#7\xB3%s|#1 Name %s|#7\xB3|#9Size|#7\xB3",
              s1, s3, s1, s3);
    } else {
      sprintf(s, "|#7\xB3|#2 # |#7\xB3%s|#1 Name%s|#7\xB3|#2 # |#7\xB3%s|#1Name%s|#7\xB3", s1, s3, s1, s3);
    }
  } else {
    if (which) {
      sprintf(s, "| # |%sName %s|Size| # |%s Name%s|Size|", s1, s1, s1, s1);
    } else {
      sprintf(s, "| # |%s Name     %s| # |%s Name    %s|", s1, s1, s1, s1);
    }
  }
  pla(s, &abort);
  bout.Color(0);
  if (okansi()) {
    if (which) {
      sprintf(s, "|#7\xC3\xC4\xC4\xC4\xC5%s\xC5\xC4\xC4\xC4\xC4\xC5\xC4\xC4\xC4\xC5%s\xC5\xC4\xC4\xC4\xC4\xB4", s2, s2);
    } else {
      sprintf(s, "|#7\xC3\xC4\xC4\xC4\xC5%s\xC4\xC4\xC4\xC4\xC4\xC5\xC4\xC4\xC4\xC5%s\xC4\xC4\xC4\xC4\xB4", s2, s2);
    }
  } else {
    if (which) {
      sprintf(s, "+---+%s+----+---+%s+----+", s2, s2);
    } else {
      sprintf(s, "+---+%s-----+---+%s----+", s2, s2);
    }
  }
  pla(s, &abort);
  bout.Color(0);
}

void list_sec(int *map, int nmap) {
  char s[255], s1[255], s2[81], s3[81], s4[81], s5[81], s7[81];
  char lnum[5], rnum[5];

  int i2 = 0;
  bool abort = false;
  if (okansi()) {
    strcpy(s2, charstr(29, '\xC4'));
    strcpy(s3, charstr(12, '\xC4'));
    strcpy(s7, charstr(12, '\xC4'));
  } else {
    strcpy(s2, charstr(29, '-'));
    strcpy(s3, charstr(12, '-'));
    strcpy(s7, charstr(12, '-'));
  }

  bout.litebar("%s G-Files Section", syscfg.systemname);
  gfl_hdr(0);
  for (int i = 0; i < nmap && !abort && !hangup; i++) {
    sprintf(lnum, "%d", i + 1);
    strncpy(s4, gfilesec[map[i]].name, 34);
    s4[34] = '\0';
    if (i + 1 >= nmap) {
      if (okansi()) {
        sprintf(rnum, "%s", charstr(3, '\xFE'));
        sprintf(s5, "%s", charstr(29, '\xFE'));
      } else {
        sprintf(rnum, "%s", charstr(3, 'o'));
        sprintf(s5, "%s", charstr(29, 'o'));
      }
    } else {
      sprintf(rnum, "%d", i + 2);
      strncpy(s5, gfilesec[map[i + 1]].name, 29);
      s5[29] = '\0';
    }
    if (okansi()) {
      sprintf(s, "|#7\xB3|#2%3s|#7\xB3|#1%-34s|#7\xB3|#2%3s|#7\xB3|#1%-33s|#7\xB3", lnum, s4, rnum, s5);
    } else {
      sprintf(s, "|%3s|%-34s|%3s|%-33s|", lnum, s4, rnum, s5);
    }
    pla(s, &abort);
    bout.Color(0);
    i++;
    if (i2 > 10) {
      i2 = 0;
      if (okansi()) {
        sprintf(s1,
                "|#7\xC3\xC4\xC4\xC4X%s\xC4\xC4\xC4\xC4\xC4X\xC4\xC4\xC4X\xC4\xC4\xC4\xC4\xC4\xC4%s|#1\xFE|#7\xC4|#2%s|#7\xC4|#2\xFE|#7\xC4\xC4\xC4X",
                s2, s3, times());
      } else {
        sprintf(s1, "+---+%s-----+--------+%s-o-%s-o---+",
                s2, s3, times());
      }
      pla(s1, &abort);
      bout.Color(0);
      bout.nl();
      pausescr();
      gfl_hdr(1);
    }
  }
  if (!abort) {
    if (so()) {
      if (okansi()) {
        sprintf(s1, "|#7\xC3\xC4\xC4\xC4\xC1%s\xC4\xC4\xC4\xC4\xC4\xC1\xC4\xC4\xC4\xC1%s\xC4\xC4\xC4\xC4\xB4", s2, s2);
      } else {
        sprintf(s1, "+---+%s-----+---+%s----+", s2, s2);
      }
      pla(s1, &abort);
      bout.Color(0);

      if (okansi()) {
        sprintf(s1, "|#7\xB3  |#2G|#7)|#1G-File Edit%s|#7\xB3", charstr(61, ' '));
      } else {
        sprintf(s1, "|  G)G-File Edit%s|", charstr(61, ' '));
      }
      pla(s1, &abort);
      bout.Color(0);
      if (okansi()) {
        sprintf(s1,
                "|#7\xC0\xC4\xC4\xC4\xC4%s\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4%s|#1\xFE|#7\xC4|#2%s|#7\xC4|#1\xFE|#7\xC4\xC4\xC4\xD9",
                s2, s7, times());
      } else {
        sprintf(s1, "+----%s----------------%so-%s-o---+", s2, s7, times());
      }
      pla(s1, &abort);
      bout.Color(0);
    } else {
      if (okansi()) {
        sprintf(s1,
                "|#7\xC0\xC4\xC4\xC4\xC1%s\xC4\xC4\xC4\xC4\xC4\xC1\xC4\xC4\xC4\xC1\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4%s|#1\xFE|#7\xC4|#2%s|#7\xC4|#1\xFE|#7\xC4\xC4\xC4\xD9",
                s2, s3, times());
      } else {
        sprintf(s1, "+---+%s-----+---------------------+%so-%s-o---+", s2, s3, times());
      }
      pla(s1, &abort);
      bout.Color(0);
    }
  }
  bout.Color(0);
  bout.nl();
}

void list_gfiles(gfilerec* g, int nf, int sn) {
  int i, i2;
  char s[255], s1[255], s2[81], s3[81], s4[30], s5[30];
  char lnum[5], rnum[5], lsize[5], rsize[5], path_name[255];

  bool abort = false;
  bout.litebar(gfilesec[sn].name);
  i2 = 0;
  if (okansi()) {
    strcpy(s2, charstr(29, '\xC4'));
    strcpy(s3, charstr(12, '\xC4'));
  } else {
    strcpy(s2, charstr(29, '-'));
    strcpy(s3, charstr(12, '-'));
  }
  gfl_hdr(1);
  for (i = 0; i < nf && !abort && !hangup; i++) {
    i2++;
    sprintf(lnum, "%d", i + 1);
    strncpy(s4, g[i].description, 29);
    s4[29] = '\0';
    sprintf(path_name, "%s%s%c%s", syscfg.gfilesdir, gfilesec[sn].filename, File::pathSeparatorChar, g[i].filename);
    if (File::Exists(path_name)) {
      File handle(path_name);
      sprintf(lsize, "%ld""k", bytes_to_k(handle.GetLength()));
    } else {
      sprintf(lsize, "OFL");
    }
    if (i + 1 >= nf) {
      if (okansi()) {
        sprintf(rnum, "%s", charstr(3, '\xFE'));
        sprintf(s5, "%s", charstr(29, '\xFE'));
        sprintf(rsize, "%s", charstr(4, '\xFE'));
      } else {
        sprintf(rnum, "%s", charstr(3, 'o'));
        sprintf(s5, "%s", charstr(29, 'o'));
        sprintf(rsize, "%s", charstr(4, 'o'));
      }
    } else {
      sprintf(rnum, "%d", i + 2);
      strncpy(s5, g[i + 1].description, 29);
      s5[29] = '\0';
      sprintf(path_name, "%s%s%c%s", syscfg.gfilesdir, gfilesec[sn].filename,
              File::pathSeparatorChar, g[i + 1].filename);
      if (File::Exists(path_name)) {
        File handle(path_name);
        sprintf(rsize, "%ld", bytes_to_k(handle.GetLength()));
        strcat(rsize, "k");
      } else {
        sprintf(rsize, "OFL");
      }
    }
    if (okansi()) {
      sprintf(s, "|#7\xB3|#2%3s|#7\xB3|#1%-29s|#7\xB3|#2%4s|#7\xB3|#2%3s|#7\xB3|#1%-29s|#7\xB3|#2%4s|#7\xB3",
              lnum, s4, lsize, rnum, s5, rsize);
    } else {
      sprintf(s, "|%3s|%-29s|%4s|%3s|%-29s|%4s|", lnum, s4, lsize, rnum, s5, rsize);
    }
    pla(s, &abort);
    bout.Color(0);
    i++;
    if (i2 > 10) {
      i2 = 0;
      if (okansi()) {
        sprintf(s1,
                "|#7\xC3\xC4\xC4\xC4X%sX\xC4\xC4\xC4\xC4X\xC4\xC4\xC4X\xC4%s|#1\xFE|#7\xC4|#2%s|#7\xC4|#1\xFE|#7\xC4\xFE\xC4\xC4\xC4\xC4\xD9",
                s2, s3, times());
      } else {
        sprintf(s1, "+---+%s+----+---+%s-o-%s-o-+----+", s2, s3, times());
      }
      pla(s1, &abort);
      bout.Color(0);
      bout.nl();
      pausescr();
      gfl_hdr(1);
    }
  }
  if (!abort) {
    if (okansi()) {
      sprintf(s, "|#7\xC3\xC4\xC4\xC4\xC1%s\xC1\xC4\xC4\xC4\xC4\xC1\xC4\xC4\xC4\xC1%s\xC1\xC4\xC4\xC4\xC4\xB4", s2, s2);
    } else {
      sprintf(s, "+---+%s+----+---+%s+----+", s2, s2);
    }
    pla(s, &abort);
    bout.Color(0);
    if (so()) {
      if (okansi()) {
        sprintf(s1,
                "|#7\xB3 |#1A|#7)|#2Add a G-File  |#1D|#7)|#2Download a G-file  |#1E|#7)|#2Edit this section  |#1R|#7)|#2Remove a G-File |#7\xB3");
      } else {
        sprintf(s1, "| A)Add a G-File  D)Download a G-file  E)Edit this section  R)Remove a G-File |");
      }
      pla(s1, &abort);
      bout.Color(0);
    } else {
      if (okansi()) {
        sprintf(s1, "|#7\xB3  |#2D  |#1Download a G-file%s|#7\xB3", charstr(55, ' '));
      } else {
        sprintf(s1, "|  D  Download a G-file%s|", charstr(55, ' '));
      }
      pla(s1, &abort);
      bout.Color(0);
    }
  }
  if (okansi()) {
    sprintf(s1,
            "|#7\xC0\xC4\xC4\xC4\xC4%s\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4%s|#1\xFE|#7\xC4|#2%s|#7\xC4|#1\xFE|#7\xC4\xC4\xC4\xC4\xD9",
            s2, s3, times());
  } else {
    sprintf(s1, "+----%s----------------%so-%s-o----+", s2, s3, times());
  }
  pla(s1, &abort);
  bout.Color(0);
  bout.nl();
}

void gfile_sec(int sn) {
  int i, i1, i2, nf;
  char xdc[81], *ss, *ss1, szFileName[ MAX_PATH ];
  bool abort;

  gfilerec* g = read_sec(sn, &nf);
  if (g == nullptr) {
    return;
  }
  strcpy(xdc, odc);
  for (i = 0; i < 20; i++) {
    odc[i] = 0;
  }
  for (i = 1; i <= nf / 10; i++) {
    odc[i - 1] = static_cast<char>(i + '0');
  }
  list_gfiles(g, nf, sn);
  bool done = false;
  while (!done && !hangup) {
    session()->localIO()->tleft(true);
    bout << "|#9Current G|#1-|#9File Section |#1: |#5" << gfilesec[sn].name << "|#0\r\n";
    bout << "|#9Which G|#1-|#9File |#1(|#21|#1-|#2" << nf <<
                       "|#1), |#1(|#2Q|#1=|#9Quit|#1, |#2?|#1=|#9Relist|#1) : |#5";
    ss = mmkey(2);
    i = atoi(ss);
    if (IsEquals(ss, "Q")) {
      done = true;
    } else if (IsEquals(ss, "E") && so()) {
      done = true;
      gfiles3(sn);
    }
    if (IsEquals(ss, "A") && so()) {
      free(g);
      fill_sec(sn);
      g = read_sec(sn, &nf);
      if (g == nullptr) {
        return;
      }
      for (i = 0; i < 20; i++) {
        odc[i] = 0;
      }
      for (i = 1; i <= nf / 10; i++) {
        odc[i - 1] = static_cast<char>(i + '0');
      }
    } else if (IsEquals(ss, "R") && so()) {
      bout.nl();
      bout << "|#2G-file number to delete? ";
      ss1 = mmkey(2);
      i = atoi(ss1);
      if (i > 0 && i <= nf) {
        bout << "|#9Remove " << g[i - 1].description << "|#1? |#5";
        if (yesno()) {
          bout << "|#5Erase file too? ";
          if (yesno()) {
            sprintf(szFileName, "%s%s%c%s", syscfg.gfilesdir,
                    gfilesec[sn].filename, File::pathSeparatorChar, g[i - 1].filename);
            File::Remove(szFileName);
          }
          for (i1 = i; i1 < nf; i1++) {
            g[i1 - 1] = g[i1];
          }
          --nf;
          sprintf(szFileName, "%s%s.gfl", syscfg.datadir, gfilesec[sn].filename);
          File file(szFileName);
          file.Open(File::modeReadWrite | File::modeBinary | File::modeCreateFile | File::modeTruncate);
          file.Write(g, nf * sizeof(gfilerec));
          file.Close();
          bout << "\r\nDeleted.\r\n\n";
        }
      }
    } else if (IsEquals(ss, "?")) {
      list_gfiles(g, nf, sn);
    } else if (IsEquals(ss, "Q")) {
      done = true;
    } else if (i > 0 && i <= nf) {
      sprintf(szFileName, "%s%c%s", gfilesec[sn].filename, File::pathSeparatorChar, g[i - 1].filename);
      i1 = printfile(szFileName);
      session()->user()->SetNumGFilesRead(session()->user()->GetNumGFilesRead() + 1);
      if (i1 == 0) {
        sysoplogf("Read G-file '%s'", g[i - 1].filename);
      }
    } else if (IsEquals(ss, "D")) {
      bool done1 = false;
      while (!done1 && !hangup) {
        bout << "|#9Download which G|#1-|#9file |#1(|#2Q|#1=|#9Quit|#1, |#2?|#1=|#9Relist) : |#5";
        ss = mmkey(2);
        i2 = atoi(ss);
        abort = false;
        if (IsEquals(ss, "?")) {
          list_gfiles(g, nf, sn);
          bout << "|#9Current G|#1-|#9File Section |#1: |#5" << gfilesec[sn].name << wwiv::endl;
        } else if (IsEquals(ss, "Q")) {
          list_gfiles(g, nf, sn);
          done1 = true;
        } else if (!abort) {
          if (i2 > 0 && i2 <= nf) {
            sprintf(szFileName, "%s%s%c%s", syscfg.gfilesdir, gfilesec[sn].filename, File::pathSeparatorChar, g[i2 - 1].filename);
            File file(szFileName);
            if (!file.Open(File::modeReadOnly | File::modeBinary)) {
              bout << "|#6File not found : [" << file.full_pathname() << "]";
            } else {
              long lFileSize = file.GetLength();
              file.Close();
              bool sent = false;
              abort = false;
              send_file(szFileName, &sent, &abort, g[i2 - 1].filename, -1, lFileSize);
              char s1[ 255 ];
              if (sent) {
                sprintf(s1, "|#2%s |#9successfully transferred|#1.|#0\r\n", g[i2 - 1].filename);
                done1 = true;
              } else {
                sprintf(s1, "|#6\xFE |#9Error transferring |#2%s|#1.|#0", g[i2 - 1].filename);
                done1 = true;
              }
              bout.nl();
              bout << s1;
              bout.nl();
              sysoplog(s1);
            }
          } else {
            done1 = true;
          }
        }
      }
    }
  }
  free(g);
  strcpy(odc, xdc);
}

void gfiles2() {
  write_inst(INST_LOC_GFILEEDIT, 0, INST_FLAGS_ONLINE);
  sysoplog("@ Ran Gfile Edit");
  gfileedit();
  gfiles();
}

void gfiles3(int n) {
  write_inst(INST_LOC_GFILEEDIT, 0, INST_FLAGS_ONLINE);
  sysoplog("@ Ran Gfile Edit");
  modify_sec(n);
  gfile_sec(n);
}

void gfiles() {
  int* map = static_cast<int *>(BbsAllocA(session()->max_gfilesec * sizeof(int)));
  WWIV_ASSERT(map);

  bool done = false;
  int nmap = 0;
  for (int i = 0; i < 20; i++) {
    odc[i] = 0;
  }
  for (int i = 0; i < session()->num_sec; i++) {
    bool ok = true;
    if (session()->user()->GetAge() < gfilesec[i].age) {
      ok = false;
    }
    if (session()->GetEffectiveSl() < gfilesec[i].sl) {
      ok = false;
    }
    if (!session()->user()->HasArFlag(gfilesec[i].ar) && gfilesec[i].ar) {
      ok = false;
    }
    if (ok) {
      map[nmap++] = i;
      if ((nmap % 10) == 0) {
        odc[nmap / 10 - 1] = static_cast<char>('0' + (nmap / 10));
      }
    }
  }
  if (nmap == 0) {
    bout << "\r\nNo G-file sections available.\r\n\n";
    free(map);
    return;
  }
  list_sec(map, nmap);
  while (!done && !hangup) {
    session()->localIO()->tleft(true);
    bout << "|#9G|#1-|#9Files Main Menu|#0\r\n";
    bout << "|#9Which Section |#1(|#21|#1-|#2" << nmap <<
                       "|#1), |#1(|#2Q|#1=|#9Quit|#1, |#2?|#1=|#9Relist|#1) : |#5";
    char * ss = mmkey(2);
    if (IsEquals(ss, "Q")) {
      done = true;
    } else if (IsEquals(ss, "G") && so()) {
      done = true;
      gfiles2();
    } else if (IsEquals(ss, "A") && cs()) {
      bool bIsSectionFull = false;
      for (int i = 0; i < nmap && !bIsSectionFull; i++) {
        bout.nl();
        bout << "Now loading files for " << gfilesec[map[i]].name << "\r\n\n";
        bIsSectionFull = fill_sec(map[i]);
      }
    } else {
      int i = atoi(ss);
      if (i > 0 && i <= nmap) {
        gfile_sec(map[i-1]);
      }
    }
    if (!done) {
      list_sec(map, nmap);
    }
  }
  free(map);
}
