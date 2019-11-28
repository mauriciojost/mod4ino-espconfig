#ifndef MODULE_STATUS_INC
#define MODULE_STATUS_INC

enum CmdExecStatus { NotFound = 0, InvalidArgs, Executed, ExecutedInterrupt, CmdFailed };
#define CMD_EXEC_STATUS(s)                                                                                                                 \
  (s == NotFound                                                                                                                           \
       ? "Not found"                                                                                                                       \
       : (s == InvalidArgs                                                                                                                 \
              ? "Invalid args"                                                                                                             \
              : (s == Executed ? "Executed" : (s == ExecutedInterrupt ? "Executed w/int" : (s == CmdFailed ? "Failed" : "Unknown")))))

#endif // MODULE_STATUS_INC
