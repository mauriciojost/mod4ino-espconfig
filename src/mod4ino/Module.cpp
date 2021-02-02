#include <mod4ino/Module.h>

void Module::setupSettings() {
  settings->setup(this, propSync);
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

