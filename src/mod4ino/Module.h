#ifndef MODULE_INC
#define MODULE_INC

#include <log4ino/Log.h>
#include <main4ino/Actor.h>
#include <main4ino/Array.h>
#include <main4ino/Clock.h>
#include <main4ino/PropSync.h>
#include <main4ino/ClockSync.h>
#include <main4ino/HttpMethods.h>
#include <main4ino/HttpResponse.h>
#include <main4ino/Authenticable.h>
#include <main4ino/SerBot.h>
#include <main4ino/Table.h>
#include <mod4ino/MsgClearMode.h>
#include <mod4ino/Settings.h>
#include <mod4ino/Status.h>

#define CLASS_MODULE "MD"

#define COMMAND_MAX_LENGTH 128

#define PERIOD_CONFIGURE_SEC 1
#define MAX_BATCH_PERIOD_SECS 172800 // 2 days

#ifndef SLEEP_PERIOD_UPON_BOOT_SECS
#define SLEEP_PERIOD_UPON_BOOT_SECS 2
#endif // SLEEP_PERIOD_UPON_BOOT_SECS

#ifndef CAT_BUFFER_LENGTH
#define CAT_BUFFER_LENGTH 512
#endif // CAT_BUFFER_LENGTH

#define HELP_COMMAND_CLI                                                                                                                   \
  "\n  MODULE HELP"                                                                                                                        \
  "\n  int             : interrupt current ongoing action"                                                                                 \
  "\n  mode [run,conf] : get or set the mode"                                                                                              \
  "\n  info            : show info about the device"                                                                                       \
  "\n  test            : test the architecture/hardware"                                                                                   \
  "\n  update ...      : update the firmware with the given target version"                                                                \
  "\n  wifi            : init steady wifi"                                                                                                 \
  "\n  wifistop        : stop steady wifi"                                                                                                 \
  "\n  get             : display actors properties"                                                                                        \
  "\n  get ...         : display actor <actor> properties"                                                                                 \
  "\n  set ...         : set an actor property (example: 'set body msg0 HELLO')"                                                           \
  "\n  logl [...]      : get / change log level to <x> (0 is more verbose, to 3 least verbose)"                                            \
  "\n  logo [...]      : get / change log options (examples: AA0;BB2;??4; means AA to DEBUG, BB to WARN, rest to USER)"                    \
  "\n  clear           : clear device (filesystem, crashes stacktrace, etc.)"                                                              \
  "\n  actall          : all act"                                                                                                          \
  "\n  touchall        : mark actors as 'changed' to force synchronization with the server"                                                \
  "\n  actone ...      : make actor <x> act"                                                                                               \
  "\n  wifissid ...    : set wifi ssid"                                                                                                    \
  "\n  wifipass ...    : set wifi pass"                                                                                                    \
  "\n  wifissidb ...   : set wifi ssid (backup net)"                                                                                       \
  "\n  wifipassb ...   : set wifi pass (backup net)"                                                                                       \
  "\n  cat ...         : show content of a file (only if in insecure mode)"                                                                \
  "\n  load            : load properties in persistent fs (mainly for credentials)"                                                        \
  "\n  store           : save properties in persistent fs (mainly for credentials)"                                                        \
  "\n  save ...        : save a file <f> with content <y> in persistent fs (mainly for tuning) "                                           \
  "\n  help            : show this help"                                                                                                   \
  "\n  (all messages are shown as info log level)"                                                                                         \
  "\n"

enum ModuleStartupPropertiesCode {
  ModuleStartupPropertiesCodeSuccess = 0,
  ModuleStartupPropertiesCodeSkipped,
  ModuleStartupPropertiesCodePropertiesSyncFailure,
  ModuleStartupPropertiesCodeClockSyncFailure,
  ModuleStartupPropertiesCodeDelimiter
};

class StartupStatus {
public: 
  ModuleStartupPropertiesCode startupCode;
  BotMode botMode;
  StartupStatus(ModuleStartupPropertiesCode s, BotMode m) {
    startupCode = s;
    botMode = m;
  }
};

/**
 * This class represents the integration of all components (LCD, buttons, buzzer, etc).
 */

class Settings;

class Module {

private:
  PropSync *propSync;
  ClockSync *clockSync;
  Array<Actor *> *actors;
  Clock *clock;
  Settings *settings;
  SerBot *bot;
  const char *description;

  // Initialization of wifi.
  bool (*initWifi)();

  // Stop wifi.
  void (*stopWifi)();

  // Clear device function (remove filesystem, stacktraces, logs, etc...).
  void (*clearDevice)();

  // Sleep function (must be interruptable, return true if interrupted).
  bool (*sleepInterruptable)(time_t cycleBegin, time_t periodSec);

  // Deep sleep function not interruptable (after sleep it resets)
  void (*deepSleepNotInterruptable)(time_t cycleBegin, time_t periodSec);

  // Run one cycle for the being on configure mode (no actors cycle in this mode).
  void (*cycleConfigureMode)();

  // Run one cycle for the being on run mode (before actors cycle).
  void (*preCycleRunMode)();

  // Function that executes a command from the underlying architecture point of view.
  CmdExecStatus (*commandArchitecture)(const char *cmd);

  // File read function.
  bool (*fileRead)(const char *fname, Buffer *content);

  // File write function.
  bool (*fileWrite)(const char *fname, const char *content);

  // Information display function (version, general status, battery, etc.).
  void (*info)();

  // HW test.
  void (*test)();

  // Firmware update.
  void (*update)(const char *targetVersion, const char *currentVersion);

  // Retrieve the login for the main4ino API.
  const char *(*apiDeviceLogin)();

  // Retrieve the password for the main4ino API.
  const char *(*apiDevicePass)();

  // Regular HTTP request.
  HttpResponse (*httpMethod)(HttpMethod m, const char *url, Stream *body, Table *headers, const char *fingerprint);

  // Defines if this module is aimed at a single execution (some devices need this, as deep sleep modes equal full restarts).
  bool (*oneRunMode)();

  // Defines how to retrieve the buffer containing the logs to be pushed (if any)
  Buffer* (*getLogBuffer)();
   
  // Methods depending on Settings definition (to break cyclic dependency)
  void initSettings();
  bool inDebugMode();
  void setupSettings(const char *project, const char *platform);
  Timing* getBatchTiming();
  void updateIfMust();
  void setSsid(const char* c);
  const char* getSsid();
  void setPass(const char* c);
  const char* getPass();
  void setSsidBackup(const char* c);
  const char* getSsidBackup();
  void setPassBackup(const char* c);
  const char* getPassBackup();

  /**
   * Core of mod4ino
   *
   * Module that provides:
   * - time keeping and sync services (via Clock and ClockSync classes from main4ino)
   * - property synchronization services (via PropSync class from main4ino)
   *
   * Use as follows:
   *
   *   Module* module = new Module();
   *   module->getActors()->add(n, (Actor *)actor1, ...);
   *   module->setup(...);
   *   ModuleStartupPropertiesCode s = module->startupProperties(...);
   *   if (s.startupCode == ModuleStartupPropertiesCodeSuccess) {
   *     while (true) {
   *       module->loop();
   *     }
   *   }
   *
   */
public:
  Module() {
    actors = new Array<Actor *>;

    initSettings();
    propSync = new PropSync("propsync");
    clockSync = new ClockSync("clocksync");
    clock = new Clock("clock");

    actors->add(4, (Actor *)settings, (Actor *)propSync, (Actor *)clockSync, (Actor *)clock);

    bot = new SerBot(clock, actors);

    initWifi = NULL;
    stopWifi = NULL;
    clearDevice = NULL;
    httpMethod = NULL;
    sleepInterruptable = NULL;
    deepSleepNotInterruptable = NULL;
    cycleConfigureMode = NULL;
    preCycleRunMode = NULL;
    commandArchitecture = NULL;
    fileRead = NULL;
    fileWrite = NULL;
    info = NULL;
    test = NULL;
    update = NULL;
    apiDeviceLogin = NULL;
    apiDevicePass = NULL;
    oneRunMode = NULL;
    getLogBuffer = NULL;

    description = NULL;
  }

private: bool pushLogs() {

    if (getLogBuffer == NULL || getLogBuffer() == NULL) 
      return true;

    if (!inDebugMode())
      return true;

    int len = getLogBuffer()->getLength();
    log(CLASS_MODULE, Debug, "PLogs(%d)...", len);
    PropSyncStatusCode status = getPropSync()->pushLogMessages(getLogBuffer()->getBuffer());
    if (getPropSync()->isFailure(status)) {
      log(CLASS_MODULE, Warn, "PLogs KO");
      return false;
    } else {
      log(CLASS_MODULE, Debug, "PLogs OK");
      getLogBuffer()->clear();
      return true;
    }

  }

public:
  /**
   * Setup this module and the core components and the architecture.
   * 
   * WARNING: none of the passed functions should depend on this instance being already initialized. 
   */
  void setup(
             const char *project,
             const char *platform,
             bool (*initWifiFunc)(),
             void (*stopWifiFunc)(),
             HttpResponse (*httpMethodFunc)(HttpMethod m, const char *url, Stream *body, Table *headers, const char *fingerprint),
             void (*clearDeviceFunc)(),
             bool (*fileReadFunc)(const char *fname, Buffer *content),
             bool (*fileWriteFunc)(const char *fname, const char *content),
             bool (*sleepInterruptableFunc)(time_t cycleBegin, time_t periodSec),
             void (*deepSleepNotInterruptableFunc)(time_t cycleBegin, time_t periodSec),
             void (*cycleConfigureModeFunc)(),
             void (*preCycleRunModeFunc)(),
             CmdExecStatus (*commandArchitectureFunc)(const char *cmd),
             void (*infoFunc)(),
             void (*updateFunc)(const char *targetVersion, const char *currentVersion),
             void (*testFunc)(),
             const char *(*apiDeviceLoginFunc)(),
             const char *(*apiDevicePassFunc)(),
             bool (*oneRunModeFunc)(),
             Buffer* (*getLogBufferFunc)()
             ) {

    initWifi = initWifiFunc;
    stopWifi = stopWifiFunc;
    clearDevice = clearDeviceFunc;
    sleepInterruptable = sleepInterruptableFunc;
    deepSleepNotInterruptable = deepSleepNotInterruptableFunc;
    cycleConfigureMode = cycleConfigureModeFunc;
    preCycleRunMode = preCycleRunModeFunc;
    commandArchitecture = commandArchitectureFunc;
    fileRead = fileReadFunc;
    fileWrite = fileWriteFunc;
    info = infoFunc;
    test = testFunc;
    update = updateFunc;
    apiDeviceLogin = apiDeviceLoginFunc;
    apiDevicePass = apiDevicePassFunc;
    httpMethod = httpMethodFunc;
    oneRunMode = oneRunModeFunc;
    getLogBuffer = getLogBufferFunc;

    propSync->setup(bot, initWifi, httpMethod, fileRead, fileWrite);
    clockSync->setup(bot->getClock(), initWifi, httpMethod);

    setupSettings(project, platform);
  }

private: 
  StartupStatus failed(const char *msg, ModuleStartupPropertiesCode code) {
     log(CLASS_MODULE, Error, "SU: %s", msg);
     return StartupStatus(code, ConfigureMode);
  }

public:
  void setDescription(const char *d) {
    // Example:
    // {"version":"1.0.0","json":[{"patterns": ["^.*.freq$"], "descriptions: ["Description here."],"examples": ["value -> Explanation"]}],
    description = d;
  }

private:
  void loadFsProps() {

    log(CLASS_MODULE, Info, "LGProps/FS");
    getPropSync()->fsLoadActorsProps();

    log(CLASS_MODULE, Info, "LM4INOProps/FS");
    getPropSync()->setLoginPass(apiDeviceLogin(), apiDevicePass()); // may override credentials loaded in steps above
    getClockSync()->setLoginPass(apiDeviceLogin(), apiDevicePass());
  }


  /**
   * Start up all module's properties
   *
   * Retrieve credentials and other properties from FS and server and report actual ones.
   *
   * 0. Load properties from the file system (general properties/credentials not related to the framework).
   * 1. Load properties from the file system (credentials related to the framework).
   * 2. Pull/push properties from/to the server
   * 3. Set time of actors with last known time
   * 4. Find out real current time
   * 5. Return success if properties and clock sync went well
   *
   */
public:
  StartupStatus startupProperties() {

    loadFsProps();

    log(CLASS_MODULE, Info, "SyncM4INO");
    bool oneRun = oneRunModeSafe();
    PropSyncStatusCode serSyncd = PropSyncStatusCodeUnknown;
    if (oneRun) {
      log(CLASS_MODULE, Info, "(1run)Pull");
      serSyncd = getPropSync()->pullActors(); // only pull, push is postponed
    } else {
      log(CLASS_MODULE, Info, "(!1run)Pull&push");
      serSyncd = getPropSync()->pullPushActors(false); // sync properties from the server
    }

    if (getPropSync()->isFailure(serSyncd)) {
      return failed("PSync KO", ModuleStartupPropertiesCodePropertiesSyncFailure);
    }

    if (description != NULL) {
      log(CLASS_MODULE, Info, "SDesc");
      getPropSync()->pushDescription(description);
    } else {
      log(CLASS_MODULE, Debug, "No SDesc");
    }

    time_t leftTime = getBot()->getClock()->currentTime();

    Buffer timeAux(19);
    log(CLASS_MODULE, Info, "<Tims:%s", Timing::humanize(leftTime, &timeAux));
    getBot()->setActorsTime(leftTime);

    log(CLASS_MODULE, Info, "SClock");
    // sync real date / time on clock, block if a single run is requested
    bool freezeTime = oneRun;
    bool clockSyncd = getClockSync()->syncClock(freezeTime, DEFAULT_CLOCK_SYNC_ATTEMPTS);
    log(CLASS_MODULE, Info, ">Tims:%s", Timing::humanize(getBot()->getClock()->currentTime(), &timeAux));
    if (!clockSyncd) {
      return failed("SClock KO", ModuleStartupPropertiesCodeClockSyncFailure);
    }

    log(CLASS_MODULE, Info, "STUP OK");

    pushLogs();

    log(CLASS_MODULE, Debug, "Letting user interrupt...");
    bool i = sleepInterruptable(now(), SLEEP_PERIOD_UPON_BOOT_SECS);
    if (i) {
      return StartupStatus(ModuleStartupPropertiesCodeSuccess, ConfigureMode);
    } else {
      return StartupStatus(ModuleStartupPropertiesCodeSuccess, RunMode);
    }

  }

  /**
   * Start up all module's properties (light, minimum interactions with server)
   *
   * Retrieve credentials and other properties from FS and server and report actual ones.
   *
   * 0. Load properties from the file system (general properties/credentials not related to the framework).
   * 1. Load properties from the file system (credentials related to the framework).
   *
   */
public:
  StartupStatus startupPropertiesLight() {

    loadFsProps();
    pushLogs();

    log(CLASS_MODULE, Debug, "Letting user interrupt...");
    bool i = sleepInterruptable(now(), SLEEP_PERIOD_UPON_BOOT_SECS);
    if (i) {
      return StartupStatus(ModuleStartupPropertiesCodeSuccess, ConfigureMode);
    } else {
      return StartupStatus(ModuleStartupPropertiesCodeSuccess, RunMode);
    }

  }


  /**
   * Handle a user command.
   * If no command maches, commandArchitecture will be used as fallback.
   */
public:
  CmdExecStatus command(const char *cmd) {

    Buffer b(cmd);
    log(CLASS_MODULE, Info, "\n> %s\n", b.getBuffer());

    if (b.getLength() == 0) {
      return NotFound;
    }

    char *c = strtok(b.getUnsafeBuffer(), " ");

    if (strcmp("set", c) == 0) {
      const char *actor = strtok(NULL, " ");
      const char *prop = strtok(NULL, " ");
      const char *v = strtok(NULL, " ");
      if (actor == NULL || prop == NULL || v == NULL) {
        logRaw(CLASS_MODULE, User, "Arguments needed:\n  set <actor> <prop> <value>");
        return InvalidArgs;
      }
      log(CLASS_MODULE, Info, "-> Set %s.%s = %s", actor, prop, v);
      Buffer value(64, v);
      bot->setProp(actor, prop, &value);
      return Executed;
    } else if (strcmp("get", c) == 0) {
      log(CLASS_MODULE, Info, "-> Get");
      const char *actor = strtok(NULL, " ");
      getProps(actor);
      return Executed;
    } else if (strcmp("int", c) == 0) {
      log(CLASS_MODULE, Info, "Interrupt");
      return ExecutedInterrupt;
    } else if (strcmp("mode", c) == 0) {
      const char *m = strtok(NULL, " ");
      if (m == NULL) {
        log(CLASS_MODULE, User, "Mode: %s", getBot()->getModeName());
        return Executed;
      } else if (strcmp("run", m) == 0) {
        log(CLASS_MODULE, Info, "-> Run mode");
        runCmd();
        return ExecutedInterrupt;
      } else if (strcmp("conf", m) == 0) {
        log(CLASS_MODULE, Info, "-> Configure mode");
        confCmd();
        return ExecutedInterrupt;
      } else {
        return InvalidArgs;
      }
    } else if (strcmp("info", c) == 0) {
      infoCmd();
      return Executed;
    } else if (strcmp("test", c) == 0) {
      test();
      return Executed;
    } else if (strcmp("update", c) == 0) {
      const char *targetVersion = strtok(NULL, " ");
      if (targetVersion == NULL) {
        logRaw(CLASS_MODULE, User, "Arguments needed:\n  update <targetVersion>");
        return InvalidArgs;
      }
      update(targetVersion, STRINGIFY(PROJ_VERSION));
      return Executed;
    } else if (strcmp("clear", c) == 0) {
      clearDevice();
      return Executed;
    } else if (strcmp("logl", c) == 0) {
      c = strtok(NULL, " ");
      if (c == NULL) {
        char ll = getLogLevel();
        log(CLASS_MODULE, User, "Log level: %d", ll);
      } else {
        int ll = atoi(c);
        setLogLevel(ll);
        log(CLASS_MODULE, User, "Log level: %d", ll);
      }
      log(CLASS_MODULE, User, "Visible from now on:");
      log(CLASS_MODULE, Error, "- Error");
      log(CLASS_MODULE, Warn, "- Warn");
      log(CLASS_MODULE, Info, "- Info");
      log(CLASS_MODULE, Debug, "- Debug");
      return Executed;
    } else if (strcmp("logo", c) == 0) {
      c = strtok(NULL, " ");
      if (c == NULL) {
        log(CLASS_MODULE, User, "Log options: %s", (getLogOptions()==NULL?"":getLogOptions()));
        return Executed;
      } else {
        setLogOptions(c);
        log(CLASS_MODULE, User, "Set log options: %s", c);
      }
      log(CLASS_MODULE, User, "Visible from now on:");
      log(CLASS_MODULE, Error, "- Error");
      log(CLASS_MODULE, Warn, "- Warn");
      log(CLASS_MODULE, Info, "- Info");
      log(CLASS_MODULE, Debug, "- Debug");
      return Executed;
    } else if (strcmp("wifissid", c) == 0) {
      c = strtok(NULL, " ");
      if (c == NULL) {
        logRaw(CLASS_MODULE, User, "Argument needed:\n  wifissid <ssid>");
        return InvalidArgs;
      }
      setSsid(c);
      log(CLASS_MODULE, Info, "Wifi ssid: %s", getSsid());
      return Executed;
    } else if (strcmp("wifipass", c) == 0) {
      c = strtok(NULL, " ");
      if (c == NULL) {
        logRaw(CLASS_MODULE, User, "Argument needed:\n  wifipass <pass>");
        return InvalidArgs;
      }
      setPass(c);
      log(CLASS_MODULE, Info, "Wifi pass: %s", getPass());
      return Executed;
    } else if (strcmp("wifissidb", c) == 0) {
      c = strtok(NULL, " ");
      if (c == NULL) {
        logRaw(CLASS_MODULE, User, "Argument needed:\n  wifissidb <ssid>");
        return InvalidArgs;
      }
      setSsidBackup(c);
      log(CLASS_MODULE, Info, "Wifi ssidb: %s", getSsidBackup());
      return Executed;
    } else if (strcmp("wifipassb", c) == 0) {
      c = strtok(NULL, " ");
      if (c == NULL) {
        logRaw(CLASS_MODULE, User, "Argument needed:\n  wifipassb <pass>");
        return InvalidArgs;
      }
      setPassBackup(c);
      log(CLASS_MODULE, Info, "Wifi passb: %s", getPassBackup());
      return Executed;
    } else if (strcmp("wifi", c) == 0) {
      initWifi();
      return Executed;
    } else if (strcmp("wifistop", c) == 0) {
      stopWifi();
      return Executed;
    } else if (strcmp("actall", c) == 0) {
      actall();
      return Executed;
    } else if (strcmp("touchall", c) == 0) {
      touchall();
      return Executed;
    } else if (strcmp("actone", c) == 0) {
      c = strtok(NULL, " ");
      if (c == NULL) {
        logRaw(CLASS_MODULE, User, "Argument needed:\n  actone <actorname>");
        return InvalidArgs;
      }
      actone(c);
      return Executed;
    } else if (strcmp("store", c) == 0) {
      propSync->fsStoreActorsProps(); // store credentials
      log(CLASS_MODULE, Info, "Properties stored locally");
      return Executed;
    } else if (strcmp("save", c) == 0) {
      const char *fname = strtok(NULL, " ");
      const char *content = strtok(NULL, "?????");
      bool suc = fileWrite(fname, content);
      log(CLASS_MODULE, Info, "File '%s' saved: %d", fname, (int)suc);
      return Executed;
#ifdef INSECURE
    } else if (strcmp("cat", c) == 0) { // could be potentially used to display credentials
      const char *f = strtok(NULL, " ");
      if (f == NULL) {
        logRaw(CLASS_MODULE, User, "Argument needed:\n  cat <file>");
        return InvalidArgs;
      }
      Buffer buf(CAT_BUFFER_LENGTH);
      fileRead(f, &buf);
      log(CLASS_MODULE, User, "### File: %s", f);
      logRaw(CLASS_MODULE, User, buf.getBuffer());
      log(CLASS_MODULE, User, "###");
      return Executed;
#endif // INSECURE
    } else if (strcmp("load", c) == 0) {
      propSync->fsLoadActorsProps(); // load mainly credentials already set
      log(CLASS_MODULE, Info, "Properties loaded from local copy");
      return Executed;
    } else if (strcmp("help", c) == 0 || strcmp("?", c) == 0) {
      logRaw(CLASS_MODULE, User, HELP_COMMAND_CLI);
      return commandArchitecture("?");
    } else {
      return commandArchitecture(c);
    }
  }

public:
  void cycleBot(bool mode, bool set, bool cycle) {
    TimingInterrupt interruptType = TimingInterruptNone;
    if (cycle) {
      interruptType = TimingInterruptCycle;
    }
    // execute a cycle on the bot
    bot->cycle(mode, set, interruptType);
  }

  // All getters should be removed, and initialization of these instances below should
  // be done in Module itself. This should help decrease the size of
public:
  SerBot *getBot() {
    return bot;
  }

public:
  Clock *getClock() {
    return clock;
  }

public:
  ClockSync *getClockSync() {
    return clockSync;
  }

public:
  PropSync *getPropSync() {
    return propSync;
  }

public:
  Settings *getSettings() {
    return settings;
  }

  /**
   * Make all actors act
   */
public:
  void actall() {
    for (unsigned int i = 0; i < getBot()->getActors()->size(); i++) {
      Actor *a = getBot()->getActors()->get(i);
      log(CLASS_MODULE, Info, "Oneoff:%s", a->getName());
      a->oneOff();
    }
  }

  /**
   * Touch all actors (to force them to be syncrhonized)
   */
public:
  void touchall() {
    for (unsigned int i = 0; i < getBot()->getActors()->size(); i++) {
      Actor *a = getBot()->getActors()->get(i);
      Metadata *m = a->getMetadata();
      log(CLASS_MODULE, Info, "Touch:%s", a->getName());
      m->changed();
    }
  }

  /**
   * Make a given by-name-actor act
   */
public:
  void actone(const char *actorName) {
    for (unsigned int i = 0; i < getBot()->getActors()->size(); i++) {
      Actor *a = getBot()->getActors()->get(i);
      if (strcmp(a->getName(), actorName) == 0) {
        log(CLASS_MODULE, Info, "Oneoff:%s", a->getName());
        a->oneOff();
      }
    }
  }

public:
  void runCmd() {
    bot->setMode(RunMode);
  }

public:
  void confCmd() {
    bot->setMode(ConfigureMode);
  }

public:
  void infoCmd() {
    info();
  }

public:
  void getProps(const char *actorN) {
    Buffer contentAuxBuffer(64);
    Array<Actor *> *actors = bot->getActors();
    for (unsigned int i = 0; i < actors->size(); i++) {
      Actor *actor = actors->get(i);
      log(CLASS_MODULE, Info, "# '%s'", actor->getName());
      if (actorN == NULL || strcmp(actor->getName(), actorN) == 0) {
        for (int p = 0; p < actor->getNroProps(); p++) {
          const char *propName = actor->getPropName(p);
          if (propName[0] == SENSITIVE_PROP_PREFIX[0]) { // sensitive
            contentAuxBuffer.fill("(sensitive)");
          } else {
            actor->getPropValue(p, &contentAuxBuffer);
          }
          log(CLASS_MODULE, Info, " '%s'='%s'", propName, contentAuxBuffer.getBuffer());
        }
      }
    }
  }

public:
  void configureMode() {
    cycleConfigureMode();
  }

public:
  void runMode() {
    preCycleRunMode();
    cycleBot(false, false, true);
  }

private:
  bool oneRunModeSafe() {
    return (oneRunMode == NULL ? false : oneRunMode());
  }

public:
  Array<Actor *> *getActors() {
    return actors;
  }

private:
  time_t durationToDeepSleep() {
    Timing* timing = getBatchTiming();
    time_t toMatch = timing->secsToMatch(MAX_BATCH_PERIOD_SECS);
    time_t fromMatch = timing->secsFromMatch(MAX_BATCH_PERIOD_SECS);
    if (toMatch < fromMatch) { // toMatch is closer, it's the good timing, is in the future, i.e. deep sleep completed faster than it should have
      log(CLASS_MODULE, Debug, "DS (|<---%lu   ^%lu->|)", (unsigned long)fromMatch, (unsigned long)toMatch);
      Timing tAlmost;
      tAlmost.setCurrentTime(timing->getCurrentTime() + toMatch + 1);
      tAlmost.setFreq(timing->getFreq());
      time_t toMatchAfter = tAlmost.secsToMatch(MAX_BATCH_PERIOD_SECS);
      time_t yield = toMatch + toMatchAfter;
      log(CLASS_MODULE, Debug, "uC faster, DS: (%lu+)%lu", (unsigned long)toMatch, (unsigned long)toMatchAfter);
      return yield;
    } else { // our timing is in the past, uC was slower and passed the good timing
      log(CLASS_MODULE, Debug, "DS (|<-%lu^   %lu--->|)", (unsigned long)fromMatch, (unsigned long)toMatch);
      log(CLASS_MODULE, Debug, "uC slower, DS: %lu", (unsigned long)toMatch);
      return toMatch;
    }
  }

public:
  void loop() {
    time_t cycleBegin = now();
    switch (getBot()->getMode()) {
      case (RunMode):
        log(CLASS_MODULE, Info, "#LOOP(%s)", STRINGIFY(PROJ_VERSION));
        runMode();
        log(CLASS_MODULE, Info, "#ENDLOOP");
        if (oneRunModeSafe()) {
          // before finishing store in the server the last status of all actors
          // this includes the timing of the clock, that has progressed
          // and will allow the next run to start from where we left off
          log(CLASS_MODULE, Info, "Push(1run)");
          // push properties to the server (with new props and new clock blocked timing)
          getPropSync()->pushActors(true);
          time_t s = durationToDeepSleep();
          pushLogs();
          updateIfMust();
          deepSleepNotInterruptable(cycleBegin, s);
        } else {
          pushLogs();
          updateIfMust();
          sleepInterruptable(cycleBegin, getBatchTiming()->secsToMatch(MAX_BATCH_PERIOD_SECS));
        }
        break;
      case (ConfigureMode):
        configureMode();
        sleepInterruptable(cycleBegin, PERIOD_CONFIGURE_SEC);
        break;
      default:
        break;
    }
  }
};

#endif // MODULE_INC
