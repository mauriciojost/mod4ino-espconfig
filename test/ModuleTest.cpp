#ifdef UNIT_TEST

// Being tested
#include <Module.h>

// Extra libraries needed
#include <main4ino/Misc.h>
#include <string.h>
#include <unity.h>

#define CLASS_MAIN "ModuleTest"

#define REQ_POST 1
#define REQ_GET 2

const char *replyEmptyBody = "{}";

bool wifiConnected;
int pullCount;

void setUp(void) {
  wifiConnected = true;
  pullCount = 1;
  setLogLevel(Debug);
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

const char *apiDeviceLogin() {
  return "testdevice";
}

const char *apiDevicePass() {
  return "password";
}

bool oneRunModeOff() {
  return false;
}

void logLine(const char *str) {
  log(CLASS_MAIN, Debug, "logLine('%s')", str);
}

bool initWifi(const char *ssid, const char *pass, bool skipIfConnected, int retries) {
  log(CLASS_MAIN, Debug, "initWifi('%s', '%s', %d)", ssid, pass, retries);
  return wifiConnected;
}

int httpRequest(int req, const char *url, const char *body, ParamStream *response, Table *headers) {
  char str1[128];
  char str2[128];
  long l1;

  // RESTORE BY ACTOR
  if (req == REQ_GET && sscanf(url, MAIN4INOSERVER_API_HOST_BASE "/api/v1/devices/testdevice/%[a-z]/actors/%[a-z]/last", str1, str2) == 2 &&
      strcmp(str1, "reports") == 0) {
    if (strcmp(str2, "settings") == 0) {
      log(CLASS_MAIN, Info, "Settings loaded last '%s'", str1);
      response->contentBuffer()->load("{\"+periodms\":\"10\"}");
    } else {
      response->contentBuffer()->load(replyEmptyBody);
    }
    return HTTP_OK;

    // RETRIEVE TARGETS NOT CONSUMED (I.E. CLOSED STATUS)
  } else if (req == REQ_GET && strcmp(MAIN4INOSERVER_API_HOST_BASE "/api/v1/devices/testdevice/targets?status=C&ids=true", url) == 0) {
    response->contentBuffer()->load("{\"ids\": [1]}");
    return HTTP_OK;

    // PULL BY ACTOR
  } else if (req == REQ_GET &&
             sscanf(url, MAIN4INOSERVER_API_HOST_BASE "/api/v1/devices/testdevice/%[a-z]/%ld/actors/%[a-z]", str1, &l1, str2) == 3 &&
             strcmp(str1, "targets") == 0) {
    if (strcmp(str2, "settings") == 0) {
      log(CLASS_MAIN, Info, "Settings loaded target '%s' from %ld", str1, l1);
      response->contentBuffer()->load("{\"+periodms\":\"20\"}");
    } else {
      log(CLASS_MAIN, Info, "Settings loaded generic");
      response->contentBuffer()->load(replyEmptyBody);
    }
    return HTTP_OK;

    // PUSH BY ACTOR (REPORTS)
  } else if (req == REQ_POST &&
             sscanf(url, MAIN4INOSERVER_API_HOST_BASE "/api/v1/devices/testdevice/%[a-z]/%ld/actors/%[a-z]", str1, &l1, str2) == 3) {
    return HTTP_CREATED;

    // MARK REPORT AS CLOSED
    // MARK TARGET AS CONSUMED
  } else if (req == REQ_POST && sscanf(url, MAIN4INOSERVER_API_HOST_BASE "/api/v1/devices/testdevice/%[a-z]/%ld?status=", str1, &l1) == 2) {
    return HTTP_OK;

    // PRE-PUSH BY ACTOR (REPORTS)
  } else if (req == REQ_POST && sscanf(url, MAIN4INOSERVER_API_HOST_BASE "/api/v1/devices/testdevice/%[a-z]/", str1) == 1) {
    if (strcmp(str1, "reports") == 0) {
      response->contentBuffer()->load("{\"id\": 1}");
      return HTTP_CREATED;
    } else {
      return HTTP_OK;
    }

    // RETRIEVE TIME
  } else if (req == REQ_GET && sscanf(url, MAIN4INOSERVER_API_HOST_BASE "/api/v1/time?timezone=%s", str1) == 1) {
    response->contentBuffer()->load("{\"formatted\":\"1970-01-01T00:00:01\"}");
    return HTTP_OK;

    // UNKNOWN
  } else {
    log(CLASS_MAIN, Debug, "Unknown url '%s'", url);
    TEST_FAIL();
    return 0;
  }
}

int httpGet(const char *url, ParamStream *response, Table *headers) {
  return httpRequest(REQ_GET, url, NULL, response, headers);
}

int httpPost(const char *url, const char *body, ParamStream *response, Table *headers) {
  return httpRequest(REQ_POST, url, body, response, headers);
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

void updateFirmware(const char *d) {
  log(CLASS_MAIN, Debug, "updateFirmware(%s)", d);
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

  log(CLASS_MAIN, Debug, "### module->setup(...)");
  m->setup(setupArchitecture,
           initWifiSimple,
           httpPost,
           httpGet,
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
           oneRunModeOff);
  m->startupProperties();

  TEST_ASSERT_EQUAL(1, (int)m->getBot()->getClock()->currentTime()); // remote clock sync took place
  TEST_ASSERT_EQUAL(20, m->getSettings()->periodMsec());             // loaded target value

  log(CLASS_MAIN, Debug, "### module->loop()");
  m->getPropSync()->getTiming()->setFreq("~1s");
  m->loop();
  TEST_ASSERT_EQUAL(20, m->getSettings()->periodMsec()); // no change
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_basic_behaviour);
  return (UNITY_END());
}

#endif // UNIT_TEST
