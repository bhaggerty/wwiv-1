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

#include <limits>
#include <memory>
#include <string>

#include "bbs/automsg.h"
#include "bbs/dropfile.h"
#include "bbs/input.h"
#include "bbs/datetime.h"
#include "bbs/instmsg.h"
#include "bbs/menusupp.h"
#include "bbs/printfile.h"
#include "bbs/stuffin.h"
#include "bbs/wcomm.h"
#include "bbs/wwiv.h"
#include "bbs/wconstants.h"
#include "bbs/wstatus.h"
#include "core/inifile.h"
#include "core/os.h"
#include "core/strings.h"
#include "core/wwivassert.h"

using std::string;
using std::unique_ptr;
using wwiv::core::IniFile;
using wwiv::core::FilePath;
using wwiv::os::random_number;
using namespace wwiv::strings;

#define SECS_PER_DAY 86400L

static char g_szLastLoginDate[9];


static void CleanUserInfo() {
  if (okconf(session()->user())) {
    setuconf(CONF_SUBS, session()->user()->GetLastSubConf(), 0);
    setuconf(CONF_DIRS, session()->user()->GetLastDirConf(), 0);
  }
  if (session()->user()->GetLastSubNum() > session()->GetMaxNumberMessageAreas()) {
    session()->user()->SetLastSubNum(0);
  }
  if (session()->user()->GetLastDirNum() > session()->GetMaxNumberFileAreas()) {
    session()->user()->SetLastDirNum(0);
  }
  if (usub[session()->user()->GetLastSubNum()].subnum != -1) {
    session()->SetCurrentMessageArea(session()->user()->GetLastSubNum());
  }
  if (udir[session()->user()->GetLastDirNum()].subnum != -1) {
    session()->SetCurrentFileArea(session()->user()->GetLastDirNum());
  }

}

static bool random_screen(const char *mpfn) {
  const string dot_zero = StrCat(session()->language_dir, mpfn, ".0");
  if (File::Exists(dot_zero)) {
    int nNumberOfScreens = 0;
    for (int i = 0; i < 1000; i++) {
      const string dot_n = StrCat(session()->language_dir.c_str(), mpfn, ".", i);
      if (File::Exists(dot_n)) {
        nNumberOfScreens++;
      } else {
        break;
      }
    }
    printfile(StrCat(session()->language_dir.c_str(), mpfn, ".", random_number(nNumberOfScreens)));
    return true;
  }
  return false;
}

bool IsPhoneNumberUSAFormat(WUser *pUser) {
  string country = pUser->GetCountry();
  return (country == "USA" || country == "CAN" || country == "MEX");
}

static bool GetNetworkOnlyStatus() {
  bool network_only = true;
  if (syscfg.netlowtime != syscfg.nethightime) {
    if (syscfg.nethightime > syscfg.netlowtime) {
      if ((timer() <= (syscfg.netlowtime * SECONDS_PER_MINUTE_FLOAT))
          || (timer() >= (syscfg.nethightime * SECONDS_PER_MINUTE_FLOAT))) {
        network_only = false;
      }
    } else {
      if ((timer() <= (syscfg.netlowtime * SECONDS_PER_MINUTE_FLOAT))
          && (timer() >= (syscfg.nethightime * SECONDS_PER_MINUTE_FLOAT))) {
        network_only = false;
      }
    }
  } else {
    network_only = false;
  }
  return network_only;
}

static int GetAnsiStatusAndShowWelcomeScreen(bool network_only) {
  if (network_only) {
    return -1;
  }

  if (session()->GetCurrentSpeed().length() > 0) {
    string current_speed = session()->GetCurrentSpeed();
    StringUpperCase(&current_speed);
    bout << "CONNECT " << current_speed << "\r\n\r\n";
  }
  const string osVersion = wwiv::os::os_version_string();
  bout << "\r\nWWIV " << wwiv_version << "/" << osVersion << " " << beta_version << wwiv::endl;
  bout << "Copyright (c) 1998-2015 WWIV Software Services." << wwiv::endl;
  bout << "All Rights Reserved." << wwiv::endl;

  int ans = check_ansi();
  const string filename = StrCat(session()->language_dir, WELCOME_ANS);
  if (File::Exists(filename)) {
    bout.nl();
    if (ans > 0) {
      session()->user()->SetStatusFlag(WUser::ansi);
      session()->user()->SetStatusFlag(WUser::color);
      if (!random_screen(WELCOME_NOEXT)) {
        printfile(WELCOME_ANS);
      }
    } else if (ans == 0) {
      printfile(WELCOME_MSG);
    }
  } else {
    if (ans) {
      const string filename = StrCat(session()->language_dir.c_str(), WELCOME_NOEXT, ".0");
      if (File::Exists(filename)) {
        random_screen(WELCOME_NOEXT);
      } else {
        printfile(WELCOME_MSG);
      }
    } else {
      printfile(WELCOME_MSG);
    }
  }
  if (curatr != 7) {
    bout.ResetColors();
  }
  return ans;
}

static int ShowLoginAndGetUserNumber(bool network_only) {
  bout.nl();
  if (network_only) {
    bout << "This time is reserved for net-mail ONLY.  Please try calling back again later.\r\n";
  } else {
    bout << "Enter number or name or 'NEW'\r\n";
  }
  bout << "NN: ";
  string user_name;
  input(&user_name, 30);
  StringTrim(&user_name);

  int nUserNumber = finduser(user_name);
  if (nUserNumber == 0 && !user_name.empty()) {
    bout << "Searching...";
    bool abort = false;
    for (int i = 1; i < application()->GetStatusManager()->GetUserCount() && nUserNumber == 0 && !hangup
         && !abort; i++) {
      if (i % 25 == 0) {   // changed from 15 since computers are faster now-a-days
        bout << ".";
      }
      int nTempUserNumber = smallist[i].number;
      session()->ReadCurrentUser(nTempUserNumber);
      if (user_name[0] == session()->user()->GetRealName()[0]) {
        string temp_user_name(session()->user()->GetRealName());
        StringUpperCase(&temp_user_name);
        if (user_name == temp_user_name && !session()->user()->IsUserDeleted()) {
          bout << "|#5Do you mean " << session()->user()->GetUserNameAndNumber(i) << "? ";
          if (yesno()) {
            nUserNumber = nTempUserNumber;
          }
        }
      }
      checka(&abort);
    }
  }
  return nUserNumber;
}

bool IsPhoneRequired() {
  IniFile iniFile(FilePath(application()->GetHomeDir(), WWIV_INI), INI_TAG);
  if (iniFile.IsOpen()) {
    if (iniFile.GetBooleanValue("NEWUSER_MIN")) {
      return false;
    }
    if (!iniFile.GetBooleanValue("LOGON_PHONE")) {
      return false;
    }
  }
  return true;
}

bool VerifyPhoneNumber() {
  if (IsPhoneNumberUSAFormat(session()->user()) || !session()->user()->GetCountry()[0]) {
    string phoneNumber = input_password("PH: ###-###-", 4);

    if (phoneNumber != &session()->user()->GetVoicePhoneNumber()[8]) {
      if (phoneNumber.length() == 4 && phoneNumber[3] == '-') {
        bout << "\r\n!! Enter the LAST 4 DIGITS of your phone number ONLY !!\r\n\n";
      }
      return false;
    }
  }
  return true;
}

static bool VerifyPassword() {
  application()->UpdateTopScreen();

  string password = input_password("PW: ", 8);
  return (password == session()->user()->GetPassword());
}

static bool VerifySysopPassword() {
  string password = input_password("SY: ", 20);
  return (password == syscfg.systempw) ? true : false;
}

static void DoFailedLoginAttempt() {
  session()->user()->SetNumIllegalLogons(session()->user()->GetNumIllegalLogons() + 1);
  session()->WriteCurrentUser();
  bout << "\r\n\aILLEGAL LOGON\a\r\n\n";

  const string logline = StrCat("### ILLEGAL LOGON for ",
    session()->user()->GetUserNameAndNumber(session()->usernum), 
    " (",  ctim(timer()), ")");
  sysoplog("", false);
  sysoplog(logline, false);
  sysoplog("", false);
  session()->usernum = 0;
}

static void ExecuteWWIVNetworkRequest() {
  if (incom) {
    hangup = true;
    return;
  }

  application()->GetStatusManager()->RefreshStatusCache();
  long lTime = time(nullptr);
  if (session()->usernum == -2) {
    std::stringstream networkCommand;
    networkCommand << "network /B" << modem_speed << " /T" << lTime << " /F0";
    write_inst(INST_LOC_NET, 0, INST_FLAGS_NONE);
    ExecuteExternalProgram(networkCommand.str().c_str(), EFLAG_NONE);
    if (application()->GetInstanceNumber() != 1) {
      send_inst_cleannet();
    }
    set_net_num(0);
  }
  application()->GetStatusManager()->RefreshStatusCache();
  hangup = true;
  session()->remoteIO()->dtr(false);
  global_xx = false;
  Wait(1.0);
  session()->remoteIO()->dtr(true);
  Wait(0.1);
  cleanup_net();
  hangup = true;
}

static void LeaveBadPasswordFeedback(int ans) {
  if (ans > 0) {
    session()->user()->SetStatusFlag(WUser::ansi);
  } else {
    session()->user()->ClearStatusFlag(WUser::ansi);
  }
  bout << "|#6Too many logon attempts!!\r\n\n";
  bout << "|#9Would you like to leave Feedback to " << syscfg.sysopname << "? ";
  if (yesno()) {
    bout.nl();
    bout << "What is your NAME or HANDLE? ";
    string tempName = Input1("", 31, true, wwiv::bbs::InputMode::PROPER);
    if (!tempName.empty()) {
      bout.nl();
      session()->usernum = 1;
      sprintf(irt, "** Illegal logon feedback from %s", tempName.c_str());
      session()->user()->SetName(tempName.c_str());
      session()->user()->SetMacro(0, "");
      session()->user()->SetMacro(1, "");
      session()->user()->SetMacro(2, "");
      session()->user()->SetSl(syscfg.newusersl);
      session()->user()->SetScreenChars(80);
      if (ans > 0) {
        select_editor();
      } else {
        session()->user()->SetDefaultEditor(0);
      }
      session()->user()->SetNumEmailSent(0);
      bool bSaveAllowCC = session()->IsCarbonCopyEnabled();
      session()->SetCarbonCopyEnabled(false);
      email(1, 0, true, 0, true);
      session()->SetCarbonCopyEnabled(bSaveAllowCC);
      if (session()->user()->GetNumEmailSent() > 0) {
        ssm(1, 0, "Check your mailbox.  Someone forgot their password again!");
      }
    }
  }
  session()->usernum = 0;
  hangup = 1;
}

static void CheckCallRestrictions() {
  if (!hangup && session()->usernum > 0 && session()->user()->IsRestrictionLogon() &&
      IsEquals(date(), session()->user()->GetLastOn()) &&
      session()->user()->GetTimesOnToday() > 0) {
    bout.nl();
    bout << "|#6Sorry, you can only logon once per day.\r\n";
    hangup = true;
  }
}

static void DoCallBackVerification() {
  // TODO(rushfan): This is where we would do internet email validation.
}

void getuser() {
  write_inst(INST_LOC_GETUSER, 0, INST_FLAGS_NONE);

  bool network_only = GetNetworkOnlyStatus();
  int count = 0;
  bool ok = false;

  okmacro = false;
  session()->SetCurrentConferenceMessageArea(0);
  session()->SetCurrentConferenceFileArea(0);
  session()->SetEffectiveSl(syscfg.newusersl);
  session()->user()->SetStatus(0);

  int ans = GetAnsiStatusAndShowWelcomeScreen(network_only);
  do {
    session()->usernum = ShowLoginAndGetUserNumber(network_only);
    if (network_only && session()->usernum != -2) {
      session()->usernum = 0;
    }
    if (session()->usernum > 0) {
      session()->ReadCurrentUser();
      read_qscn(session()->usernum, qsc, false);
      if (!set_language(session()->user()->GetLanguage())) {
        session()->user()->SetLanguage(0);
        set_language(0);
      }
      int nInstanceNumber;
      if (session()->user()->GetSl() < 255 && user_online(session()->usernum, &nInstanceNumber)) {
        bout << "\r\n|#6You are already online on instance " << nInstanceNumber << "!\r\n\n";
        continue;
      }
      ok = true;
      if (guest_user) {
        logon_guest();
      } else {
        session()->SetEffectiveSl(syscfg.newusersl);
        if (!VerifyPassword()) {
          ok = false;
        }

        if ((syscfg.sysconfig & sysconfig_free_phone) == 0 && IsPhoneRequired()) {
          if (!VerifyPhoneNumber()) {
            ok = false;
          }
        }
        if (session()->user()->GetSl() == 255 && incom && ok) {
          if (!VerifySysopPassword()) {
            ok = false;
          }
        }
      }
      if (ok) {
        session()->ResetEffectiveSl();
        changedsl();
      } else {
        DoFailedLoginAttempt();
      }
    } else if (session()->usernum == 0) {
      bout.nl();
      if (!network_only) {
        bout << "|#6Unknown user.\r\n";
      }
    } else if (session()->usernum == -1) {
      write_inst(INST_LOC_NEWUSER, 0, INST_FLAGS_NONE);
      play_sdf(NEWUSER_NOEXT, false);
      newuser();
      ok = true;
    } else if (session()->usernum == -2) {  // network
      ExecuteWWIVNetworkRequest();
    }
  } while (!hangup && !ok && ++count < 3);

  if (count >= 3) {
    LeaveBadPasswordFeedback(ans);
  }

  okmacro = true;
  CheckCallRestrictions();

  if (application()->HasConfigFlag(OP_FLAGS_CALLBACK) && (session()->user()->GetCbv() & 1) == 0) {
    DoCallBackVerification();
  }
}

static void FixUserLinesAndColors() {
  if (session()->user()->GetNumExtended() > session()->max_extend_lines) {
    session()->user()->SetNumExtended(session()->max_extend_lines);
  }
  if (session()->user()->GetColor(8) == 0 || session()->user()->GetColor(9) == 0) {
    session()->user()->SetColor(8, session()->newuser_colors[8]);
    session()->user()->SetColor(9, session()->newuser_colors[9]);
  }
}

static void UpdateUserStatsForLogin() {
  strcpy(g_szLastLoginDate, date());
  if (IsEquals(g_szLastLoginDate, session()->user()->GetLastOn())) {
    session()->user()->SetTimesOnToday(session()->user()->GetTimesOnToday() + 1);
  } else {
    session()->user()->SetTimesOnToday(1);
    session()->user()->SetTimeOnToday(0.0);
    session()->user()->SetExtraTime(0.0);
    session()->user()->SetNumPostsToday(0);
    session()->user()->SetNumEmailSentToday(0);
    session()->user()->SetNumFeedbackSentToday(0);
  }
  session()->user()->SetNumLogons(session()->user()->GetNumLogons() + 1);
  session()->SetCurrentMessageArea(0);
  session()->SetNumMessagesReadThisLogon(0);
  if (udir[0].subnum == 0 && udir[1].subnum > 0) {
    session()->SetCurrentFileArea(1);
  } else {
    session()->SetCurrentFileArea(0);
  }
  if (session()->GetEffectiveSl() != 255 && !guest_user) {
    WStatus* pStatus = application()->GetStatusManager()->BeginTransaction();
    pStatus->IncrementCallerNumber();
    pStatus->IncrementNumCallsToday();
    application()->GetStatusManager()->CommitTransaction(pStatus);
  }
}

static void PrintLogonFile() {
  if (!incom) {
    return;
  }
  play_sdf(LOGON_NOEXT, false);
  if (!printfile(LOGON_NOEXT)) {
    pausescr();
  }
}

static void PrintUserSpecificFiles() {
  const WUser* user = session()->user();  // not-owned
  printfile(StringPrintf("sl%d", user->GetSl()));
  printfile(StringPrintf("dsl%d", user->GetDsl()));

  const int short_size = std::numeric_limits<uint16_t>::digits - 1;
  for (int i=0; i < short_size; i++) {
    if (user->HasArFlag(1 << i)) {
      printfile(StringPrintf("ar%c", static_cast<char>('A' + i)));
    }
  }

  for (int i=0; i < short_size; i++) {
    if (user->HasDarFlag(1 << i)) {
      printfile(StringPrintf("dar%c", static_cast<char>('A' + i)));
    }
  }
}

/**
 * Copies the next line located at pszWholeBuffer[plBufferPtr] to pszOutLine
 *
 * @param @pszOutLine The output buffer
 * @param pszWholeBuffer The original text buffer
 * @param plBufferPtr The offset into pszWholeBuffer
 * @param lBufferLength The length of pszWholeBuffer
 */
static string copy_line(char *pszWholeBuffer, long *plBufferPtr, long lBufferLength) {
  WWIV_ASSERT(pszWholeBuffer);
  WWIV_ASSERT(plBufferPtr);
  string result;

  if (*plBufferPtr >= lBufferLength) {
    return result;
  }
  long lCurrentPtr = *plBufferPtr;
  while ((pszWholeBuffer[lCurrentPtr] != '\r') && (pszWholeBuffer[lCurrentPtr] != '\n')
         && (lCurrentPtr < lBufferLength)) {
    result.push_back(pszWholeBuffer[lCurrentPtr++]);
  }
  if ((pszWholeBuffer[lCurrentPtr] == '\r') && (lCurrentPtr < lBufferLength)) {
    ++lCurrentPtr;
  }
  if ((pszWholeBuffer[lCurrentPtr] == '\n') && (lCurrentPtr < lBufferLength)) {
    ++lCurrentPtr;
  }
  *plBufferPtr = lCurrentPtr;
  return result;
}

static void UpdateLastOnFileAndUserLog() {
  unique_ptr<WStatus> pStatus(application()->GetStatusManager()->GetStatus());
  const string laston_txt_filename = StrCat(syscfg.gfilesdir, LASTON_TXT);
  long len;
  unique_ptr<char[], void (*)(void*)> ss(get_file(laston_txt_filename, &len), &std::free);
  long pos = 0;
  if (ss) {
    if (!cs()) {
      for (int i = 0; i < 4; i++) {
        copy_line(ss.get(), &pos, len);
      }
    }
    bool needs_header = true;
    do {
      const string s1 = copy_line(ss.get(), &pos, len);
      if (!s1.empty()) {
        if (needs_header) {
          bout.nl(2);
          bout << "|#1Last few callers|#7: |#0";
          bout.nl(2);
          if (application()->HasConfigFlag(OP_FLAGS_SHOW_CITY_ST) &&
              (syscfg.sysconfig & sysconfig_extended_info)) {
            bout << "|#2Number Name/Handle               Time  Date  City            ST Cty Modem    ##" << wwiv::endl;
          } else {
            bout << "|#2Number Name/Handle               Language   Time  Date  Speed                ##" << wwiv::endl;
          }
          unsigned char chLine = (okansi()) ? 205 : '=';
          bout << "|#7" << charstr(79, chLine) << wwiv::endl;
          needs_header = false;
        }
        bout << s1;
        bout.nl();
      }
    } while (pos < len);
    bout.nl(2);
    pausescr();
  }

  if (session()->GetEffectiveSl() != 255 || incom) {
    const string sysop_log_line = StringPrintf("%ld: %s %s %s   %s - %d (%u)",
        pStatus->GetCallerNumber(),
        session()->user()->GetUserNameAndNumber(session()->usernum),
        times(),
        fulldate(),
        session()->GetCurrentSpeed().c_str(),
        session()->user()->GetTimesOnToday(),
        application()->GetInstanceNumber());

    sysoplog("", false);
    sysoplog(stripcolors(sysop_log_line), false);
    sysoplog("", false);
    string remoteAddress = session()->remoteIO()->GetRemoteAddress();
    string remoteName = session()->remoteIO()->GetRemoteName();
    if (remoteAddress.length() > 0) {
      sysoplogf("CID NUM : %s", remoteAddress.c_str());
    }
    if (remoteName.length() > 0) {
      sysoplogf("CID NAME: %s", remoteName.c_str());
    }
    string log_line;
    if (application()->HasConfigFlag(OP_FLAGS_SHOW_CITY_ST) &&
        (syscfg.sysconfig & sysconfig_extended_info)) {
      log_line = StringPrintf(
          "|#1%-6ld %-25.25s %-5.5s %-5.5s %-15.15s %-2.2s %-3.3s %-8.8s %2d\r\n",
          pStatus->GetCallerNumber(),
          session()->user()->GetUserNameAndNumber(session()->usernum),
          times(),
          fulldate(),
          session()->user()->GetCity(),
          session()->user()->GetState(),
          session()->user()->GetCountry(),
          session()->GetCurrentSpeed().c_str(),
          session()->user()->GetTimesOnToday());
    } else {
      log_line = StringPrintf(
          "|#1%-6ld %-25.25s %-10.10s %-5.5s %-5.5s %-20.20s %2d\r\n",
          pStatus->GetCallerNumber(),
          session()->user()->GetUserNameAndNumber(session()->usernum),
          cur_lang_name,
          times(),
          fulldate(),
          session()->GetCurrentSpeed().c_str(),
          session()->user()->GetTimesOnToday());
    }

    if (session()->GetEffectiveSl() != 255) {
      File userLog(syscfg.gfilesdir, USER_LOG);
      if (userLog.Open(File::modeReadWrite | File::modeBinary | File::modeCreateFile)) {
        userLog.Seek(0L, File::seekEnd);
        userLog.Write(log_line);
        userLog.Close();
      }
      File lastonFile(laston_txt_filename);
      if (lastonFile.Open(File::modeReadWrite | File::modeBinary |
                          File::modeCreateFile | File::modeTruncate)) {
        if (ss) {
          // Need to ensure ss is not null here
          pos = 0;
          // skip the 1st line.
          copy_line(ss.get(), &pos, len);
          for (int i = 1; i < 8; i++) {
            const string s1 = copy_line(ss.get(), &pos, len) + "\r\n";
            lastonFile.Write(s1);
          }
        }
        lastonFile.Write(log_line);
        lastonFile.Close();
      }
    }
  }
}

static void CheckAndUpdateUserInfo() {
  fsenttoday = 0;
  if (session()->user()->GetBirthdayYear() == 0) {
    bout << "\r\nPlease enter the following information:\r\n";
    do {
      bout.nl();
      input_age(session()->user());
      bout.nl();
      bout.bprintf("%02d/%02d/%02d -- Correct? ",
          session()->user()->GetBirthdayMonth(),
          session()->user()->GetBirthdayDay(),
          session()->user()->GetBirthdayYear());
      if (!yesno()) {
        session()->user()->SetBirthdayYear(0);
      }
    } while (!hangup && session()->user()->GetBirthdayYear() == 0);
  }

  if (!session()->user()->GetRealName()[0]) {
    input_realname();
  }
  if (!(syscfg.sysconfig & sysconfig_extended_info)) {
    return;
  }
  if (!session()->user()->GetStreet()[0]) {
    input_street();
  }
  if (!session()->user()->GetCity()[0]) {
    input_city();
  }
  if (!session()->user()->GetState()[0]) {
    input_state();
  }
  if (!session()->user()->GetCountry()[0]) {
    input_country();
  }
  if (!session()->user()->GetZipcode()[0]) {
    input_zipcode();
  }
  if (!session()->user()->GetDataPhoneNumber()[0]) {
    input_dataphone();
  }
  if (session()->user()->GetComputerType() == -1) {
    input_comptype();
  }

  if (!application()->HasConfigFlag(OP_FLAGS_USER_REGISTRATION)) {
    return;
  }

  if (session()->user()->GetRegisteredDateNum() == 0) {
    return;
  }

  time_t lTime = time(nullptr);
  if ((session()->user()->GetExpiresDateNum() < static_cast<unsigned long>(lTime + 30 * SECS_PER_DAY))
      && (session()->user()->GetExpiresDateNum() > static_cast<unsigned long>(lTime + 10 * SECS_PER_DAY))) {
    bout << "Your registration expires in " <<
                       static_cast<int>((session()->user()->GetExpiresDateNum() - lTime) / SECS_PER_DAY) <<
                       "days";
  } else if ((session()->user()->GetExpiresDateNum() > static_cast<unsigned long>(lTime)) &&
             (session()->user()->GetExpiresDateNum() < static_cast<unsigned long>(lTime + 10 * SECS_PER_DAY))) {
    if (static_cast<int>((session()->user()->GetExpiresDateNum() - lTime) / static_cast<unsigned long>
                         (SECS_PER_DAY)) > 1) {
      bout << "|#6Your registration expires in " <<
                         static_cast<int>((session()->user()->GetExpiresDateNum() - lTime) / static_cast<unsigned long>
                                          (SECS_PER_DAY))
                         << " days";
    } else {
      bout << "|#6Your registration expires in " <<
                         static_cast<int>((session()->user()->GetExpiresDateNum() - lTime) / static_cast<unsigned long>(3600L))
                         << " hours.";
    }
    bout.nl(2);
    pausescr();
  }
  if (session()->user()->GetExpiresDateNum() < static_cast<unsigned long>(lTime)) {
    if (!so()) {
      if (session()->user()->GetSl() > syscfg.newusersl ||
          session()->user()->GetDsl() > syscfg.newuserdsl) {
        session()->user()->SetSl(syscfg.newusersl);
        session()->user()->SetDsl(syscfg.newuserdsl);
        session()->user()->SetExempt(0);
        ssm(1, 0, "%s%s", session()->user()->GetUserNameAndNumber(session()->usernum),
            "'s registration has expired.");
        session()->WriteCurrentUser();
        session()->ResetEffectiveSl();
        changedsl();
      }
    }
    bout << "|#6Your registration has expired.\r\n\n";
    pausescr();
  }
}

static void DisplayUserLoginInformation() {
  char s1[255];
  bout.nl();

  bout << "|#9Name/Handle|#0....... |#2" << session()->user()->GetUserNameAndNumber(
                       session()->usernum) << wwiv::endl;
  bout << "|#9Internet Address|#0.. |#2";
  if (check_inet_addr(session()->user()->GetEmailAddress())) {
    bout << session()->user()->GetEmailAddress() << wwiv::endl;
  } else if (!session()->internetEmailName.empty()) {
    bout << (session()->IsInternetUseRealNames() ? session()->user()->GetRealName() :
                           session()->user()->GetName())
                       << "<" << session()->internetFullEmailAddress << ">\r\n";
  } else {
    bout << "None.\r\n";
  }
  bout << "|#9Time allowed on|#0... |#2" << static_cast<int>((nsl() + 30) / SECONDS_PER_MINUTE_FLOAT)
       << wwiv::endl;
  if (session()->user()->GetNumIllegalLogons() > 0) {
    bout << "|#9Illegal logons|#0.... |#2" << session()->user()->GetNumIllegalLogons()
         << wwiv::endl;
  }
  if (session()->user()->GetNumMailWaiting() > 0) {
    bout << "|#9Mail waiting|#0...... |#2" << session()->user()->GetNumMailWaiting()
         << wwiv::endl;
  }
  if (session()->user()->GetTimesOnToday() == 1) {
    bout << "|#9Date last on|#0...... |#2" << session()->user()->GetLastOn() << wwiv::endl;
  } else {
    bout << "|#9Times on today|#0.... |#2" << session()->user()->GetTimesOnToday() << wwiv::endl;
  }
  bout << "|#9Sysop currently|#0... |#2";
  if (sysop2()) {
    bout << "Available\r\n";
  } else {
    bout << "NOT Available\r\n";
  }

  bout << "|#9System is|#0......... |#2WWIV " << wwiv_version << beta_version << "  " << wwiv::endl;

  /////////////////////////////////////////////////////////////////////////
  application()->GetStatusManager()->RefreshStatusCache();
  for (int i = 0; i < session()->GetMaxNetworkNumber(); i++) {
    if (net_networks[i].sysnum) {
      sprintf(s1, "|#9%s node|#0%s|#2 @%u", net_networks[i].name, charstr(13 - strlen(net_networks[i].name), '.'),
              net_networks[i].sysnum);
      if (i) {
        bout << s1;
        bout.nl();
      } else {
        int i1;
        for (i1 = strlen(s1); i1 < 26; i1++) {
          s1[i1] = ' ';
        }
        s1[i1] = '\0';
        std::unique_ptr<WStatus> pStatus(application()->GetStatusManager()->GetStatus());
        bout << s1 << "(net" << pStatus->GetNetworkVersion() << ")\r\n";
      }
    }
  }

  bout << "|#9OS|#0................ |#2" << wwiv::os::os_version_string() << wwiv::endl;
  bout << "|#9Instance|#0.......... |#2" << application()->GetInstanceNumber() << "\r\n\n";
  if (session()->user()->GetForwardUserNumber()) {
    if (session()->user()->GetForwardSystemNumber() != 0) {
      set_net_num(session()->user()->GetForwardNetNumber());
      if (!valid_system(session()->user()->GetForwardSystemNumber())) {
        session()->user()->ClearMailboxForward();
        bout << "Forwarded to unknown system; forwarding reset.\r\n";
      } else {
        bout << "Mail set to be forwarded to ";
        if (session()->GetMaxNetworkNumber() > 1) {
          bout << "#" << session()->user()->GetForwardUserNumber()
               << " @"
               << session()->user()->GetForwardSystemNumber()
               << "." << session()->GetNetworkName() << "."
               << wwiv::endl;
        } else {
          bout << "#" << session()->user()->GetForwardUserNumber() << " @"
               << session()->user()->GetForwardSystemNumber() << "." << wwiv::endl;
        }
      }
    } else {
      if (session()->user()->IsMailboxClosed()) {
        bout << "Your mailbox is closed.\r\n\n";
      } else {
        bout << "Mail set to be forwarded to #" << session()->user()->GetForwardUserNumber() <<
                           wwiv::endl;
      }
    }
  } else if (session()->user()->GetForwardSystemNumber() != 0) {
    char szInternetEmailAddress[ 255 ];
    read_inet_addr(szInternetEmailAddress, session()->usernum);
    bout << "Mail forwarded to Internet " << szInternetEmailAddress << ".\r\n";
  }
  if (session()->IsTimeOnlineLimited()) {
    bout << "\r\n|#3Your on-line time is limited by an external event.\r\n\n";
  }
}

static void LoginCheckForNewMail() {
  bout << "|#9Scanning for new mail... ";
  if (session()->user()->GetNumMailWaiting() > 0) {
    int nNumNewMessages = check_new_mail(session()->usernum);
    if (nNumNewMessages) {
      bout << "|#9You have |#2" << nNumNewMessages << "|#9 new message(s).\r\n\r\n" <<
                         "|#9Read your mail now? ";
      if (noyes()) {
        readmail(1);
      }
    } else {
      bout << "|#9You have |#2" << session()->user()->GetNumMailWaiting() <<
                         "|#9 old message(s) in your mailbox.\r\n";
    }
  } else {
    bout << " |#9No mail found.\r\n";
  }
}

static void CheckUserForVotingBooth() {
  if (!session()->user()->IsRestrictionVote() && session()->GetEffectiveSl() > syscfg.newusersl) {
    for (int i = 0; i < 20; i++) {
      if (questused[i] && session()->user()->GetVote(i) == 0) {
        bout.nl();
        bout << "|#9You haven't voted yet.\r\n";
        return;
      }
    }
  }
}

void logon() {
  if (session()->usernum < 1) {
    hangup = true;
    return;
  }
  session()->SetUserOnline(true);
  write_inst(INST_LOC_LOGON, 0, INST_FLAGS_NONE);
  get_user_ppp_addr();
  get_next_forced_event();
  bout.ResetColors();
  bout.Color(0);
  bout.cls();

  FixUserLinesAndColors();
  UpdateUserStatsForLogin();
  PrintLogonFile();
  UpdateLastOnFileAndUserLog();
  PrintUserSpecificFiles();

  read_automessage();
  timeon = timer();
  application()->UpdateTopScreen();
  bout.nl(2);
  pausescr();
  if (!syscfg.logon_cmd.empty()) {
    bout.nl();
    const string command = stuff_in(syscfg.logon_cmd, create_chain_file(), "", "", "", "");
    ExecuteExternalProgram(command, application()->GetSpawnOptions(SPAWNOPT_LOGON));
    bout.nl(2);
  }

  DisplayUserLoginInformation();

  CheckAndUpdateUserInfo();

  application()->UpdateTopScreen();
  application()->read_subs();
  rsm(session()->usernum, session()->user(), true);

  LoginCheckForNewMail();

  if (session()->user()->GetNewScanDateNumber()) {
    nscandate = session()->user()->GetNewScanDateNumber();
  } else {
    nscandate = session()->user()->GetLastOnDateNumber();
  }
  batchtime = 0.0;
  session()->numbatchdl = session()->numbatch = 0;

  CheckUserForVotingBooth();

  if ((incom || sysop1()) && session()->user()->GetSl() < 255) {
    broadcast("%s Just logged on!", session()->user()->GetName());
  }
  setiia(90);

  // New Message Scan
  if (session()->IsNewScanAtLogin()) {
    bout << "\r\n|#5Scan All Message Areas For New Messages? ";
    if (yesno()) {
      NewMsgsAllConfs();
    }
  }

  // Handle case of first conf with no subs avail
  if (usub[0].subnum == -1 && okconf(session()->user())) {
    for (session()->SetCurrentConferenceMessageArea(0); (session()->GetCurrentConferenceMessageArea() < subconfnum)
         && (uconfsub[session()->GetCurrentConferenceMessageArea()].confnum != -1);
         session()->SetCurrentConferenceMessageArea(session()->GetCurrentConferenceMessageArea() + 1)) {
      setuconf(CONF_SUBS, session()->GetCurrentConferenceMessageArea(), -1);
      if (usub[0].subnum != -1) {
        break;
      }
    }
    if (usub[0].subnum == -1) {
      session()->SetCurrentConferenceMessageArea(0);
      setuconf(CONF_SUBS, session()->GetCurrentConferenceMessageArea(), -1);
    }
  }
  g_preloaded = false;

  if (application()->HasConfigFlag(OP_FLAGS_USE_FORCESCAN)) {
    int nNextSubNumber = 0;
    if (session()->user()->GetSl() < 255) {
      forcescansub = true;
      qscan(session()->GetForcedReadSubNumber(), &nNextSubNumber);
      forcescansub = false;
    } else {
      qscan(session()->GetForcedReadSubNumber(), &nNextSubNumber);
    }
  }
  CleanUserInfo();
}

void logoff() {
  mailrec m;

  if (incom) {
    play_sdf(LOGOFF_NOEXT, false);
  }

  if (session()->usernum > 0) {
    if ((incom || sysop1()) && session()->user()->GetSl() < 255) {
      broadcast("%s Just logged off!", session()->user()->GetName());
    }
  }
  setiia(90);
  session()->remoteIO()->dtr(false);
  hangup = true;
  if (session()->usernum < 1) {
    return;
  }

  string text = "  Logged Off At ";
  text += times();
  if (session()->GetEffectiveSl() != 255 || incom) {
    sysoplog("", false);
    sysoplog(stripcolors(text.c_str()), false);
  }
  session()->user()->SetLastBaudRate(modem_speed);

  // put this back here where it belongs... (not sure why it te
  session()->user()->SetLastOn(g_szLastLoginDate);

  session()->user()->SetNumIllegalLogons(0);
  if ((timer() - timeon) < -30.0) {
    timeon -= HOURS_PER_DAY_FLOAT * SECONDS_PER_DAY_FLOAT;
  }
  double dTimeOnNow = timer() - timeon;
  session()->user()->SetTimeOn(session()->user()->GetTimeOn() + static_cast<float>(dTimeOnNow));
  session()->user()->SetTimeOnToday(session()->user()->GetTimeOnToday() + static_cast<float>
      (dTimeOnNow - extratimecall));
  WStatus* pStatus = application()->GetStatusManager()->BeginTransaction();
  int nActiveToday = pStatus->GetMinutesActiveToday();
  pStatus->SetMinutesActiveToday(nActiveToday + static_cast<unsigned short>(dTimeOnNow / MINUTES_PER_HOUR_FLOAT));
  application()->GetStatusManager()->CommitTransaction(pStatus);
  if (g_flags & g_flag_scanned_files) {
    session()->user()->SetNewScanDateNumber(session()->user()->GetLastOnDateNumber());
  }
  long lTime = time(nullptr);
  session()->user()->SetLastOnDateNumber(lTime);
  sysoplogfi(false, "Read: %lu   Time on: %u", session()->GetNumMessagesReadThisLogon(),
             static_cast<int>((timer() - timeon) / MINUTES_PER_HOUR_FLOAT));
  if (mailcheck) {
    unique_ptr<File> pFileEmail(OpenEmailFile(true));
    if (pFileEmail->IsOpen()) {
      session()->user()->SetNumMailWaiting(0);
      int t = static_cast< int >(pFileEmail->GetLength() / sizeof(mailrec));
      int r = 0;
      int w = 0;
      while (r < t) {
        pFileEmail->Seek(static_cast<long>(sizeof(mailrec)) * static_cast<long>(r), File::seekBegin);
        pFileEmail->Read(&m, sizeof(mailrec));
        if (m.tosys != 0 || m.touser != 0) {
          if (m.tosys == 0 && m.touser == session()->usernum) {
            if (session()->user()->GetNumMailWaiting() != 255) {
              session()->user()->SetNumMailWaiting(session()->user()->GetNumMailWaiting() + 1);
            }
          }
          if (r != w) {
            pFileEmail->Seek(static_cast<long>(sizeof(mailrec)) * static_cast<long>(w), File::seekBegin);
            pFileEmail->Write(&m, sizeof(mailrec));
          }
          ++w;
        }
        ++r;
      }
      if (r != w) {
        m.tosys = 0;
        m.touser = 0;
        for (int w1 = w; w1 < r; w1++) {
          pFileEmail->Seek(static_cast<long>(sizeof(mailrec)) * static_cast<long>(w1), File::seekBegin);
          pFileEmail->Write(&m, sizeof(mailrec));
        }
      }
      pFileEmail->SetLength(static_cast<long>(sizeof(mailrec)) * static_cast<long>(w));
      WStatus *pStatus = application()->GetStatusManager()->BeginTransaction();
      pStatus->IncrementFileChangedFlag(WStatus::fileChangeEmail);
      application()->GetStatusManager()->CommitTransaction(pStatus);
      pFileEmail->Close();
    }
  } else {
    // re-calculate mail waiting'
    unique_ptr<File> pFileEmail(OpenEmailFile(false));
    if (pFileEmail->IsOpen()) {
      int nTotalEmailMessages = static_cast<int>(pFileEmail->GetLength() / sizeof(mailrec));
      session()->user()->SetNumMailWaiting(0);
      for (int i = 0; i < nTotalEmailMessages; i++) {
        pFileEmail->Read(&m, sizeof(mailrec));
        if (m.tosys == 0 && m.touser == session()->usernum) {
          if (session()->user()->GetNumMailWaiting() != 255) {
            session()->user()->SetNumMailWaiting(session()->user()->GetNumMailWaiting() + 1);
          }
        }
      }
      pFileEmail->Close();
    }
  }
  if (smwcheck) {
    File smwFile(syscfg.datadir, SMW_DAT);
    if (smwFile.Open(File::modeReadWrite | File::modeBinary | File::modeCreateFile)) {
      int t = static_cast<int>(smwFile.GetLength() / sizeof(shortmsgrec));
      int r = 0;
      int w = 0;
      while (r < t) {
        shortmsgrec sm;
        smwFile.Seek(r * sizeof(shortmsgrec), File::seekBegin);
        smwFile.Read(&sm, sizeof(shortmsgrec));
        if (sm.tosys != 0 || sm.touser != 0) {
          if (sm.tosys == 0 && sm.touser == session()->usernum) {
            session()->user()->SetStatusFlag(WUser::SMW);
          }
          if (r != w) {
            smwFile.Seek(w * sizeof(shortmsgrec), File::seekBegin);
            smwFile.Write(&sm, sizeof(shortmsgrec));
          }
          ++w;
        }
        ++r;
      }
      smwFile.SetLength(w * sizeof(shortmsgrec));
      smwFile.Close();
    }
  }
  if (session()->usernum == 1) {
    fwaiting = session()->user()->GetNumMailWaiting();
  }
  session()->WriteCurrentUser();
  write_qscn(session()->usernum, qsc, false);
  remove_from_temp("*.*", syscfgovr.tempdir, false);
  remove_from_temp("*.*", syscfgovr.batchdir, false);
  if (session()->numbatch && (session()->numbatch != session()->numbatchdl)) {
    for (int i = 0; i < session()->numbatch; i++) {
      if (!batch[i].sending) {
        didnt_upload(i);
      }
    }
  }
  session()->numbatch = session()->numbatchdl = 0;
}


void logon_guest() {
  bout.nl(2);
  input_ansistat();

  printfile(GUEST_NOEXT);
  pausescr();

  string userName, reason;
  int count = 0;
  do {
    bout << "\r\n|#5Enter your real name : ";
    input(&userName, 25, true);
    bout << "\r\n|#5Purpose of your call?\r\n";
    input(&reason, 79, true);
    if (!userName.empty() && !reason.empty()) {
      break;
    }
    count++;
  } while (!hangup && count < 3);

  if (count >= 3) {
    printfile(REJECT_NOEXT);
    ssm(1, 0, "Guest Account failed to enter name and purpose");
    hangup = true;
  } else {
    ssm(1, 0, "Guest Account accessed by %s on %s for", userName.c_str(), times());
    ssm(1, 0, reason.c_str());
  }
}

