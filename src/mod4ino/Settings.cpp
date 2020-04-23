#include <mod4ino/Settings.h>

  void Settings::command(const char* c) {
    mod->command(cmdLine->getBuffer());
  }
