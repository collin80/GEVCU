#ifndef ELMPROC_H_
#define ELMPROC_H_

#include <Arduino.h>
#include "config.h"
#include "Sys_Messages.h"
#include "DeviceTypes.h"
#include "OBD2Handler.h"
#include "SocketProcessor.h"

class ELM327Processor : public SocketProcessor
{
public:
    ELM327Processor();
    String generateUpdate();
    String generateLogEntry(char *logLevel, char *deviceName, char *message);
    String processInput(char *input);

private:
    OBD2Handler *obd2Handler;
    char buffer[30]; // a buffer for various string conversions
    bool bLineFeed;
    bool bHeader;
};

#endif
