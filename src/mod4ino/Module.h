#ifndef MODULE_INC
#define MODULE_INC

#include <log4ino/Log.h>
#include <main4ino/Actor.h>
#include <main4ino/Array.h>
#include <main4ino/Clock.h>
#include <main4ino/PropSync2.h>
#include <main4ino/ClockSync.h>
#include <main4ino/HttpMethods.h>
#include <main4ino/HttpResponse.h>
#include <main4ino/Authenticable.h>
#include <main4ino/SerBot.h>
#include <main4ino/Table.h>
#include <main4ino/Device.h>
#include <mod4ino/MsgClearMode.h>
#include <mod4ino/Settings.h>
#include <main4ino/CmdExecStatus.h>
#include <main4ino/Cmd.h>

#define CLASS_MODULE "MO"

#define PERIOD_CONFIGURE_SEC 1
#define MAX_BATCH_PERIOD_SECS 172800 // 2 days

#ifndef SLEEP_PERIOD_UPON_BOOT_SECS
#define SLEEP_PERIOD_UPON_BOOT_SECS 2
#endif // SLEEP_PERIOD_UPON_BOOT_SECS

#ifndef CAT_BUFFER_LENGTH
#define CAT_BUFFER_LENGTH 512
#endif // CAT_BUFFER_LENGTH

#ifndef ERR_BUFFER_LENGTH
#define ERR_BUFFER_LENGTH 64
#endif // ERR_BUFFER_LENGTH

#define LOG_CAPACITY_THRESHOLD 0.8
#define LOG_PLOG_REPORT_LENGTH 32
#define LOGS_PUSH_CONF_COUNT 10

// convention for firmware file name: firmware-<version>.<platform>.bin
// to replace: base + project + version + platform
#define FIRMWARE_UPDATE_URL MAIN4INOSERVER_API_HOST_BASE "/api/v1/session/%s/devices/%s/firmware/firmwares/%s/%s/content?version=%s"


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
  Buffer msg;
  StartupStatus(ModuleStartupPropertiesCode s, BotMode m, Buffer u) {
    startupCode = s;
    botMode = m;
    msg = u;
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
  Device *device;
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
  CmdExecStatus (*commandPlatform)(Cmd *cmd);

  // Function that executes a command from the project using this module
  std::function<CmdExecStatus (Cmd* cmd)> commandProjectExtended;

  // File read function.
  bool (*fileRead)(const char *fname, Buffer *content);

  // File write function.
  bool (*fileWrite)(const char *fname, const char *content);

  // Information display function (version, general status, battery, etc.).
  void (*info)();

  // HW test.
  void (*test)();

  // Firmware update.
  void (*update)(const char *url, const char *currentVersion);

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
  void initDevice();
  bool inDebugMode();
  void setupSettings();
  Timing* getBatchTiming();
  void setSsid(const char* c);
  const char* getSsid();
  void setPass(const char* c);
  const char* getPass();
  void setSsidBackup(const char* c);
  const char* getSsidBackup();
  void setPassBackup(const char* c);
  const char* getPassBackup();

  std::function<CmdExecStatus (Cmd*)> commandPlatformFuncStd = [&](Cmd* cmd) {return commandPlatform(cmd);};
  std::function<CmdExecStatus (Cmd*)> commandProjectFuncStd = [&](Cmd* cmd) {return commandProject(cmd);};

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
    settings = new Settings("mod4ino");
    device = new Device("device");
    propSync = new PropSync("propsync");
    clockSync = new ClockSync("clocksync");
    clock = new Clock("clock");

    actors->add(5, (Actor *)device, (Actor *)settings, (Actor *)propSync, (Actor *)clockSync, (Actor *)clock);

    bot = new SerBot(clock, actors);

    initWifi = NULL;
    stopWifi = NULL;
    clearDevice = NULL;
    httpMethod = NULL;
    sleepInterruptable = NULL;
    deepSleepNotInterruptable = NULL;
    cycleConfigureMode = NULL;
    preCycleRunMode = NULL;
    commandPlatform = NULL;
    commandProjectExtended = NULL;
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

public: bool pushLogs() {

    if (getLogBuffer == NULL || getLogBuffer() == NULL) 
      return true;

    if (!inDebugMode())
      return true;

    sizet len = getLogBuffer()->getLength();
    if (len > getLogBuffer()->getEffCapacity() * LOG_CAPACITY_THRESHOLD) {
      if (getLogBuffer()->getEffCapacity() - len > LOG_PLOG_REPORT_LENGTH) {
        getLogBuffer()->drop(LOG_PLOG_REPORT_LENGTH);
      }
      log(CLASS_MODULE, Warn, "...PLog(%lu/%lu)...", (unsigned long)len, getLogBuffer()->getEffCapacity());
    } else {
      log(CLASS_MODULE, Info, "PLog(%lu/%lu)...", (unsigned long)len, getLogBuffer()->getEffCapacity());
    }
    PropSyncStatusCode status = getPropSync()->pushLogMessages(getLogBuffer()->getBuffer());
    if (getPropSync()->isFailure(status)) {
      log(CLASS_MODULE, Warn, "PLog KO");
      return false;
    } else {
      log(CLASS_MODULE, Info, "PLog OK");
      getLogBuffer()->clear();
      return true;
    }

  }

public: void updateFirmwareFromMain4ino(const char* url, const char *project, const char* platform, const char *targetVersion, const char* currentVersion) {
#ifndef UPDATE_FIRMWARE_MAIN4INO_DISABLED
  Buffer aux(UPDATE_FIRMWARE_URL_MAX_LENGTH);
  aux.fill(url, getPropSync()->getSession(), getPropSync()->getLogin(), project, platform, targetVersion);
  update(aux.getBuffer(), currentVersion);
#else // UPDATE_FIRMWARE_MAIN4INO_DISABLED
  log(CLASS_MODULE, Warn, "Update disabled");
#endif // UPDATE_FIRMWARE_MAIN4INO_DISABLED
}

public: void updateToProjectVersion(const char* project, const char *targetVersion) {
  bool c = initWifi();
  if (c) {
    updateFirmwareFromMain4ino(FIRMWARE_UPDATE_URL, project, STRINGIFY(PLATFORM_ID), targetVersion, STRINGIFY(PROJECT_VERSION));
  } else {
    log(CLASS_MODULE, Error, "Could not update");
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
             CmdExecStatus (*commandPlatformFunc)(Cmd *cmd),
             std::function<CmdExecStatus (Cmd* cmd)> commandProjectExtendedFunc,
             void (*infoFunc)(),
             void (*updateFunc)(const char* url, const char *currentVersion),
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
    commandPlatform = commandPlatformFunc;
    commandProjectExtended = commandProjectExtendedFunc;
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

    bot->setProjectCommand(commandProjectFuncStd);
    bot->setPlatformCommand(commandPlatformFuncStd);

    setupSettings();
  }

private: 
  StartupStatus failed(Buffer msg, ModuleStartupPropertiesCode code) {
     log(CLASS_MODULE, Error, "Startup failed: %s", msg.getBuffer());
     return StartupStatus(code, ConfigureMode, msg);
  }

public:
  void setDescription(const char *d) {
    // Example:
    // {"version":"1.0.0","json":[{"patterns": ["^.*.freq$"], "descriptions: ["Description here."],"examples": ["value -> Explanation"]}],
    description = d;
  }

private:
  void loadFsProps() {
    log(CLASS_MODULE, Info, "Load props (fs)");
    getPropSync()->fsLoadActorsProps();
    getPropSync()->setLoginPass(apiDeviceLogin(), apiDevicePass()); // may override credentials loaded in steps above
    getClockSync()->setLoginPass(apiDeviceLogin(), apiDevicePass());
  }


  /**
   * Start up all module's properties
   *
   * Retrieve credentials and other properties from FS and server and report actual ones.
   * By this point, only resolution and server-sync of properties of actors took place, 
   * no actual acting on them.
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

    log(CLASS_MODULE, Info, "Load props (fs)");
    loadFsProps();

    bool oneRun = oneRunModeSafe();
    PropSyncStatusCode serSyncd = PropSyncStatusCodeUnknown;
    if (oneRun) {
      log(CLASS_MODULE, Info, "Pull props (server)");
      serSyncd = getPropSync()->pullActors(); // only pull, push is postponed
    } else {
      log(CLASS_MODULE, Info, "Pull&push props (server)");
      serSyncd = getPropSync()->pullPushActors(false); // sync properties from the server
    }

    if (getPropSync()->isFailure(serSyncd)) {
      Buffer b(ERR_BUFFER_LENGTH);
      b.fill("Prop sync KO(%d:%s)", serSyncd, getPropSync()->statusDescription(serSyncd));
      return failed(b, ModuleStartupPropertiesCodePropertiesSyncFailure);
    } else {
      log(CLASS_MODULE, Info, "Store props (fs)");
      propSync->fsStoreActorsProps(); // store credentials
    }

    if (description != NULL) {
      log(CLASS_MODULE, Info, "Push descr.");
      getPropSync()->pushDescription(description);
    } else {
      log(CLASS_MODULE, Fine, "Skip push descr.");
    }

    log(CLASS_MODULE, Info, "Load time (fs)");
    time_t leftTime = getBot()->getClock()->currentTime();

    Buffer timeAux(19);
    log(CLASS_MODULE, Info, "Time (fs):%s", Timing::humanize(leftTime, &timeAux));
    getBot()->setActorsTime(leftTime);

    log(CLASS_MODULE, Info, "Load time (server)");
    // sync real date / time on clock, block if a single run is requested
    bool freezeTime = oneRun;
    bool clockSyncd = getClockSync()->syncClock(freezeTime, DEFAULT_CLOCK_SYNC_ATTEMPTS);
    log(CLASS_MODULE, Info, "Time (server):%s", Timing::humanize(getBot()->getClock()->currentTime(), &timeAux));
    if (!clockSyncd) {
      Buffer b(ERR_BUFFER_LENGTH);
      b.fill("Sync clock KO(%d)", clockSyncd);
      return failed(b, ModuleStartupPropertiesCodeClockSyncFailure);
    }

    log(CLASS_MODULE, Info, "Startup OK");

    pushLogs();

    if (getSettings()->getWaitOnBoot()) {
      log(CLASS_MODULE, Debug, "Wait on boot");
      bool i = sleepInterruptable(now(), SLEEP_PERIOD_UPON_BOOT_SECS);
      if (i) {
        Buffer b("Interrupted");
        return StartupStatus(ModuleStartupPropertiesCodeSuccess, ConfigureMode, b);
      } else {
        Buffer b("OK");
        return StartupStatus(ModuleStartupPropertiesCodeSuccess, RunMode, b);
      }
    } else {
      Buffer b("OK");
      return StartupStatus(ModuleStartupPropertiesCodeSuccess, RunMode, b);
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
      Buffer b("Interrupted");
      return StartupStatus(ModuleStartupPropertiesCodeSuccess, ConfigureMode, b);
    } else {
      Buffer b("OK");
      return StartupStatus(ModuleStartupPropertiesCodeSuccess, RunMode, b);
    }

  }


void logDemo() {
  log(CLASS_MODULE, User, "Visible from now on:");
  log(CLASS_MODULE, User, "- User");
  log(CLASS_MODULE, Error, "- Error");
  log(CLASS_MODULE, Warn, "- Warn");
  log(CLASS_MODULE, Info, "- Info");
  log(CLASS_MODULE, Debug, "- Debug");
}


public:
  CmdExecStatus command(Cmd *cmd) {
    return bot->command(cmd);
  }

private:
  CmdExecStatus commandProject(Cmd *c) {
    if (c->matches("setp", "set properties of an actor and store", 3, "actor", "property", "value")) {
      Buffer actor(32);
      Buffer prop(32);
      Buffer value(64);
      c->getArg(2, &value);
      bot->setProp(c->getArg(0, &actor), c->getArg(1, &prop), &value);
      propSync->fsStoreActorsProps();
      return Executed;
    } else if (c->matches("getp", "get properties of a given actor", 1, "actor")) {
      getProps(c->getAsLastArg(0));
      return Executed;
    } else if (c->matches("getp", "get properties of all actors", 0)) {
      getAllProps();
      return Executed;
    } else if (c->matches("inte", "interrupt current cycle", 0)) {
      return ExecutedInterrupt;
    } else if (c->matches("mode", "change to mode", 1, "[run|conf]")) {
      const char* m = c->getAsLastArg(0);
      if (strcmp("run", m) == 0) {
        log(CLASS_MODULE, Info, "-> run mode");
        runCmd();
        return ExecutedInterrupt;
      } else if (strcmp("conf", m) == 0) {
        log(CLASS_MODULE, Info, "-> conf mode");
        confCmd();
        return ExecutedInterrupt;
      } else {
        return InvalidArgs;
      }
    } else if (c->matches("info", "show information", 0)) {
      infoCmd();
      return Executed;
    } else if (c->matches("vers", "show version of the software", 0)) {
      logRaw(CLASS_MODULE, User, STRINGIFY(PROJ_VERSION));
      return Executed;
    } else if (c->matches("test", "perform tests", 0)) {
      test();
      return Executed;
    } else if (c->matches("upda", "update firmware", 1, "tgt-version")) {
      updateToProjectVersion(STRINGIFY(PROJECT_ID), c->getAsLastArg(0));
      return Executed;
#ifdef INSECURE
    } else if (c->matches("updc", "update firmware cross project", 2, "tgt-project", "tgt-version")) {
      Buffer bf(32);
      const char *project = c->getArg(0, &bf);
      const char *version = c->getAsLastArg(1);
      updateToProjectVersion(project, version);
      return Executed;
    } else if (c->matches("updd", "update firmware (development mode)", 1, "url")) {
      // example: http://10.0.0.11:8080/a.firmware
      update(c->getAsLastArg(0), STRINGIFY(PROJECT_VERSION));
      return Executed;
#endif // INSECURE
    } else if (c->matches("clea", "clear device", 0)) {
      clearDevice();
      return Executed;
    } else if (c->matches("glog", "get log options", 0)) {
      log(CLASS_MODULE, User, "Options: %s", (getLogOptions()==NULL?"":getLogOptions()));
      logDemo();
      return Executed;
    } else if (c->matches("slog", "set log options", 1, "options (example: ??F.)")) {
      setLogOptions(c->getAsLastArg(0));
      logDemo();
      return Executed;
    } else if (c->matches("wis1", "set ssid of main wifi network", 1, "ssid")) {
      setSsid(c->getAsLastArg(0));
      propSync->fsStoreActorsProps();
      return Executed;
    } else if (c->matches("wip1", "set pass of main wifi network", 1, "pass")) {
      setPass(c->getAsLastArg(0));
      propSync->fsStoreActorsProps();
      return Executed;
    } else if (c->matches("wis2", "set ssid of backup wifi network", 1, "ssid")) {
      setSsidBackup(c->getAsLastArg(0));
      propSync->fsStoreActorsProps();
      return Executed;
    } else if (c->matches("wip2", "set pass of backup wifi network", 1, "pass")) {
      setPassBackup(c->getAsLastArg(0));
      propSync->fsStoreActorsProps();
      return Executed;
    } else if (c->matches("wigo", "connect to wifi", 0)) {
      initWifi();
      return Executed;
    } else if (c->matches("wist", "disconnect from wifi", 0)) {
      stopWifi();
      return Executed;
    } else if (c->matches("acta", "make all devices act", 0)) {
      actall();
      return Executed;
    } else if (c->matches("tall", "touch all devices", 0)) {
      touchall();
      return Executed;
    } else if (c->matches("acto", "make the provided actor act", 1, "actor-name")) {
      actone(c->getAsLastArg(0));
      return Executed;
    } else if (c->matches("stor", "store all actors properties in FS", 0)) {
      propSync->fsStoreActorsProps(); // store credentials
      return Executed;
    } else if (c->matches("save", "save into the given file the provided content", 2, "file", "content")) {
      Buffer bf(32);
      const char *fname = c->getArg(0, &bf);
      const char *content = c->getAsLastArg(1);
      bool suc = fileWrite(fname, content);
      return Executed;
#ifdef INSECURE
    } else if (c->matches("cat", "show content of a file", 1, "filename")) { // could be used to display credentials, that's why insecure
      Buffer buf(CAT_BUFFER_LENGTH);
      fileRead(c->getAsLastArg(0), &buf);
      log(CLASS_MODULE, User, "### File: %s", c->getAsLastArg(0));
      logRaw(CLASS_MODULE, User, buf.getBuffer());
      log(CLASS_MODULE, User, "###");
      return Executed;
#endif // INSECURE
    } else if (c->matches("load", "load all actors properties from FS", 0)) {
      propSync->fsLoadActorsProps(); // load mainly credentials already set
      log(CLASS_MODULE, Info, "Properties loaded from local copy");
      return Executed;
    } else {
      return commandProjectExtended(c);
    }
  }

private:
  void actorAct(time_t currentTime, Actor *actor) {
    Timing *t = actor->getTiming();
    const char *actorName = actor->getName();

    if (t == NULL) {
      log(CLASS_MODULE, Error, "No Timing! %s", actor->getName());
      return;
    }

    log(CLASS_MODULE, Debug, "%.5s t%ld(+%ld)", actorName, t->getCurrentTime(), currentTime - t->getCurrentTime());
    while (t->catchesUp(currentTime)) {
      Act act = actor->act(actor->getMetadata());
      if (!act.isEmpty()) {
        act.iterate();
        Buffer cmd(COMMAND_MAX_LENGTH);
        while(act.command(&cmd) != -1) {
          Cmd c(&cmd);
          CmdExecStatus s = command(&c);
          if (s != Executed) {
            log(CLASS_MODULE, Error, "Cmd failed %s: %s", actor->getName(), cmd.getBuffer());
            return;
          }
        }
      }
    }
  }


private:
  void cycleBotRunMode() {
    time_t t = getClock()->currentTime();
    log(CLASS_MODULE, Info, "CYCLE: %04d-%02d-%02d %02d:%02d:%02d", GET_YEARS(t), GET_MONTHS(t), GET_DAYS(t), GET_HOURS(t), GET_MINUTES(t), GET_SECONDS(t));

    for (unsigned int aIndex = 0; aIndex < actors->size(); aIndex++) {
      Actor* actor = actors->get(aIndex);
      log(CLASS_MODULE, Info, "%s: act!", actor->getName());
      time_t currentTime = getClock()->currentTime();
      actorAct(currentTime, actor);
      pushLogs();
    }
  }

  // All getters should be removed if possible.
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

  Device *getDevice() {
    return device;
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
    log(CLASS_MODULE, User, "Platfor: %s", STRINGIFY(PLATFORM_ID));
    log(CLASS_MODULE, User, "Project: %s", STRINGIFY(PROJECT_ID));
    log(CLASS_MODULE, User, "Version: %s", STRINGIFY(PROJ_VERSION));
    info();
  }

public:

  void getAllProps() {
    Array<Actor *> *actors = bot->getActors();
    for (unsigned int i = 0; i < actors->size(); i++) {
      const char *aname = actors->get(i)->getName();
      getProps(aname);
    }
  }

  void getProps(const char *actorN) {
    Buffer contentAuxBuffer(64);
    Array<Actor *> *actors = bot->getActors();
    for (unsigned int i = 0; i < actors->size(); i++) {
      Actor *actor = actors->get(i);
      if (actorN == NULL || strcmp(actor->getName(), actorN) == 0) {
        log(CLASS_MODULE, Info, "# '%s'", actor->getName());
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
      log(CLASS_MODULE, Fine, "DS (|<---%lu   ^%lu->|)", (unsigned long)fromMatch, (unsigned long)toMatch);
      Timing tAlmost("almost", CLASS_MODULE);
      tAlmost.setCurrentTime(timing->getCurrentTime() + toMatch + 1);
      tAlmost.setFreq(timing->getFreq());
      time_t toMatchAfter = tAlmost.secsToMatch(MAX_BATCH_PERIOD_SECS);
      time_t yield = toMatch + toMatchAfter;
      log(CLASS_MODULE, Fine, "uC faster, DS: (%lu+)%lu", (unsigned long)toMatch, (unsigned long)toMatchAfter);
      return yield;
    } else { // our timing is in the past, uC was slower and passed the good timing
      log(CLASS_MODULE, Fine, "DS (|<-%lu^   %lu--->|)", (unsigned long)fromMatch, (unsigned long)toMatch);
      log(CLASS_MODULE, Fine, "uC slower, DS: %lu", (unsigned long)toMatch);
      return toMatch;
    }
  }

public:
  void loop() {
    time_t cycleBegin = now();
    switch (getBot()->getMode()) {
      case (RunMode): {
          log(CLASS_MODULE, Info, "#LOOP(%s)", STRINGIFY(PROJ_VERSION));

          preCycleRunMode();
          pushLogs();
          cycleBotRunMode();

          log(CLASS_MODULE, Info, "#ENDLOOP");
          pushLogs();
          log(CLASS_MODULE, Info, "Storing actors...");
          getPropSync()->fsStoreActorsProps(); // store credentials
          pushLogs();
          if (getBot()->getMode() != RunMode) {
            log(CLASS_MODULE, Debug, "No longer run mode!");
          } else if (oneRunModeSafe()) {
            // before finishing store in the server the last status of all actors
            // this includes the timing of the clock, that has progressed
            // and will allow the next run to start from where we left off
            log(CLASS_MODULE, Info, "Push(1run)");
            // push properties to the server (with new props and new clock blocked timing)
            getPropSync()->pushActors(true);
            time_t s = durationToDeepSleep();
            log(CLASS_MODULE, Info, "DS:%lu", (unsigned long)s);
            pushLogs();
            updateIfMust();
            deepSleepNotInterruptable(cycleBegin, s);
          } else {
            time_t s = getBatchTiming()->secsToMatch(MAX_BATCH_PERIOD_SECS);
            log(CLASS_MODULE, Fine, "LS:%lu", (unsigned long)s);
            pushLogs();
            updateIfMust();
            sleepInterruptable(cycleBegin, s);
          }
        }
        break;
      case (ConfigureMode): {
          static int confCnt = 0;
          confCnt++;
          cycleConfigureMode();
          if (confCnt % LOGS_PUSH_CONF_COUNT == 0) {
            pushLogs();
          }
          sleepInterruptable(cycleBegin, PERIOD_CONFIGURE_SEC);
        }
        break;
      default:
        break;
    }
  }

private: void updateIfMust() {
    if (getSettings()->isUpdateScheduled()) {
      if (!getSettings()->getTarget()->equals(SKIP_UPDATES_CODE)) {
        log(CLASS_SETTINGS, Warn, "Update:'%s'->'%s'", STRINGIFY(PROJ_VERSION), getSettings()->getTarget()->getBuffer());
        if (update != NULL) {
          PropSyncStatusCode st = getPropSync()->pushActors(true); // push properties to the server
          if (!getPropSync()->isFailure(st)) {
            updateToProjectVersion(STRINGIFY(PROJECT_ID), getSettings()->getTarget()->getBuffer()); // update
            getSettings()->setUpdateScheduled(false); // in case update failed, forget the attempt
          } else {
            log(CLASS_MODULE, Warn, "Update skipped(%d)", (int)st);
          }
        } else {
          log(CLASS_MODULE, Warn, "No init.");
        }
      }
    }
  }

};

#endif // MODULE_INC
