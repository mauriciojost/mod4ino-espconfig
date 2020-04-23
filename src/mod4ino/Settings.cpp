#include <mod4ino/Settings.h>

  void Settings::command(const char* c) {
#ifdef INSECURE
    mod->command(cmdLine->getBuffer());
#endif // INSECURE
  }
