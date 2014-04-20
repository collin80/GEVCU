#ifndef ELMPROC_H_
#define ELMPROC_H_

#include <Arduino.h>
#include "config.h"
#include "constants.h"
#include "Sys_Messages.h"
#include "DeviceTypes.h"
#include "OBD2Handler.h"

class ELM327Processor
{
public:
    ELM327Processor();
    String processELMCmd(char *cmd);
private:
    OBD2Handler *obd2Handler;
    char buffer[30]; // a buffer for various string conversions
    bool bLineFeed;
    bool bHeader;
};

#endif