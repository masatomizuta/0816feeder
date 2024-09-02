#ifndef _SHIELD_H
#define _SHIELD_H

#include "shields_def.h"

#if SHIELD(NATIVE)
#include "shield_native.h"
#elif SHIELD(SENSOR)
#include "shield_sensor.h"
#elif SHIELD(UNO)
#include "shield_uno.h"
#else
#error "invalid board, please select a proper shield"
#endif

#endif // _SHIELD_H
