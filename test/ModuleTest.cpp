#ifdef UNIT_TEST

// Being tested
#include <Module.h>

// Extra libraries needed
#include <main4ino/Misc.h>
#include <string.h>
#include <unity.h>

#define CLASS_MAIN "ModuleTest"

const char *replyEmptyBody = "{}";
ParamStream *response = new ParamStream(1024);
std::function<void ()> nop = []() {};

bool wifiConnected;
int pullCount;

void logLine(const char *msg, const char *clz, LogLevel l, bool newline) {
}

void setUp(void) {
  wifiConnected = true;
  pullCount = 1;
  setupLog(logLine);
  setLogOptions("??D;");
}

void tearDown() {}

unsigned long millis() {
  static unsigned long boot = -1;
  struct timespec tms;
  if (clock_gettime(CLOCK_REALTIME, &tms)) {
    log(CLASS_MAIN, Warn, "Couldn't get time");
    return -1;
  }
  unsigned long m = tms.tv_sec * 1000 + tms.tv_nsec / 1000000;
  if (boot == -1) {
    boot = m;
  }
  return m - boot;
}

bool initWifiSimple() {
  return wifiConnected;
}

void stopWifi() {}

const char *apiDeviceLogin() {
  return "testdevice";
}

const char *apiDevicePass() {
  return "password";
}

bool oneRunModeOff() {
  return false;
}

bool initWifi(const char *ssid, const char *pass, bool skipIfConnected, int retries) {
  log(CLASS_MAIN, Debug, "initWifi('%s', '%s', %d)", ssid, pass, retries);
  return wifiConnected;
}

HttpResponse httpRequest(HttpMethod req, const char *url, Stream *body, Table *headers, const char *fingerprint) {
  char str1[128];
  char str2[128];
  long l1;

  // RESTORE SUMMARY
  if (req == HttpGet && strcmp(MAIN4INOSERVER_API_HOST_BASE "/api/v1/devices/testdevice/reports/summary?status=C", url) == 0) {
    return HttpResponse(HTTP_OK, response->fill("{\"settings\":{\"+bfreq\":\"~10s\"}}"), nop);

  // RETRIEVE LAST SUMMARY REPORT
  } else if (req == HttpGet && strcmp(MAIN4INOSERVER_API_HOST_BASE "/api/v1/devices/testdevice/reports/summary?status=C", url) == 0) {
    response->fill("{"); // broken
    return HttpResponse(HTTP_OK, response, nop);

  // RETRIEVE TARGETS NOT CONSUMED (I.E. CLOSED STATUS) AND COMMIT THEM
  } else if (req == HttpGet && strcmp(MAIN4INOSERVER_API_HOST_BASE "/api/v1/devices/testdevice/targets/summary?status=C&newstatus=X", url) == 0) {
    return HttpResponse(HTTP_OK, response->fill("{\"settings\":{\"+bfreq\":\"~20s\"}}"), nop);

    // PRE-PUSH BY ACTOR (REPORTS)
  } else if (req == HttpPost && sscanf(url, MAIN4INOSERVER_API_HOST_BASE "/api/v1/devices/testdevice/%[a-z]", str1) == 1) {
    if (strcmp(str1, "reports") == 0) {
      return HttpResponse(HTTP_CREATED, response->fill("{\"id\": 1}"), nop);
    } else {
      return HttpResponse(HTTP_OK, response->fill(""), nop);
    }

    // RETRIEVE TIME
  } else if (req == HttpGet && sscanf(url, MAIN4INOSERVER_API_HOST_BASE "/api/v1/time?timezone=%s", str1) == 1) {
    response->fill("{\"formatted\":\"1970-01-01T00:00:01\"}");
    return HttpResponse(HTTP_OK, response, nop);

  // LOGIN CALL
  } else if (req == HttpPost && strcmp(MAIN4INOSERVER_API_HOST_BASE "/api/v1/session"  , url) == 0) {
    return HttpResponse(HTTP_OK, response->fill("an-id-here"), nop);

    // UNKNOWN
  } else {
    log(CLASS_MAIN, Debug, "Unknown request %s->%s ", HTTP_METHOD_STR(req), url);
    TEST_FAIL();
    return HttpResponse(HTTP_BAD_REQUEST, response->fill(""), nop);
  }
}

void clearDevice() {
  log(CLASS_MAIN, Debug, "clearDevice()");
}

bool readFile(const char *f, Buffer *content) {
  Buffer fname(64);
  fname.fill("./test/fs/%s", f);
  log(CLASS_MAIN, Debug, "readFile('%s', xxx)", fname.getBuffer());
  bool success = false;
  char c;
  int i = 0;
  FILE *fp = fopen(fname.getBuffer(), "r");
  content->clear();
  if (fp != NULL) {
    while ((c = getc(fp)) != EOF) {
      content->append(c);
      i++;
    }
    fclose(fp);
    success = true;
  } else {
    log(CLASS_MAIN, Warn, "Could not load file: %s", fname.getBuffer());
    success = false;
  }
  return success;
}

bool writeFile(const char *fname, const char *content) {
  bool success = false;
  FILE *file = fopen(fname, "w+");
  int results = fputs(content, file);
  if (results == EOF) {
    log(CLASS_MAIN, Warn, "Failed to write %s ", fname);
    success = false;
  } else {
    success = true;
  }
  fclose(file);
  return success;
}

void infoArchitecture() {
  log(CLASS_MAIN, Debug, "infoArchitecture()");
}

void testArchitecture() {
  log(CLASS_MAIN, Debug, "testArchitecture()");
}

void updateFirmware(const char *d, const char* c) {
  log(CLASS_MAIN, Debug, "updateFirmware(target=%s, current=%s)", d, c);
}

bool sleepInterruptable(time_t cycleBegin, time_t periodSecs) {
  log(CLASS_MAIN, Info, "sleepInterruptable(%ds)...", (int)periodSecs);
  return false;
}

void sleepNotInterruptable(time_t cycleBegin, time_t periodSecs) {
  log(CLASS_MAIN, Info, "sleepNotInterruptable(%ds)...", (int)periodSecs);
}

BotMode setupArchitecture() {
  log(CLASS_MAIN, Debug, "setupArchitecture()");
  setExternalMillis(millis);
  return RunMode;
}

void runModeArchitecture() {
  log(CLASS_MAIN, Debug, "runModeArchitecture()");
}

CmdExecStatus commandArchitecture(const char *command) {
  log(CLASS_MAIN, Debug, "commandArchitecture('%s')", command);
  return NotFound;
}

void configureModeArchitecture() {
  log(CLASS_MAIN, Debug, "configureModeArchitecture()");
}

void test_basic_behaviour() {
  Module *m = new Module();
  TEST_ASSERT_EQUAL(0, (int)m->getBot()->getClock()->currentTime());
  BotMode mode = setupArchitecture();
  log(CLASS_MAIN, Debug, "### module->setup(...)");
  m->setup("testproject",
           "testplatform",
           initWifiSimple,
           stopWifi,
           httpRequest,
           clearDevice,
           readFile,
           writeFile,
           sleepInterruptable,
           sleepNotInterruptable,
           configureModeArchitecture,
           runModeArchitecture,
           commandArchitecture,
           infoArchitecture,
           updateFirmware,
           testArchitecture,
           apiDeviceLogin,
           apiDevicePass,
           oneRunModeOff,
           NULL // logs to push
           );
  m->getBot()->setMode(mode);
  StartupStatus s = m->startupProperties();

  TEST_ASSERT_EQUAL(ModuleStartupPropertiesCodeSuccess, s.startupCode);          // success
  TEST_ASSERT_EQUAL(1, (int)m->getBot()->getClock()->currentTime()); // remote clock sync took place
  TEST_ASSERT_EQUAL_STRING("~20s", m->getSettings()->getBatchTiming()->getFreq());             // loaded target value

  log(CLASS_MAIN, Debug, "### module->loop()");
  m->getPropSync()->getTiming()->setFreq("~1s");
  m->loop();
  TEST_ASSERT_EQUAL_STRING("~20s", m->getSettings()->getBatchTiming()->getFreq()); // no change
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_basic_behaviour);
  return (UNITY_END());
}

#endif // UNIT_TEST
