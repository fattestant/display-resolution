#ifndef MY_NETWORK_H
#define MY_NETWORK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lua.h"
#include "lualib.h"

void register_network_function(lua_State* L);

#ifdef __cplusplus
}
#endif

#endif //MY_NETWORK_H
