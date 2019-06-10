#ifndef MODULE_INC
#define MODULE_INC

#include <log4ino/Log.h>
#include <main4ino/Actor.h>
#include <main4ino/Array.h>
#include <main4ino/Clock.h>
#include <main4ino/ClockSync.h>
#include <main4ino/PropSync.h>
#include <main4ino/SerBot.h>
#include <main4ino/Table.h>
#include <mod4ino/MsgClearMode.h>
#include <mod4ino/Settings.h>

#define CLASS_MODULE "MD"

#define COMMAND_MAX_LENGTH 128

#define PERIOD_CONFIGURE_MSEC 4000

#define HELP_COMMAND_CLI                                                                                                                   \
  "\n  MODULE HELP"                                                                                                                        \
  "\n  int             : interrupt current ongoing action"                                                                                 \
  "\n  run             : go to run mode"                                                                                                   \
  "\n  conf            : go to conf mode"                                                                                                  \
  "\n  info            : show info about the device"                                                                                       \
  "\n  test            : test the architecture/hardware"                                                                                   \
  "\n  update ...      : update the firmware with the given descriptor"                                                                                              \
  "\n  wifi            : init steady wifi"                                                                                                 \
  "\n  get             : display actors properties"                                                                                        \
  "\n  get ...         : display actor <actor> properties"                                                                                 \
  "\n  set ...         : set an actor property (example: 'set body msg0 HELLO')"                                                           \
  "\n  logl [...]      : get / change log level to <x> (0 is more verbose, to 3 least verbose)"                                            \
  "\n  clear           : clear device (filesystem, crashes stacktrace, etc.)"                                                              \
  "\n  actall          : all act"                                                                                                          \
  "\n  touchall        : mark actors as 'changed' to force synchronization with the server"                                                \
  "\n  actone ...      : make actor <x> act"                                                                                               \
  "\n  wifissid ...    : set wifi ssid"                                                                                                    \
  "\n  wifipass ...    : set wifi pass"                                                                                                    \
  "\n  cat ...         : show content of a file (only if in insecure mode)"                                                              \
  "\n  load            : load properties in persistent fs (mainly for credentials)"                                                        \
  "\n  store           : save properties in persistent fs (mainly for credentials)"                                                        \
  "\n  save ...        : save a file <f> with content <y> in persistent fs (mainly for tuning) "                                           \
  "\n  help            : show this help"                                                                                                   \
  "\n  (all messages are shown as info log level)"                                                                                         \
  "\n"

enum CmdExecStatus { NotFound = 0, InvalidArgs, Executed, ExecutedInterrupt, CmdFailed };
#define CMD_EXEC_STATUS(s) (s == NotFound? "Not found": (s == InvalidArgs? "Invalid args": (s == Executed? "Executed": (s == ExecutedInterrupt? "Executed w/int": ("Cmd failed")))))

/**
 * This class represents the integration of all components (LCD, buttons, buzzer, etc).
 */
class Module {

private:
  PropSync *propSync;
  ClockSync *clockSync;
  Array<Actor *> *actors;
  Clock *clock;
  Settings *settings;
  SerBot *bot;

  bool (*initWifi)();
  void (*clearDevice)();
  bool (*sleepInterruptable)(time_t cycleBegin, time_t periodSec);
  void (*configureModeArchitecture)();
  void (*runModeArchitecture)();
  CmdExecStatus (*commandArchitecture)(const char *cmd);
  bool (*fileRead)(const char *fname, Buffer *content);
  bool (*fileWrite)(const char *fname, const char *content);
  void (*info)();
  void (*test)();
  void (*update)(const char* descriptor);
  const char *(*apiDeviceLogin)();
  const char *(*apiDevicePass)();
  int (*httpPost)(const char *url, const char *body, ParamStream *response, Table *headers);
  int (*httpGet)(const char *url, ParamStream *response, Table *headers);

  /**
   * Synchronize with the server.
   *
   * 0. Load properties from the file system (credentials related to the framework).
   * 1. Load properties from the file system (other properties/credentials not related to the framework).
   * 2. Pull/push properties from/to the server
   * 3. Set time of actors with last known time
   * 4. Find out real current time
   * 5. Return success if properties and clock sync went well
   *
   */
  bool sync() {

    log(CLASS_MODULE, Info, "# Loading main4ino properties/creds stored in FS...");
    getPropSync()->setLoginPass(apiDeviceLogin(), apiDevicePass());
    getClockSync()->setLoginPass(apiDeviceLogin(), apiDevicePass());

    log(CLASS_MODULE, Info, "# Loading other properties/creds stored in FS...");
    getPropSync()->fsLoadActorsProps();

    log(CLASS_MODULE, Info, "# Syncing actors with main4ino server...");
    bool serSyncd = getPropSync()->pullPushActors(DEFAULT_PROP_SYNC_ATTEMPTS, getSettings()->oneRun()); // sync properties from the server

    if (!serSyncd)
      return false; // fail fast

    time_t leftTime = getBot()->getClock()->currentTime();

    Buffer timeAux(19);
    log(CLASS_MODULE, Info, "# Previous actors' times: %s...", Timing::humanize(leftTime, &timeAux));
    getBot()->setActorsTime(leftTime);

    log(CLASS_MODULE, Info, "# Syncing clock...");
    // sync real date / time on clock, block if a single run is requested
    bool clockSyncd = getClockSync()->syncClock(getSettings()->oneRun(), DEFAULT_CLOCK_SYNC_ATTEMPTS);
    log(CLASS_MODULE, Info, "# Current time: %s", Timing::humanize(getBot()->getClock()->currentTime(), &timeAux));

    return clockSyncd;
  }

public:
  Module() {
    actors = new Array<Actor *>;

    settings = new Settings("settings");
    propSync = new PropSync("propsync");
    clockSync = new ClockSync("clocksync");
    clock = new Clock("clock");

    actors->add(4,
      (Actor *)settings,
      (Actor *)propSync,
      (Actor *)clockSync,
      (Actor *)clock
		);

    bot = new SerBot(clock, actors);

    initWifi = NULL;
    clearDevice = NULL;
    httpGet = NULL;
    httpPost = NULL;
    sleepInterruptable = NULL;
    configureModeArchitecture = NULL;
    runModeArchitecture = NULL;
    commandArchitecture = NULL;
    fileRead = NULL;
    fileWrite = NULL;
    info = NULL;
    test = NULL;
    update = NULL;
    apiDeviceLogin = NULL;
    apiDevicePass = NULL;
  }

  void setup(BotMode (*setupArchitecture)(),
             bool (*initWifiFunc)(),
             int (*httpPostFunc)(const char *url, const char *body, ParamStream *response, Table *headers),
             int (*httpGetFunc)(const char *url, ParamStream *response, Table *headers),
             void (*clearDeviceFunc)(),
             bool (*fileReadFunc)(const char *fname, Buffer *content),
             bool (*fileWriteFunc)(const char *fname, const char *content),
             void (*abortFunc)(const char *msg),
             bool (*sleepInterruptableFunc)(time_t cycleBegin, time_t periodSec),
             void (*configureModeArchitectureFunc)(),
             void (*runModeArchitectureFunc)(),
             CmdExecStatus (*commandArchitectureFunc)(const char *cmd),
             void (*infoFunc)(),
             void (*updateFunc)(const char* descriptor),
             void (*testFunc)(),
             const char *(*apiDeviceLoginFunc)(),
             const char *(*apiDevicePassFunc)()) {

    // Unstable situation from now until the end of the function
    //
    // Actors are being initialized. When they act, they use callback functions that
    // may trigger low level calls (like IO, LCD, etc) which are not set up yet.
    // As a good practice, actors should do no low level calls unless they are asked to act.
    // Setup of low level calls happens in setupArchitecture().
    // After that actors will be eventually asked for acting.

    initWifi = initWifiFunc;
    clearDevice = clearDeviceFunc;
    sleepInterruptable = sleepInterruptableFunc;
    configureModeArchitecture = configureModeArchitectureFunc;
    runModeArchitecture = runModeArchitectureFunc;
    commandArchitecture = commandArchitectureFunc;
    fileRead = fileReadFunc;
    fileWrite = fileWriteFunc;
    info = infoFunc;
    test = testFunc;
    update = updateFunc;
    apiDeviceLogin = apiDeviceLoginFunc;
    apiDevicePass = apiDevicePassFunc;
    httpGet = httpGetFunc;
    httpPost = httpPostFunc;

    propSync->setup(bot, initWifi, httpGet, httpPost, fileRead, fileWrite);
    clockSync->setup(bot->getClock(), initWifi, httpGet);

    BotMode mode = setupArchitecture(); // module objects initialized, architecture can be initialized now

    getBot()->setMode(mode);

    if (mode == RunMode) {
      bool syncSucc = sync();
      if (!syncSucc) {
        abortFunc("Could not sync");
      }
    }
  }

  /**
   * Handle a user command.
   * If no command maches, commandArchitecture will be used as fallback.
   */
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
        logRawUser("Arguments needed:\n  set <actor> <prop> <value>");
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
    } else if (strcmp("run", c) == 0) {
      log(CLASS_MODULE, Info, "-> Run mode");
      runCmd();
      return ExecutedInterrupt;
    } else if (strcmp("conf", c) == 0) {
      log(CLASS_MODULE, Info, "-> Configure mode");
      confCmd();
      return ExecutedInterrupt;
    } else if (strcmp("info", c) == 0) {
      infoCmd();
      return Executed;
    } else if (strcmp("test", c) == 0) {
      test();
      return Executed;
    } else if (strcmp("update", c) == 0) {
      const char *descriptor = strtok(NULL, " ");
      if (descriptor == NULL) {
        logRawUser("Arguments needed:\n  update <descriptor>");
        return InvalidArgs;
      }
      update(descriptor);
      return Executed;
    } else if (strcmp("clear", c) == 0) {
      clearDevice();
      return Executed;
    } else if (strcmp("logl", c) == 0) {
      c = strtok(NULL, " ");
      if (c == NULL) {
        char ll = getLogLevel();
        log(CLASS_MODULE, Info, "Log level: %d", ll);
        return InvalidArgs;
      }
      int ll = atoi(c);
      setLogLevel(ll);
      log(CLASS_MODULE, Info, "Log level: %d", ll);
      return Executed;
    } else if (strcmp("wifissid", c) == 0) {
      c = strtok(NULL, " ");
      if (c == NULL) {
        logRawUser("Argument needed:\n  wifissid <ssid>");
        return InvalidArgs;
      }
      settings->setSsid(c);
      log(CLASS_MODULE, Info, "Wifi ssid: %s", settings->getSsid());
      return Executed;
    } else if (strcmp("wifipass", c) == 0) {
      c = strtok(NULL, " ");
      if (c == NULL) {
        logRawUser("Argument needed:\n  wifipass <pass>");
        return InvalidArgs;
      }
      settings->setPass(c);
      log(CLASS_MODULE, Info, "Wifi pass: %s", settings->getPass());
      return Executed;
    } else if (strcmp("wifi", c) == 0) {
      initWifi();
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
        logRawUser("Argument needed:\n  actone <actorname>");
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
      Buffer buf(128);
      fileRead(f, &buf);
      logUser("### File: %s", f);
      logRawUser(buf.getBuffer());
      logUser("###");
      return Executed;
#endif // INSECURE
    } else if (strcmp("load", c) == 0) {
      propSync->fsLoadActorsProps(); // load mainly credentials already set
      log(CLASS_MODULE, Info, "Properties loaded from local copy");
      return Executed;
    } else if (strcmp("help", c) == 0 || strcmp("?", c) == 0) {
      logRawUser(HELP_COMMAND_CLI);
      commandArchitecture(c);
      return Executed;
    } else {
      return commandArchitecture(c);
    }
  }

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
  SerBot *getBot() {
    return bot;
  }


  ClockSync *getClockSync() {
    return clockSync;
  }

  PropSync *getPropSync() {
    return propSync;
  }

  Settings *getSettings() {
    return settings;
  }

  /**
   * Make all actors act
   */
  void actall() {
    for (int i = 0; i < getBot()->getActors()->size(); i++) {
      Actor *a = getBot()->getActors()->get(i);
      log(CLASS_MODULE, Info, "One off: %s", a->getName());
      a->oneOff();
    }
  }

  /**
   * Touch all actors (to force them to be syncrhonized)
   */
  void touchall() {
    for (int i = 0; i < getBot()->getActors()->size(); i++) {
      Actor *a = getBot()->getActors()->get(i);
      Metadata *m = a->getMetadata();
      log(CLASS_MODULE, Info, "Touch: %s", a->getName());
      m->changed();
    }
  }

  /**
   * Make a given by-name-actor act
   */
  void actone(const char *actorName) {
    for (int i = 0; i < getBot()->getActors()->size(); i++) {
      Actor *a = getBot()->getActors()->get(i);
      if (strcmp(a->getName(), actorName) == 0) {
        log(CLASS_MODULE, Info, "One off: %s", a->getName());
        a->oneOff();
      }
    }
  }

  void runCmd() {
    bot->setMode(RunMode);
  }

  void confCmd() {
    bot->setMode(ConfigureMode);
  }

  void infoCmd() {
    info();
  }

  void getProps(const char *actorN) {
    Buffer contentAuxBuffer(256);
    Array<Actor *> *actors = bot->getActors();
    for (int i = 0; i < actors->size(); i++) {
      Actor *actor = actors->get(i);
      if (actorN == NULL || strcmp(actor->getName(), actorN) == 0) {
        bot->getPropsJson(&contentAuxBuffer, i, EXCLUSIVE_FILTER_MODE, FIRST_CHAR(SENSITIVE_PROP_PREFIX));
        contentAuxBuffer.replace('{', ' ');
        contentAuxBuffer.replace('}', ' ');
        contentAuxBuffer.replace(',', '\n');
        log(CLASS_MODULE, Info, "### %s\n%s", actor->getName(), contentAuxBuffer.getBuffer());
      }
    }
  }

  void configureMode() {
    time_t cycleBegin = now();
    configureModeArchitecture();
    sleepInterruptable(cycleBegin, PERIOD_CONFIGURE_MSEC / 1000);
  }

  void runMode() {
    time_t cycleBegin = now();
    runModeArchitecture();
    cycleBot(false, false, true);
    sleepInterruptable(cycleBegin, getSettings()->periodMsec() / 1000);
  }

  Array<Actor *> *getActors() {
    return actors;
  }

  void loop() {
    switch (getBot()->getMode()) {
      case (RunMode):
        log(CLASS_MODULE, Info, "\n\nBEGIN LOOP (ver: %s)", STRINGIFY(PROJ_VERSION));
        runMode();
        log(CLASS_MODULE, Info, "END LOOP\n\n");
        break;
      case (ConfigureMode):
        configureMode();
        break;
      default:
        break;
    }
  }
};

#endif // MODULE_INC
