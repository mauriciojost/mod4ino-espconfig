#ifndef SETTINGS_MODULE_INC
#define SETTINGS_MODULE_INC

/**
 * Settings
 *
 * Holds global settings for the device (like log level, debug mode, etc.).
 *
 */

#include <log4ino/Log.h>
#include <main4ino/Actor.h>
#include <main4ino/Boolean.h>
#include <main4ino/Integer.h>
#include <main4ino/Cmd.h>
#include <main4ino/CmdExecStatus.h>
#include <mod4ino/Module.h>

#define CLASS_SETTINGS "SE"

#ifndef WIFI_SSID_STEADY
#define WIFI_SSID_STEADY "???"
#endif // WIFI_SSID_STEADY

#ifndef WIFI_PASSWORD_STEADY
#define WIFI_PASSWORD_STEADY "???"
#endif // WIFI_PASSWORD_STEADY

#define CMD_LINE_BUFFER_SIZE 64

#ifndef FRAG_TO_SLEEP_MS_MAX
#define FRAG_TO_SLEEP_MS_MAX 1000 // maximum sleeping time for which the module can be unresponsive
#endif                            // FRAG_TO_SLEEP_MS_MAX

#define CREDENTIAL_BUFFER_SIZE 64
#define STATUS_BUFFER_SIZE 64
#define DEV_DEBUG_BUFFER_SIZE 16

#define LOGOPTS_BUFFER_SIZE (4 * 8)
#define LOGOPTS_DEFAULT "??D;"
#define TARGET_BUFFER_SIZE 32
#define SKIP_UPDATES_CODE "SKIP"
#define LATEST_UPDATES_CODE "LATEST"

enum SettingsProps {
  SettingsDebugProp = 0,    // boolean, define if the device is in debug mode
  SettingsMiniPeriodMsProp, // period in msec for the device to got to sleep before waking up if any interrupts (remaining unresponsive from
                            // user) (only if no deep sleep)
  SettingsWifiSsidProp,     // wifi ssid
  SettingsWifiPassProp,     // wifi pass
  SettingsWifiSsidbProp,    // wifi ssid (backup net)
  SettingsWifiPassbProp,    // wifi pass (backup net)
  SettingsLogOptionsProp,   // options of the log messages (example: AAE;BBF;??D)
  SettingsUpdateTargetProp, // target version to upgrade the firmware to
  SettingsUpdateFreqProp,   // frequency of upgrade
  SettingsBatchFreqProp,    // frequency of batch runs for all actors
#ifdef INSECURE
  SettingsCmdsFreqProp,     // frequency of execution of commands
  SettingsCmdsProp,         // commands to execute ath the given frequency
#endif // INSECURE
  SettingsPropsDelimiter    // amount of properties
};

class Module;

class Settings : public Actor {

private:
  const char *name;
  Buffer* devDebug;
  int miniperiodms;
  bool updateScheduled;
  Buffer *ssid;
  Buffer *pass;
  Buffer *ssidb;
  Buffer *passb;
  Buffer *logOpts;
  Buffer *target;
#ifdef INSECURE
  Buffer *cmdLine;
  Timing *cmdTiming;
  Module *mod;
#endif // INSECURE
  Metadata *md;
  Timing *batchTiming;
  PropSync *propSync;

public:
  Settings(const char *n) {
    name = n;
    ssid = new Buffer(CREDENTIAL_BUFFER_SIZE);
    ssid->load(WIFI_SSID_STEADY);

    pass = new Buffer(CREDENTIAL_BUFFER_SIZE);
    pass->load(WIFI_PASSWORD_STEADY);

    ssidb = new Buffer(CREDENTIAL_BUFFER_SIZE);
    ssidb->load(WIFI_SSID_STEADY);

    passb = new Buffer(CREDENTIAL_BUFFER_SIZE);
    passb->load(WIFI_PASSWORD_STEADY);

    logOpts = new Buffer(LOGOPTS_BUFFER_SIZE);
    logOpts->load(LOGOPTS_DEFAULT);

    devDebug = new Buffer(DEV_DEBUG_BUFFER_SIZE);
    devDebug->clear();

    miniperiodms = FRAG_TO_SLEEP_MS_MAX;
    updateScheduled = false;
    md = new Metadata(n, CLASS_SETTINGS);
    md->setPersist(true); // persist it in FS when possible
    md->getTiming()->setFreq("~24h");
    batchTiming = new Timing("m4inobatch", CLASS_SETTINGS);
    batchTiming->setFreq("~1m");

#ifdef INSECURE
    cmdLine = new Buffer(CMD_LINE_BUFFER_SIZE);
    cmdLine->clear();
    cmdTiming = new Timing("m4inocmd", CLASS_SETTINGS);
    cmdTiming->setFreq("never");
    mod = NULL;
#endif // INSECURE

    target = new Buffer(TARGET_BUFFER_SIZE);
    target->load(LATEST_UPDATES_CODE);
    propSync = NULL;
  }

  const char *getName() {
    return name;
  }

  CmdExecStatus command(Cmd *) {
    return NotFound;
  }

  Act act(Metadata* md) {
    if (getTiming()->matches()) {
      updateScheduled = true;
    }
    batchTiming->setCurrentTime(getTiming()->getCurrentTime()); // align with time
#ifdef INSECURE
    cmdTiming->setCurrentTime(getTiming()->getCurrentTime()); // align with time
    if (cmdTiming->matches()) {
      return Act(cmdLine->getBuffer());
    }
#endif // INSECURE
    return Act("");
  }

  void setup(
    Module *m,
    PropSync *ps
  ) {
#ifdef INSECURE
    mod = m;
#endif // INSECURE
    propSync = ps;
  }

  const char *getPropName(int propIndex) {
    switch (propIndex) {
      case (SettingsWifiSsidProp):
        return SENSITIVE_PROP_PREFIX "wifissid";
      case (SettingsWifiPassProp):
        return SENSITIVE_PROP_PREFIX "wifipass";
      case (SettingsWifiSsidbProp):
        return SENSITIVE_PROP_PREFIX "wifissidb";
      case (SettingsWifiPassbProp):
        return SENSITIVE_PROP_PREFIX "wifipassb";
      case (SettingsDebugProp):
        return DEBUG_PROP_PREFIX "debug";
      case (SettingsMiniPeriodMsProp):
        return ADVANCED_PROP_PREFIX "mperiodms";
      case (SettingsLogOptionsProp):
        return DEBUG_PROP_PREFIX "logo";
      case (SettingsUpdateTargetProp):
        return ADVANCED_PROP_PREFIX "utarget";
      case (SettingsUpdateFreqProp):
        return ADVANCED_PROP_PREFIX "ufreq";
      case (SettingsBatchFreqProp):
        return ADVANCED_PROP_PREFIX "bfreq";
#ifdef INSECURE
      case (SettingsCmdsFreqProp):
        return ADVANCED_PROP_PREFIX "cmdsfreq";
      case (SettingsCmdsProp):
        return ADVANCED_PROP_PREFIX "cmds";
#endif // INSECURE
      default:
        return "";
    }
  }

  void getSetPropValue(int propIndex, GetSetMode m, const Value *targetValue, Value *actualValue) {
    switch (propIndex) {
      case (SettingsDebugProp):
        setPropValue(m, targetValue, actualValue, devDebug);
        break;
      case (SettingsMiniPeriodMsProp):
        setPropInteger(m, targetValue, actualValue, &miniperiodms);
        break;
      case (SettingsWifiSsidProp):
        setPropValue(m, targetValue, actualValue, ssid);
        break;
      case (SettingsWifiPassProp):
        setPropValue(m, targetValue, actualValue, pass);
        break;
      case (SettingsWifiSsidbProp):
        setPropValue(m, targetValue, actualValue, ssidb);
        break;
      case (SettingsWifiPassbProp):
        setPropValue(m, targetValue, actualValue, passb);
        break;
      case (SettingsLogOptionsProp):
        setPropValue(m, targetValue, actualValue, logOpts);
        if (m == SetCustomValue) {
          setLogOptions(logOpts->getBuffer());
        }
        break;
      case (SettingsUpdateTargetProp):
        setPropValue(m, targetValue, actualValue, target);
        break;
      case (SettingsUpdateFreqProp):
        setPropTiming(m, targetValue, actualValue, getTiming());
        break;
      case (SettingsBatchFreqProp):
        setPropTiming(m, targetValue, actualValue, batchTiming);
        break;
#ifdef INSECURE
      case (SettingsCmdsFreqProp):
        setPropTiming(m, targetValue, actualValue, cmdTiming);
        break;
      case (SettingsCmdsProp):
        setPropValue(m, targetValue, actualValue, cmdLine);
        break;
#endif // INSECURE
      default:
        break;
    }
    if (m != GetValue) {
      getMetadata()->changed();
    }
  }

  int getNroProps() {
    return SettingsPropsDelimiter;
  }

  Metadata *getMetadata() {
    return md;
  }

  bool getDebugFlag(char flag, bool d = false) {
    int f = tolower(flag);
    int F = toupper(flag);
    for (unsigned int i = 0; i < devDebug->getLength(); i++) {
      int v = (int)(devDebug->getUnsafeBuffer()[i]);
      if (v == F) {
        return true;
      } else if (v == f) {
        return false;
      }
    }
    return d;
  }

  // used from letter P - Z (A - O reserved for the user)
  bool pushLogsEnabled() {
    return getDebugFlag('P', false);
  }

  bool getWaitOnBoot() {
    return getDebugFlag('W', true);
  }

  const char *getSsid() {
    return ssid->getBuffer();
  }

  void setSsid(const char *s) {
    if (!ssid->equals(s)) {
      ssid->load(s);
      getMetadata()->changed();
    }
  }

  const char *getPass() {
    return pass->getBuffer();
  }

  void setPass(const char *s) {
    if (!pass->equals(s)) {
      pass->load(s);
      getMetadata()->changed();
    }
  }

  const char *getSsidBackup() {
    return ssidb->getBuffer();
  }

  void setSsidBackup(const char *s) {
    if (!ssidb->equals(s)) {
      ssidb->load(s);
      getMetadata()->changed();
    }
  }

  const char *getPassBackup() {
    return passb->getBuffer();
  }

  void setPassBackup(const char *s) {
    if (!passb->equals(s)) {
      passb->load(s);
      getMetadata()->changed();
    }
  }

  int miniPeriodMsec() {
    return miniperiodms;
  }

  Timing* getBatchTiming() {
    return batchTiming;
  }

  Buffer* getTarget() {
    return target;
  }

  bool isUpdateScheduled() {
    return updateScheduled;
  }

  void setUpdateScheduled(bool b) {
    updateScheduled = b;
  }

};

#endif // SETTINGS_MODULE_INC
