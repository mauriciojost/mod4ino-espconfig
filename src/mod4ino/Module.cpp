#include <mod4ino/Module.h>

bool Module::inDebugMode() {
  return settings->getDebug();
}

void Module::setupProjectPlatform(const char *project, const char *platform) {
  settings->setup(this, project, platform, update, propSync);
}

void Module::setSsid(const char* c) {
  settings->setSsid(c);
}

const char* Module::getSsid() {
  return settings->getSsid();
}

void Module::setPass(const char* c) {
  settings->setPass(c);
}

const char* Module::getPass() {
  return settings->getPass();
}

void Module::setSsidBackup(const char* c) {
  settings->setSsidBackup(c);
}

const char* Module::getSsidBackup() {
  return settings->getSsidBackup();
}

void Module::setPassBackup(const char* c) {
  settings->setPassBackup(c);
}

const char* Module::getPassBackup() {
  return settings->getPassBackup();
}


Timing* Module::getBatchTiming() {
  return getSettings()->getBatchTiming();
}

void Module::updateIfMust() {
  getSettings()->updateIfMust();
}
