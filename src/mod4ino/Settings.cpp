#include <mod4ino/Settings.h>

  void Settings::commandObs(const char* c) {
#ifdef INSECURE
    mod->command(cmdLine->getBuffer());
#endif // INSECURE
  }
