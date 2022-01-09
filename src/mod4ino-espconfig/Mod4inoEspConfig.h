#ifndef ESPCONFIG_INC
#define ESPCONFIG_INC

#define CLASS_ESPCONFIG "EC"

#include <log4ino/Log.h>

#include <main4ino/Actor.h>
#include <main4ino/Queue.h>
#include <mod4ino/Module.h>

#include <WiFiManager.h>
WiFiManager wm(Serial);

#define DELAY_MS_WIFI_STA 2000
#define MAX_PARAM_SIZE 32
#define CONFIG_PORTAL_TIMEOUT_SECS 240
#define WIFI_STA_PASS "main4ino"
#define MAX_AMOUNT_OF_PROPS 64
#define PROP_ID_LENGTH 3

bool propApplicable(const char* name) {
  return true;
}

void saveParamCallback(Module* m){
  log(CLASS_ESPCONFIG, Debug, "[CALLBACK] saveParamCallback fired");
  WiFiManagerParameter** params = wm.getParameters();
  Array<Actor *> *actors = m->getBot()->getActors();
  int c = 0;
  for (unsigned int i = 0; i < actors->size(); i++) {
    Actor *actor = actors->get(i);
    c++; // ignore actor name label prefix
    c++; // ignore actor name label actor name
    c++; // ignore actor name label suffix
    log(CLASS_ESPCONFIG, Fine, "%s:", actor->getName());
    for (int p = 0; p < actor->getNroProps(); p++) {
      const char *propName = actor->getPropName(p);
      if (propApplicable(propName)) {
        WiFiManagerParameter* pa = params[c];
        Buffer contentAuxBuffer(MAX_PARAM_SIZE);
        contentAuxBuffer.load(pa->getValue());
        actor->setPropValue(p, &contentAuxBuffer);
        contentAuxBuffer.clear();
        actor->getPropValue(p, &contentAuxBuffer);
        log(CLASS_ESPCONFIG, Debug, " %s<-'%s'", propName, contentAuxBuffer.getBuffer());
        c++;
      }
    }
  }
  wm.stopConfigPortal();
}

std::function<void (Module* md)> firstSetupArchitecture = [&](Module* md) {

  log(CLASS_ESPCONFIG, Info, "Started first setup");

  // https://github.com/tzapu/WiFiManager/blob/master/examples/Super/OnDemandConfigPortal/OnDemandConfigPortal.ino
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP

  // Reset WIFI settings systematically
  // If this function is called, is because it's really needed
  ESP.eraseConfig();
  wm.resetSettings();
  wm.erase();

  delay(DELAY_MS_WIFI_STA);

  Array<Actor *> *actors = md->getBot()->getActors();
  Queue<MAX_AMOUNT_OF_PROPS, PROP_ID_LENGTH> queue;
  Buffer idBuffer(PROP_ID_LENGTH);
  int c = 0;
  Buffer contentAuxBuffer(MAX_PARAM_SIZE);
  for (unsigned int i = 0; i < actors->size(); i++) {
    Actor *actor = actors->get(i);
    WiFiManagerParameter* ap1 = new WiFiManagerParameter("<p><h2>Actor ");
    wm.addParameter(ap1);
    c++;
    WiFiManagerParameter* ap2 = new WiFiManagerParameter(actor->getName());
    wm.addParameter(ap2);
    c++;
    WiFiManagerParameter* ap3 = new WiFiManagerParameter("</h2></p>");
    wm.addParameter(ap3);
    c++;

    for (int p = 0; p < actor->getNroProps(); p++) {
      const char *propName = actor->getPropName(p);
      if (propApplicable(propName)) {
        actor->getPropValue(p, &contentAuxBuffer);
        log(CLASS_ESPCONFIG, Debug, " '%s'='%s'", propName, contentAuxBuffer.getBuffer());
        idBuffer.fill("%03x", c);
        WiFiManagerParameter* pam = new WiFiManagerParameter(
          queue.getAt(queue.push(idBuffer.getBuffer()) - 1, "def"),
          actor->getPropNameAlpha(p), 
          contentAuxBuffer.getBuffer(), 
          MAX_PARAM_SIZE
        );
        wm.addParameter(pam);
        c++;
      }
    }
  }

  wm.setDarkMode(true);

  // callbacks
  wm.setSaveParamsCallback([&]{saveParamCallback(md);});

  std::vector<const char *> menu = {"param","sep","restart","exit"};
  wm.setMenu(menu); // custom menu, pass vector

  // set country
  // setting wifi country seems to improve OSX soft ap connectivity,
  // may help others as well, default is CN which has different channels
  wm.setCountry("US");

  // set Hostname
  wm.setHostname(md->getApiDeviceLogin());

  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep in seconds
  wm.setConfigPortalTimeout(CONFIG_PORTAL_TIMEOUT_SECS);

  // connect after portal save toggle
  wm.setSaveConnect(false); // do not connect, only save

  // This is sometimes necessary, it is still unknown when and why this is needed but it may solve some race condition or bug in esp SDK/lib
  wm.setCleanConnect(true); // disconnect before connect, clean connect

  wm.setBreakAfterConfig(true); // needed to use saveWifiCallback
 
  wm.setCaptivePortalEnable(true);

  if(!wm.autoConnect(md->getApiDeviceLogin(), WIFI_STA_PASS)) {
    log(CLASS_ESPCONFIG, Info, "Auto connect finished");
  } else {
    //if you get here you have connected to the WiFi
    log(CLASS_ESPCONFIG, Debug, "Connected!");
  }

  WiFiManagerParameter** params = wm.getParameters();
  for (int p=0; p< wm.getParametersCount(); p++) {
    WiFiManagerParameter* pa = params[p];
    free(pa);
  }

};

#endif // ESPCONFIG_INC

