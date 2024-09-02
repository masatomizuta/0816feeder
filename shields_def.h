#ifndef _SHIELDS_DEF_H
#define _SHIELDS_DEF_H

#define SHIELD_NATIVE 1
#define SHIELD_SENSOR 2
#define SHIELD_UNO    3

#define SHIELD(shield) (CONTROLLER_SHIELD == SHIELD_##shield)

#endif // _SHIELDS_DEF_H
