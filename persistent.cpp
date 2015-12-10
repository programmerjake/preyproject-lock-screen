#define NTDDI_VERSION (0x06020000)
#define _WIN32_WINNT (0x0602)
#if defined(UNICODE) && !defined(_UNICODE)
#define _UNICODE
#elif !defined(UNICODE) && defined(_UNICODE)
#define UNICODE
#endif
#define NOMINMAX
#include <windows.h>
#include <wtsapi32.h>
#include <tchar.h>
#include <sddl.h>
#include <iostream>
#include "get_option.h"

#define REGISTRY_SETTINGS_LOCATION "Software\\Prey\\lock-screen"

#ifndef ERROR_ELEVATION_REQUIRED
#define ERROR_ELEVATION_REQUIRED (740)
#endif // ERROR_ELEVATION_REQUIRED

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);

#ifdef UNICODE
typedef std::wstring tstring;
typedef std::wostringstream tostringstream;
typedef std::wistringstream tistringstream;
typedef wget_option tget_option;
#define Tcout (::std::wcout)
#define Tcerr (::std::wcerr)
#else
typedef std::string tstring;
typedef std::ostringstream tostringstream;
typedef std::istringstream tistringstream;
typedef get_option tget_option;
#define Tcout (::std::cout)
#define Tcerr (::std::cerr)
#endif // UNICODE

bool verbose = true;


struct PersistentSystemOptions final
{
  bool hasLockComputer;
  bool hasSwitchUser;
  bool hasLogOff;
  bool hasChangePassword;
  bool hasTaskManager;
  bool hasPower;
  bool hasFilterKeysHotkey;
  bool hasMouseKeysHotkey;
  bool hasStickyKeysHotkey;
  bool hasToggleKeysHotkey;
  bool hasHighContrastHotkey;
  PersistentSystemOptions(bool enabled = true)
    : hasLockComputer(enabled),
    hasSwitchUser(enabled),
    hasLogOff(enabled),
    hasChangePassword(enabled),
    hasTaskManager(enabled),
    hasPower(enabled),
    hasFilterKeysHotkey(enabled),
    hasMouseKeysHotkey(enabled),
    hasStickyKeysHotkey(enabled),
    hasToggleKeysHotkey(enabled),
    hasHighContrastHotkey(enabled)
  {
  }
  static PersistentSystemOptions AllDisabled()
  {
    return PersistentSystemOptions(false);
  }
  static PersistentSystemOptions AllEnabled()
  {
    return PersistentSystemOptions(true);
  }
  static PersistentSystemOptions ReadSystemSettings();
  bool WriteSystemSettings() const;
  static PersistentSystemOptions ReadFromStorage();
  bool WriteToStorage() const;
  std::ostream &Dump(std::ostream &os) const
  {
    os << "hasLockComputer = " << (hasLockComputer ? "true\n" : "false\n");
    os << "hasSwitchUser = " << (hasSwitchUser ? "true\n" : "false\n");
    os << "hasLogOff = " << (hasLogOff ? "true\n" : "false\n");
    os << "hasChangePassword = " << (hasChangePassword ? "true\n" : "false\n");
    os << "hasTaskManager = " << (hasTaskManager ? "true\n" : "false\n");
    os << "hasPower = " << (hasPower ? "true\n" : "false\n");
    os << "hasFilterKeysHotkey = " << (hasFilterKeysHotkey ? "true\n" : "false\n");
    os << "hasMouseKeysHotkey = " << (hasMouseKeysHotkey ? "true\n" : "false\n");
    os << "hasStickyKeysHotkey = " << (hasStickyKeysHotkey ? "true\n" : "false\n");
    os << "hasToggleKeysHotkey = " << (hasToggleKeysHotkey ? "true\n" : "false\n");
    os << "hasHighContrastHotkey = " << (hasHighContrastHotkey ? "true\n" : "false\n");
    return os;
  }
  std::wostream &Dump(std::wostream &os) const
  {
    os << L"hasLockComputer = " << (hasLockComputer ? L"true\n" : L"false\n");
    os << L"hasSwitchUser = " << (hasSwitchUser ? L"true\n" : L"false\n");
    os << L"hasLogOff = " << (hasLogOff ? L"true\n" : L"false\n");
    os << L"hasChangePassword = " << (hasChangePassword ? L"true\n" : L"false\n");
    os << L"hasTaskManager = " << (hasTaskManager ? L"true\n" : L"false\n");
    os << L"hasPower = " << (hasPower ? L"true\n" : L"false\n");
    os << L"hasFilterKeysHotkey = " << (hasFilterKeysHotkey ? L"true\n" : L"false\n");
    os << L"hasMouseKeysHotkey = " << (hasMouseKeysHotkey ? L"true\n" : L"false\n");
    os << L"hasStickyKeysHotkey = " << (hasStickyKeysHotkey ? L"true\n" : L"false\n");
    os << L"hasToggleKeysHotkey = " << (hasToggleKeysHotkey ? L"true\n" : L"false\n");
    os << L"hasHighContrastHotkey = " << (hasHighContrastHotkey ? L"true\n" : L"false\n");
    return os;
  }
  private:
  static bool ReadRegistryKey(HKEY baseHive, tstring path, tstring name, bool defaultValue);
  static bool WriteRegistryKey(HKEY baseHive, tstring path, tstring name, bool value);
  static bool ReadFilterKeysHotkey();
  static bool WriteFilterKeysHotkey(bool value);
  static bool ReadMouseKeysHotkey();
  static bool WriteMouseKeysHotkey(bool value);
  static bool ReadStickyKeysHotkey();
  static bool WriteStickyKeysHotkey(bool value);
  static bool ReadToggleKeysHotkey();
  static bool WriteToggleKeysHotkey(bool value);
  static bool ReadHighContrastHotkey();
  static bool WriteHighContrastHotkey(bool value);
};

bool getActiveConsoleId(LPDWORD id) {
  PWTS_SESSION_INFO sessioninfo;
  DWORD count;

  if (!WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &sessioninfo, &count))
    return FALSE;

  // Ok, now find the active session

  DWORD i;
  for (i = 0; i < count; i++) {
    if (sessioninfo[i].State == WTSActive) {
      *id = sessioninfo[i].SessionId;
      break;
    }
  }

  WTSFreeMemory(&sessioninfo);
  return TRUE;
}

bool getSid(char* &string_sid) {

  DWORD sessionid;
  DWORD usersize = 0;
  char* username;

  if (!getActiveConsoleId(&sessionid)) {
    //error("WTSEnumerateSessions failed: %ld", GetLastError());
    return FALSE;
  } else {
    //info("WTSEnumerateSessions got active session: %d", sessionid);
  }

  if (!WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, sessionid, WTSUserName, &username, &usersize)) {
    //error("WTSQuerySessionInformation failed: %ld", GetLastError());
    return FALSE;
  } else {
    //info("WTSQuerySessionInformation got user: %s", username);
    return TRUE;
  }

  PSID sid = NULL;
  DWORD sid_size = 0;
  char* domain = NULL;
  DWORD domain_size = 0;
  SID_NAME_USE use;

  // First, we need to get the buffer size needed to store the SID
  if(!LookupAccountName(NULL, username, sid, &sid_size, domain, &domain_size, &use)) {
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
      //error("LookupAccountName error while getting buffer size: %ld", GetLastError());
      return FALSE;
    }
  }

  try {
    sid = (PSID) LocalAlloc(LMEM_FIXED, sid_size);
    domain = (char*) malloc(domain_size * sizeof(char));
    if ( !sid || !domain ) {
      throw(0);
    }
    if (!LookupAccountName(NULL, username, sid, &sid_size, domain, &domain_size, &use)) {
      //error("LookupAccountName error while getting buffer size: %ld", GetLastError());
      throw(0);
    }
  } catch (int) {
    LocalFree(sid);
    sid = NULL;
    //error("LookupAccountName error while getting actual data: %ld", GetLastError());
    return FALSE;
  }

  if(!ConvertSidToStringSid(sid, &string_sid)) {
    //error("Could not convert to string from SID");
    return FALSE;
  }

  return TRUE;
}

PersistentSystemOptions PersistentSystemOptions::ReadSystemSettings()
{
  char *sid;
  std::string system = "\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System";
  std::string explorer = "\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer";

  if(!getSid(sid)) {
    return false;
  }

  PersistentSystemOptions retval;
  retval.hasChangePassword = !ReadRegistryKey(HKEY_USERS, _T(sid + system), _T("DisableChangePassword"), false);
  retval.hasLockComputer = !ReadRegistryKey(HKEY_USERS, _T(sid + system), _T("DisableLockWorkstation"), false);
  retval.hasLogOff = !ReadRegistryKey(HKEY_USERS, _T(sid + explorer), _T("NoLogoff"), false);
  retval.hasSwitchUser = !ReadRegistryKey(HKEY_LOCAL_MACHINE, _T(system), _T("HideFastUserSwitching"), false);
  retval.hasTaskManager = !ReadRegistryKey(HKEY_USERS, _T(sid + system), _T("DisableTaskMgr"), false);
  retval.hasPower = !ReadRegistryKey(HKEY_USERS, _T(sid + explorer), _T("NoClose"), false);
  retval.hasFilterKeysHotkey = ReadFilterKeysHotkey();
  retval.hasMouseKeysHotkey = ReadMouseKeysHotkey();
  retval.hasStickyKeysHotkey = ReadStickyKeysHotkey();
  retval.hasToggleKeysHotkey = ReadToggleKeysHotkey();
  retval.hasHighContrastHotkey = ReadHighContrastHotkey();
  return retval;
}

bool PersistentSystemOptions::WriteSystemSettings() const
{
  bool allWorked = true;
  char* sid;
  std::string system = "\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System";
  std::string explorer = "\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer";

  if(!getSid(sid)) {
    return false;
  }

  if(!WriteRegistryKey(HKEY_USERS, _T(sid + system), _T("DisableChangePassword"), !hasChangePassword))
    allWorked = false;

  if(!WriteRegistryKey(HKEY_USERS, _T(sid + system), _T("DisableLockWorkstation"), !hasLockComputer))
    allWorked = false;

  if(!WriteRegistryKey(HKEY_USERS, _T(sid + explorer), _T("NoLogoff"), !hasLogOff))
    allWorked = false;

  if(!WriteRegistryKey(HKEY_LOCAL_MACHINE, _T(system), _T("HideFastUserSwitching"), !hasSwitchUser))
    allWorked = false;

#ifndef DEBUG

  if(!WriteRegistryKey(HKEY_USERS, _T(sid + system), _T("DisableTaskMgr"), !hasTaskManager))
    allWorked = false;

#endif // DEBUG

  if(!WriteRegistryKey(HKEY_USERS, _T(sid + explorer), _T("NoClose"), !hasPower))
    allWorked = false;

  if(!WriteFilterKeysHotkey(hasFilterKeysHotkey))
    allWorked = false;

  if(!WriteMouseKeysHotkey(hasMouseKeysHotkey))
    allWorked = false;

  if(!WriteStickyKeysHotkey(hasStickyKeysHotkey))
    allWorked = false;

  if(!WriteToggleKeysHotkey(hasToggleKeysHotkey))
    allWorked = false;

  if(!WriteHighContrastHotkey(hasHighContrastHotkey))
    allWorked = false;

  return allWorked;
}

bool PersistentSystemOptions::ReadRegistryKey(HKEY baseHive, tstring path, tstring name, bool defaultValue)
{
  HKEY key;

  if(ERROR_SUCCESS != RegOpenKeyEx(baseHive, path.c_str(), 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY | STANDARD_RIGHTS_READ, &key))
  {
    return defaultValue;
  }

  DWORD type;
  DWORD value;
  DWORD valueSize = sizeof(value);

  if(ERROR_SUCCESS != RegQueryValueEx(key, name.c_str(), NULL, &type, (LPBYTE)&value, (LPDWORD)&valueSize))
  {
    RegCloseKey(key);
    return defaultValue;
  }

  RegCloseKey(key);

  if(type != REG_DWORD)
  {
    return defaultValue;
  }

  return value != 0;
}

bool PersistentSystemOptions::WriteRegistryKey(HKEY baseHive, tstring path, tstring name, bool value)
{
  HKEY key;

  if(ERROR_SUCCESS != RegCreateKeyEx(baseHive, path.c_str(), 0, NULL, REG_OPTION_VOLATILE, KEY_SET_VALUE | KEY_WOW64_64KEY | STANDARD_RIGHTS_WRITE | WRITE_OWNER, NULL, &key, NULL))
  {
    return false;
  }

  DWORD finalValue = value ? 1 : 0;

  if(ERROR_SUCCESS != RegSetValueEx(key, name.c_str(), 0, REG_DWORD, (LPCBYTE)&finalValue, sizeof(finalValue)))
  {
    RegCloseKey(key);
    return false;
  }

  RegCloseKey(key);
  return true;
}

bool PersistentSystemOptions::ReadFilterKeysHotkey()
{
  FILTERKEYS settings;
  settings.cbSize = sizeof(settings);
  if(!SystemParametersInfo(SPI_GETFILTERKEYS, sizeof(settings), (PVOID)&settings, 0))
    return true;
  if(settings.dwFlags & FKF_HOTKEYACTIVE)
    return true;
  return false;
}

bool PersistentSystemOptions::WriteFilterKeysHotkey(bool value)
{
  FILTERKEYS settings;
  settings.cbSize = sizeof(settings);
  if(!SystemParametersInfo(SPI_GETFILTERKEYS, sizeof(settings), (PVOID)&settings, 0))
    return false;
  const DWORD flag = FKF_HOTKEYACTIVE;
  settings.dwFlags &= ~flag;
  if(value)
    settings.dwFlags |= flag;
  if(!SystemParametersInfo(SPI_SETFILTERKEYS, sizeof(settings), (PVOID)&settings, SPIF_SENDCHANGE))
    return false;
  return true;
}

bool PersistentSystemOptions::ReadMouseKeysHotkey()
{
  MOUSEKEYS settings;
  settings.cbSize = sizeof(settings);
  if(!SystemParametersInfo(SPI_GETMOUSEKEYS, sizeof(settings), (PVOID)&settings, 0))
    return true;
  if(settings.dwFlags & MKF_HOTKEYACTIVE)
    return true;
  return false;
}

bool PersistentSystemOptions::WriteMouseKeysHotkey(bool value)
{
  MOUSEKEYS settings;
  settings.cbSize = sizeof(settings);
  if(!SystemParametersInfo(SPI_GETMOUSEKEYS, sizeof(settings), (PVOID)&settings, 0))
    return false;
  const DWORD flag = MKF_HOTKEYACTIVE;
  settings.dwFlags &= ~flag;
  if(value)
    settings.dwFlags |= flag;
  if(!SystemParametersInfo(SPI_SETMOUSEKEYS, sizeof(settings), (PVOID)&settings, SPIF_SENDCHANGE))
    return false;
  return true;
}

bool PersistentSystemOptions::ReadStickyKeysHotkey()
{
  STICKYKEYS settings;
  settings.cbSize = sizeof(settings);
  if(!SystemParametersInfo(SPI_GETSTICKYKEYS, sizeof(settings), (PVOID)&settings, 0))
    return true;
  if(settings.dwFlags & SKF_HOTKEYACTIVE)
    return true;
  return false;
}

bool PersistentSystemOptions::WriteStickyKeysHotkey(bool value)
{
  STICKYKEYS settings;
  settings.cbSize = sizeof(settings);
  if(!SystemParametersInfo(SPI_GETSTICKYKEYS, sizeof(settings), (PVOID)&settings, 0))
    return false;
  const DWORD flag = SKF_HOTKEYACTIVE;
  settings.dwFlags &= ~flag;
  if(value)
    settings.dwFlags |= flag;
  if(!SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(settings), (PVOID)&settings, SPIF_SENDCHANGE))
    return false;
  return true;
}

bool PersistentSystemOptions::ReadToggleKeysHotkey()
{
  TOGGLEKEYS settings;
  settings.cbSize = sizeof(settings);
  if(!SystemParametersInfo(SPI_GETTOGGLEKEYS, sizeof(settings), (PVOID)&settings, 0))
    return true;
  if(settings.dwFlags & TKF_HOTKEYACTIVE)
    return true;
  return false;
}

bool PersistentSystemOptions::WriteToggleKeysHotkey(bool value)
{
  TOGGLEKEYS settings;
  settings.cbSize = sizeof(settings);
  if(!SystemParametersInfo(SPI_GETTOGGLEKEYS, sizeof(settings), (PVOID)&settings, 0))
    return false;
  const DWORD flag = TKF_HOTKEYACTIVE;
  settings.dwFlags &= ~flag;
  if(value)
    settings.dwFlags |= flag;
  if(!SystemParametersInfo(SPI_SETTOGGLEKEYS, sizeof(settings), (PVOID)&settings, SPIF_SENDCHANGE))
    return false;
  return true;
}

bool PersistentSystemOptions::ReadHighContrastHotkey()
{
  HIGHCONTRAST settings;
  settings.cbSize = sizeof(settings);
  if(!SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(settings), (PVOID)&settings, 0))
    return true;
  if(settings.dwFlags & HCF_HOTKEYACTIVE)
    return true;
  return false;
}

bool PersistentSystemOptions::WriteHighContrastHotkey(bool value)
{
  HIGHCONTRAST settings;
  settings.cbSize = sizeof(settings);
  if(!SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(settings), (PVOID)&settings, 0))
    return false;
  const DWORD flag = HCF_HOTKEYACTIVE;
  settings.dwFlags &= ~flag;
  if(value)
    settings.dwFlags |= flag;
  if(!SystemParametersInfo(SPI_SETHIGHCONTRAST, sizeof(settings), (PVOID)&settings, SPIF_SENDCHANGE))
    return false;
  return true;
}

PersistentSystemOptions PersistentSystemOptions::ReadFromStorage()
{
  PersistentSystemOptions retval;
  retval.hasChangePassword = ReadRegistryKey(HKEY_CURRENT_USER, _T(REGISTRY_SETTINGS_LOCATION "\\PersistentSystemOptions"), _T("hasChangePassword"), true);
  retval.hasLockComputer = ReadRegistryKey(HKEY_CURRENT_USER, _T(REGISTRY_SETTINGS_LOCATION "\\PersistentSystemOptions"), _T("hasLockComputer"), true);
  retval.hasLogOff = ReadRegistryKey(HKEY_CURRENT_USER, _T(REGISTRY_SETTINGS_LOCATION "\\PersistentSystemOptions"), _T("hasLogOff"), true);
  retval.hasSwitchUser = ReadRegistryKey(HKEY_CURRENT_USER, _T(REGISTRY_SETTINGS_LOCATION "\\PersistentSystemOptions"), _T("hasSwitchUser"), true);
  retval.hasTaskManager = ReadRegistryKey(HKEY_CURRENT_USER, _T(REGISTRY_SETTINGS_LOCATION "\\PersistentSystemOptions"), _T("hasTaskManager"), true);
  retval.hasPower = ReadRegistryKey(HKEY_CURRENT_USER, _T(REGISTRY_SETTINGS_LOCATION "\\PersistentSystemOptions"), _T("hasPower"), true);
  retval.hasFilterKeysHotkey = ReadRegistryKey(HKEY_CURRENT_USER, _T(REGISTRY_SETTINGS_LOCATION "\\PersistentSystemOptions"), _T("hasFilterKeysHotkey"), true);
  retval.hasMouseKeysHotkey = ReadRegistryKey(HKEY_CURRENT_USER, _T(REGISTRY_SETTINGS_LOCATION "\\PersistentSystemOptions"), _T("hasMouseKeysHotkey"), true);
  retval.hasStickyKeysHotkey = ReadRegistryKey(HKEY_CURRENT_USER, _T(REGISTRY_SETTINGS_LOCATION "\\PersistentSystemOptions"), _T("hasStickyKeysHotkey"), true);
  retval.hasToggleKeysHotkey = ReadRegistryKey(HKEY_CURRENT_USER, _T(REGISTRY_SETTINGS_LOCATION "\\PersistentSystemOptions"), _T("hasToggleKeysHotkey"), true);
  retval.hasHighContrastHotkey = ReadRegistryKey(HKEY_CURRENT_USER, _T(REGISTRY_SETTINGS_LOCATION "\\PersistentSystemOptions"), _T("hasHighContrastHotkey"), true);
  return retval;
}

bool PersistentSystemOptions::WriteToStorage() const
{
  bool allWorked = true;
  if(!WriteRegistryKey(HKEY_CURRENT_USER, _T(REGISTRY_SETTINGS_LOCATION "\\PersistentSystemOptions"), _T("hasChangePassword"), hasChangePassword))
    allWorked = false;

  if(!WriteRegistryKey(HKEY_CURRENT_USER, _T(REGISTRY_SETTINGS_LOCATION "\\PersistentSystemOptions"), _T("hasLockComputer"), hasLockComputer))
    allWorked = false;

  if(!WriteRegistryKey(HKEY_CURRENT_USER, _T(REGISTRY_SETTINGS_LOCATION "\\PersistentSystemOptions"), _T("hasLogOff"), hasLogOff))
    allWorked = false;

  if(!WriteRegistryKey(HKEY_CURRENT_USER, _T(REGISTRY_SETTINGS_LOCATION "\\PersistentSystemOptions"), _T("hasSwitchUser"), hasSwitchUser))
    allWorked = false;

  if(!WriteRegistryKey(HKEY_CURRENT_USER, _T(REGISTRY_SETTINGS_LOCATION "\\PersistentSystemOptions"), _T("hasTaskManager"), hasTaskManager))
    allWorked = false;

  if(!WriteRegistryKey(HKEY_CURRENT_USER, _T(REGISTRY_SETTINGS_LOCATION "\\PersistentSystemOptions"), _T("hasPower"), hasPower))
    allWorked = false;

  if(!WriteRegistryKey(HKEY_CURRENT_USER, _T(REGISTRY_SETTINGS_LOCATION "\\PersistentSystemOptions"), _T("hasFilterKeysHotkey"), hasFilterKeysHotkey))
    allWorked = false;

  if(!WriteRegistryKey(HKEY_CURRENT_USER, _T(REGISTRY_SETTINGS_LOCATION "\\PersistentSystemOptions"), _T("hasMouseKeysHotkey"), hasMouseKeysHotkey))
    allWorked = false;

  if(!WriteRegistryKey(HKEY_CURRENT_USER, _T(REGISTRY_SETTINGS_LOCATION "\\PersistentSystemOptions"), _T("hasStickyKeysHotkey"), hasStickyKeysHotkey))
    allWorked = false;

  if(!WriteRegistryKey(HKEY_CURRENT_USER, _T(REGISTRY_SETTINGS_LOCATION "\\PersistentSystemOptions"), _T("hasToggleKeysHotkey"), hasToggleKeysHotkey))
    allWorked = false;

  if(!WriteRegistryKey(HKEY_CURRENT_USER, _T(REGISTRY_SETTINGS_LOCATION "\\PersistentSystemOptions"), _T("hasHighContrastHotkey"), hasHighContrastHotkey))
    allWorked = false;

  return allWorked;
}

PersistentSystemOptions persistentSystemOptions;

int action_block() {
  return 0;
//  DWORD sessionid;
//  char *username;
//  DWORD usersize = 0;
//
//  if (!getActiveConsoleId(&sessionid)) {
//    //error("WTSEnumerateSessions failed: %ld", GetLastError());
//    return 1;
//  } else {
//    //info("WTSEnumerateSessions got active session: %d", sessionid);
//  }
//
//  if (!WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, sessionid, WTSUserName, &username, &usersize)) {
//    //error("WTSQuerySessionInformation failed: %ld", GetLastError());
//    return 1;
//  } else {
//    //info("WTSQuerySessionInformation got user: %s", username);
//  }
//
//  PSID sid = NULL;
//  DWORD sid_size = 0;
//  char* domain = NULL;
//  DWORD domain_size = 0;
//  SID_NAME_USE use;
//
//  // First, we need to get the buffer size needed to store the SID
//  if(!LookupAccountName(NULL, (LPSTR)username, sid, &sid_size, domain, &domain_size, &use)) {
//    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
//      //error("LookupAccountName error while getting buffer size: %ld", GetLastError());
//      return 1;
//    }
//  }
//
//  try {
//    sid = (PSID) LocalAlloc(LMEM_FIXED, sid_size);
//    domain = (char*) malloc(domain_size * sizeof(char));
//    if ( !sid || !domain ) {
//      throw(0);
//    }
//    if (!LookupAccountName(NULL, (LPSTR)username, sid, &sid_size, domain, &domain_size, &use)) {
//      //error("LookupAccountName error while getting buffer size: %ld", GetLastError());
//      throw(0);
//    }
//  } catch (int) {
//    LocalFree(sid);
//    sid = NULL;
//    //error("LookupAccountName error while getting actual data: %ld", GetLastError());
//    return 1;
//  }
//
//  char* string_sid;
//  if(!ConvertSidToStringSid(sid, &string_sid)) {
//    //error("Could not convert to string from SID");
//    return 1;
//  } else {
//    //info("Got SID: %s", string_sid);
//  }
//
//  persistentSystemOptions = PersistentSystemOptions::ReadSystemSettings();
//  if(verbose)
//  {
//    Tcout << _T("Read persistent system options:") << std::endl;
//    persistentSystemOptions.Dump(Tcout);
//  }
//  bool succeded = persistentSystemOptions.WriteToStorage();
//  if(verbose)
//  {
//    if(succeded)
//      Tcout << _T("Saved persistent system options.") << std::endl;
//    else
//      Tcout << _T("Saving persistent system options failed.") << std::endl;
//  }
//  if(!succeded)
//    return 1;
//  succeded = PersistentSystemOptions::AllDisabled().WriteSystemSettings();
//  if(verbose)
//  {
//    if(succeded)
//      Tcout << _T("Blocked persistent system options.") << std::endl;
//    else
//      Tcout << _T("Blocking persistent system options failed.") << std::endl;
//  }
//  if(!succeded)
//    return 1;
//  return 0;
}

int action_unblock() {
  return 0;
}

int action_dump() {
  persistentSystemOptions = PersistentSystemOptions::ReadSystemSettings();
  persistentSystemOptions.Dump(Tcout);
  return 0;
}

int action_force_unblock() {
  bool succeded = PersistentSystemOptions::AllEnabled().WriteSystemSettings();
  if(verbose)
  {
    if(succeded)
      Tcout << _T("Reset persistent system options.") << std::endl;
    else
      Tcout << _T("Reseting persistent system options failed.") << std::endl;
  }
  if(!succeded)
    return 1;
  return 0;
}

int action_save() {
  persistentSystemOptions = PersistentSystemOptions::ReadSystemSettings();
  if(verbose)
  {
    Tcout << _T("Read persistent system options:") << std::endl;
    persistentSystemOptions.Dump(Tcout);
  }
  bool succeded = persistentSystemOptions.WriteToStorage();
  if(verbose)
  {
    if(succeded)
      Tcout << _T("Saved persistent system options.") << std::endl;
    else
      Tcout << _T("Saving persistent system options failed.") << std::endl;
  }
  if(!succeded)
    return 1;
  return 0;
}

int action_write_settings() {
  if(!persistentSystemOptions.WriteSystemSettings())
  {
    if(verbose)
    {
      Tcout << _T("Writing persistent system options failed.");
    }
  }
}
//int registry_unblock() {
//  persistentSystemOptions = PersistentSystemOptions::ReadSystemSettings();
//  if(verbose)
//  {
//    Tcout << _T("Read persistent system options:") << std::endl;
//    persistentSystemOptions.Dump(Tcout);
//  }
//  bool succeded = persistentSystemOptions.WriteToStorage();
//  if(verbose)
//  {
//    if(succeded)
//      Tcout << _T("Saved persistent system options.") << std::endl;
//    else
//      Tcout << _T("Saving persistent system options failed.") << std::endl;
//  }
//  if(!succeded)
//    return 1;
//  succeded = PersistentSystemOptions::AllDisabled().WriteSystemSettings();
//  if(verbose)
//  {
//    if(succeded)
//      Tcout << _T("Blocked persistent system options.") << std::endl;
//    else
//      Tcout << _T("Blocking persistent system options failed.") << std::endl;
//  }
//  if(!succeded)
//    return 1;
//  return 0;
//
//}
