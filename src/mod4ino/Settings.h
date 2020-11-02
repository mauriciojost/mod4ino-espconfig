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
#define VERSION_BUFFER_SIZE 32
#define DEV_DEBUG_BUFFER_SIZE 16

#define LOGOPTS_BUFFER_SIZE (4 * 8)
#define LOGOPTS_DEFAULT "??D;"
#define TARGET_BUFFER_SIZE 32
#define SKIP_UPDATES_CODE "SKIP"
#define LATEST_UPDATES_CODE "LATEST"

#define ALIAS_BUFFER_SIZE 16
#define DEFAULT_ALIAS "alias"

#define PROJECT_BUFFER_SIZE 16
#define DEFAULT_PROJECT "project"

#define PLATFORM_BUFFER_SIZE 16
#define DEFAULT_PLATFORM "platform"

enum SettingsProps {
  SettingsDebugProp = 0,    // boolean, define if the device is in debug mode
  SettingsVersionProp,      // string, defines the current version
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
  SettingsCmdFreqProp,      // frequency of execution of command
  SettingsCmdLineProp,      // command line to execute
#endif // INSECURE
  SettingsAliasProp,        // alias name for this device
  SettingsProjectProp,      // project name
  SettingsPlatformProp,     // platform name
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
  Buffer *version;
  Buffer *target;
  Buffer *alias;
  Buffer *project;
  Buffer *platform;
#ifdef INSECURE
  Buffer *cmdLine;
  Timing *cmdTiming;
  Module *mod;
#endif // INSECURE
  Metadata *md;
  Timing *batchTiming;
  void (*update)(const char *targetVersion, const char *currentVersion);
  PropSync *propSync;

  void commandObs(const char* c); // deprecated

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

    alias = new Buffer(ALIAS_BUFFER_SIZE);
    alias->load(DEFAULT_ALIAS);

    project = new Buffer(PROJECT_BUFFER_SIZE);
    project->load(DEFAULT_PROJECT);

    platform = new Buffer(PLATFORM_BUFFER_SIZE);
    platform->load(DEFAULT_PLATFORM);

    logOpts = new Buffer(LOGOPTS_BUFFER_SIZE);
    logOpts->load(LOGOPTS_DEFAULT);

    version = new Buffer(VERSION_BUFFER_SIZE);
    version->load(STRINGIFY(PROJ_VERSION));

    devDebug = new Buffer(DEV_DEBUG_BUFFER_SIZE);
    devDebug->clear();

    miniperiodms = FRAG_TO_SLEEP_MS_MAX;
    updateScheduled = false;
    md = new Metadata(n);
    md->setPersist(true); // persist it in FS when possible
    md->getTiming()->setFreq("~24h");
    batchTiming = new Timing("stn-btch");
    batchTiming->setFreq("~1m");

#ifdef INSECURE
    cmdLine = new Buffer(CMD_LINE_BUFFER_SIZE);
    cmdLine->clear();
    cmdTiming = new Timing("stn-cmd");
    cmdTiming->setFreq("never");
    mod = NULL;
#endif // INSECURE

    target = new Buffer(TARGET_BUFFER_SIZE);
    target->load(LATEST_UPDATES_CODE);
    update = NULL;
    propSync = NULL;
  }

  const char *getName() {
    return name;
  }

  CmdExecStatus command(const char *) {
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


  void updateIfMust() {
    const char *currVersion = STRINGIFY(PROJ_VERSION);
    if (updateScheduled) {
      if (!target->equals(SKIP_UPDATES_CODE)) {
        log(CLASS_SETTINGS, Warn, "Update:'%s'->'%s'", currVersion, target->getBuffer());
        if (update != NULL) {
          PropSyncStatusCode st = propSync->pushActors(true); // push properties to the server
          if (!propSync->isFailure(st)) {
            update(target->getBuffer(), currVersion); // update
            updateScheduled = false; // in case update failed, forget the attempt
          } else {
            log(CLASS_SETTINGS, Warn, "UPD SKP(%d)", (int)st);
          }
        } else {
          log(CLASS_SETTINGS, Warn, "No init.");
        }
      }
    }
    
  }

  void setup(
    Module *m,
    const char *pr,
    const char *pl,
    void (*u)(const char *targetVersion, const char *currentVersion),
    PropSync *ps
  ) {
#ifdef INSECURE
    mod = m;
#endif // INSECURE
    update = u;
    setProject(pr);
    setPlatform(pl);
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
      case (SettingsVersionProp):
        return DEBUG_PROP_PREFIX "version";
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
      case (SettingsCmdFreqProp):
        return ADVANCED_PROP_PREFIX "cmdfreq";
      case (SettingsCmdLineProp):
        return ADVANCED_PROP_PREFIX "cmd";
#endif // INSECURE
      case (SettingsAliasProp):
        return "alias";
      case (SettingsProjectProp):
        return "project";
      case (SettingsPlatformProp):
        return "platform";
      default:
        return "";
    }
  }

  void getSetPropValue(int propIndex, GetSetMode m, const Value *targetValue, Value *actualValue) {
    switch (propIndex) {
      case (SettingsDebugProp):
        setPropValue(m, targetValue, actualValue, devDebug);
        break;
      case (SettingsVersionProp):
        setPropValue(m, NULL, actualValue, version); // read only, truth comes from firmware
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
      case (SettingsCmdFreqProp):
        setPropTiming(m, targetValue, actualValue, cmdTiming);
        break;
      case (SettingsCmdLineProp):
        setPropValue(m, targetValue, actualValue, cmdLine);
        break;
#endif // INSECURE
      case (SettingsAliasProp):
        setPropValue(m, targetValue, actualValue, alias);
        break;
      case (SettingsProjectProp):
        setPropValue(m, NULL, actualValue, project); // read only, truth comes from the firmware
        break;
      case (SettingsPlatformProp):
        setPropValue(m, NULL, actualValue, platform); // read only, truth comes from the firmware
        break;
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

  bool getDebug() {
    return getDebugFlag('D', true);
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

  const char* getAlias() {
    return alias->getBuffer();
  }

  const char* getProject() {
    return project->getBuffer();
  }

  const char* getPlatform() {
    return platform->getBuffer();
  }

  void setProject(const char *p) {
    project->load(p);
  }

  void setPlatform(const char *p) {
    platform->load(p);
  }

};

#endif // SETTINGS_MODULE_INC
