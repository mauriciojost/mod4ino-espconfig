#ifndef GLOBAL_INC
#define GLOBAL_INC

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

#define CLASS_SETTINGS "ST"

#ifndef WIFI_SSID_STEADY
#define WIFI_SSID_STEADY "???"
#endif // WIFI_SSID_STEADY

#ifndef WIFI_PASSWORD_STEADY
#define WIFI_PASSWORD_STEADY "???"
#endif // WIFI_PASSWORD_STEADY

#ifndef PERIOD_SEC
#define PERIOD_SEC 60
#endif // PERIOD_SEC

#ifndef FRAG_TO_SLEEP_MS_MAX
#define FRAG_TO_SLEEP_MS_MAX 1000 // maximum sleeping time for which the module can be unresponsive
#endif                            // FRAG_TO_SLEEP_MS_MAX

#define PERIOD_MSEC (PERIOD_SEC * 1000)

#define CREDENTIAL_BUFFER_SIZE 64
#define STATUS_BUFFER_SIZE 64
#define VERSION_BUFFER_SIZE 32

enum SettingsProps {
  SettingsDebugProp = 0,    // boolean, define if the device is in debug mode
  SettingsStatusProp,       // string, defines the current general status of the device (vcc level, heap, etc)
  SettingsVersionProp,      // string, defines the current version
  SettingsLcdLogsProp,      // boolean, define if the device display logs in LCD
  SettingsOneRunProp,       // boolean, define if the device is to be launched only once and then reseted (used in deep sleep mode)
  SettingsFsLogsProp,       // boolean, define if logs are to be dumped in the file system (only in debug mode)
  SettingsPeriodMsProp,     // period in msec for the device to wait until update clock and make actors catch up with acting (if any)
  SettingsMiniPeriodMsProp, // period in msec for the device to got to sleep (and remain unresponsive from user) (only if no deep sleep)
  SettingsWifiSsidProp,     // wifi ssid
  SettingsWifiPassProp,     // wifi pass
  SettingsPropsDelimiter    // amount of properties
};

class Settings : public Actor {

private:
  const char *name;
  bool devDebug;
  bool lcdLogs;
  bool oRun;
  bool fsLogs;
  int periodms;
  int miniperiodms;
  Buffer *ssid;
  Buffer *pass;
  Buffer *status;
  Buffer *version;
  Metadata *md;

public:
  Settings(const char *n) {
    name = n;
    ssid = new Buffer(CREDENTIAL_BUFFER_SIZE);
    ssid->load(WIFI_SSID_STEADY);

    pass = new Buffer(CREDENTIAL_BUFFER_SIZE);
    pass->load(WIFI_PASSWORD_STEADY);

    status = new Buffer(STATUS_BUFFER_SIZE);

    version = new Buffer(VERSION_BUFFER_SIZE);

    devDebug = true;
    lcdLogs = false;
    oRun = false;
    fsLogs = false;
    periodms = PERIOD_MSEC;
    miniperiodms = FRAG_TO_SLEEP_MS_MAX;
    md = new Metadata(n);
  }

  const char *getName() {
    return name;
  }

  void act() {}

  const char *getPropName(int propIndex) {
    switch (propIndex) {
      case (SettingsWifiSsidProp):
        return SENSITIVE_PROP_PREFIX "wifissid";
      case (SettingsWifiPassProp):
        return SENSITIVE_PROP_PREFIX "wifipass";
      case (SettingsDebugProp):
        return DEBUG_PROP_PREFIX "debug";
      case (SettingsStatusProp):
        return STATUS_PROP_PREFIX "status";
      case (SettingsVersionProp):
        return DEBUG_PROP_PREFIX "version";
      case (SettingsLcdLogsProp):
        return DEBUG_PROP_PREFIX "lcdlogs";
      case (SettingsOneRunProp):
        return ADVANCED_PROP_PREFIX "onerun";
      case (SettingsFsLogsProp):
        return DEBUG_PROP_PREFIX "fslogs";
      case (SettingsPeriodMsProp):
        return DEBUG_PROP_PREFIX "periodms";
      case (SettingsMiniPeriodMsProp):
        return DEBUG_PROP_PREFIX "mperiodms";
      default:
        return "";
    }
  }

  void getSetPropValue(int propIndex, GetSetMode m, const Value *targetValue, Value *actualValue) {
    switch (propIndex) {
      case (SettingsDebugProp):
        setPropBoolean(m, targetValue, actualValue, &devDebug);
        break;
      case (SettingsStatusProp):
        setPropValue(m, targetValue, actualValue, status);
        break;
      case (SettingsVersionProp):
        setPropValue(m, targetValue, actualValue, version);
        break;
      case (SettingsLcdLogsProp):
        setPropBoolean(m, targetValue, actualValue, &lcdLogs);
        break;
      case (SettingsOneRunProp):
        setPropBoolean(m, targetValue, actualValue, &oRun);
        break;
      case (SettingsFsLogsProp):
        setPropBoolean(m, targetValue, actualValue, &fsLogs);
        break;
      case (SettingsPeriodMsProp):
        setPropInteger(m, targetValue, actualValue, &periodms);
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

  bool getDebug() {
    return devDebug;
  }

  bool getLcdLogs() {
    return lcdLogs;
  }

  void setDebug(bool b) {
    if (b != devDebug) {
      getMetadata()->changed();
    }
    devDebug = b;
  }

  const char *getSsid() {
    return ssid->getBuffer();
  }

  void setSsid(const char *s) {
    ssid->load(s);
  }

  const char *getPass() {
    return pass->getBuffer();
  }

  void setPass(const char *s) {
    pass->load(s);
  }

  void setVersion(const char *v) {
    if (!version->equals(v)){
      version->load(v);
      getMetadata()->changed();
    }
  }

  void setStatus(float vcc, int heap) {
    status->fill("vcc:%0.2f,heap:%d", vcc, heap);
    getMetadata()->changed();
  }

  const char* getStatus() {
    return status->getBuffer();
  }

  bool oneRun() {
    return oRun;
  }

  int periodMsec() {
    return periodms;
  }

  int miniPeriodMsec() {
    return miniperiodms;
  }

  bool fsLogsEnabled() {
    return fsLogs;
  }
};

#endif // GLOBAL_INC
