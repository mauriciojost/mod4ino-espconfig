#ifndef ESPCONFIG_INC
#define ESPCONFIG_INC

#define CLASS_ESPCONFIG "EC"

#include <log4ino/Log.h>

#include <main4ino/Actor.h>
#include <mod4ino/Module.h>

#include <WiFiManager.h>
WiFiManager wm;

// TODO
// seems wifi pass and ssid are not needed anymore, could be completely removed
// constants extraction (too many magic numbers)
// move to primitives so that othe rprojects can use this
// see how could add description + samples
void saveParamCallback(Module* m){
  log(CLASS_ESPCONFIG, Debug, "[CALLBACK] saveParamCallback fired");
  // wm.stopConfigPortal();
  WiFiManagerParameter** params = wm.getParameters();
  Array<Actor *> *actors = m->getBot()->getActors();
  int c = 0;
  for (unsigned int i = 0; i < actors->size(); i++) {
    Actor *actor = actors->get(i);
    Buffer contentAuxBuffer(64);
    for (int p = 0; p < actor->getNroProps(); p++) {
      const char *propName = actor->getPropName(p);
      actor->getPropValue(p, &contentAuxBuffer);
      WiFiManagerParameter* pa = params[c];
      log(CLASS_ESPCONFIG, Info, " '%s'='%s'", propName, pa->getValue());
      c++;
    }
  }
  m->getModule()->getPropSync()->fsStoreActorsProps();
}

std::function<void (const char* hostname, Module* md)> firstSetupArchitecture = [&](Module* md) {

  // https://github.com/tzapu/WiFiManager/blob/master/examples/Super/OnDemandConfigPortal/OnDemandConfigPortal.ino
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP

  delay(1000);

  wm.debugPlatformInfo();

  //reset settings - for testing
  //wm.resetSettings();
  //wm.erase();

  Array<Actor *> *actors = md->getBot()->getActors();
  int c = 0;
  for (unsigned int i = 0; i < actors->size(); i++) {
    Actor *actor = actors->get(i);
    Buffer contentAuxBuffer(64);
    for (int p = 0; p < actor->getNroProps(); p++) {
      const char *propName = actor->getPropName(p);
      actor->getPropValue(p, &contentAuxBuffer);
      log(CLASS_ESPCONFIG, Info, " '%s'='%s'", propName, contentAuxBuffer.getBuffer());
      WiFiManagerParameter* pam = new WiFiManagerParameter(actor->getPropNameAlpha(p), actor->getPropNameAlpha(p), contentAuxBuffer.getBuffer(), 32);
      wm.addParameter(pam);
      c++;
    }
  }

  // callbacks
  wm.setSaveParamsCallback([&]{saveParamCallback(md);});

  // invert theme, dark
  wm.setDarkMode(true);

  std::vector<const char *> menu = {"wifi","info","param","sep","restart","exit"};
  wm.setMenu(menu); // custom menu, pass vector

  // set country
  // setting wifi country seems to improve OSX soft ap connectivity,
  // may help others as well, default is CN which has different channels
  wm.setCountry("US");

  // set Hostname
  wm.setHostname(hostname);

  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep in seconds
  wm.setConfigPortalTimeout(120);

  // connect after portal save toggle
  wm.setSaveConnect(false); // do not connect, only save

  // This is sometimes necessary, it is still unknown when and why this is needed but it may solve some race condition or bug in esp SDK/lib
  // wm.setCleanConnect(true); // disconnect before connect, clean connect

  if(!wm.autoConnect(hostname, "0123456789")) {
    log(CLASS_ESPCONFIG, Warn, "Failed to connect and hit timeout");
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

