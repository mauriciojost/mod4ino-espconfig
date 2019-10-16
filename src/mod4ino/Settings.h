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
  SettingsVersionProp,      // string, defines the current version
  SettingsPeriodMsProp,     // period in msec for the device to wait until update clock and make actors catch up with acting (if any)
  SettingsMiniPeriodMsProp, // period in msec for the device to got to sleep before waking up if any interrupts (remaining unresponsive from user) (only if no deep sleep)
  SettingsWifiSsidProp,     // wifi ssid
  SettingsWifiPassProp,     // wifi pass
  SettingsWifiSsidbProp,    // wifi ssid (backup net)
  SettingsWifiPassbProp,    // wifi pass (backup net)
  SettingsPropsDelimiter    // amount of properties
};

class Settings : public Actor {

private:
  const char *name;
  bool devDebug;
  int periodms;
  int miniperiodms;
  Buffer *ssid;
  Buffer *pass;
  Buffer *ssidb;
  Buffer *passb;
  Buffer *version;
  Metadata *md;

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

    version = new Buffer(VERSION_BUFFER_SIZE);

    devDebug = true;
    periodms = PERIOD_MSEC;
    miniperiodms = FRAG_TO_SLEEP_MS_MAX;
    md = new Metadata(n);
  }

  const char *getName() {
    return name;
  }

  void act() {
    static bool first = true;
    if (first) {
      first = false;
      version->load(STRINGIFY(PROJ_VERSION));
    }
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
      case (SettingsPeriodMsProp):
        return ADVANCED_PROP_PREFIX "periodms";
      case (SettingsMiniPeriodMsProp):
        return ADVANCED_PROP_PREFIX "mperiodms";
      default:
        return "";
    }
  }

  void getSetPropValue(int propIndex, GetSetMode m, const Value *targetValue, Value *actualValue) {
    switch (propIndex) {
      case (SettingsDebugProp):
        setPropBoolean(m, targetValue, actualValue, &devDebug);
        break;
      case (SettingsVersionProp):
        setPropValue(m, targetValue, actualValue, version);
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
      case (SettingsWifiSsidbProp):
        setPropValue(m, targetValue, actualValue, ssidb);
        break;
      case (SettingsWifiPassbProp):
        setPropValue(m, targetValue, actualValue, passb);
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

  void setDebug(bool b) {
    if (b != devDebug) {
      devDebug = b;
      getMetadata()->changed();
    }
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


  void setVersion(const char *v) {
    if (!version->equals(v)) {
      version->load(v);
      getMetadata()->changed();
    }
  }

  int periodMsec() {
    return periodms;
  }

  int miniPeriodMsec() {
    return miniperiodms;
  }
};

#endif // GLOBAL_INC
