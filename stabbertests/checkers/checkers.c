#include "resource.h"

int
resource_equal_check(void *param, void *value)
{
    Resource *param_res = (Resource *)param;
    Resource *value_res = (Resource *)value;
    
    if (g_strcmp0(param_res->name, value_res->name) != 0) {
        return 0;
    }
    
    if (param_res->presence != value_res->presence) {
        return 0;
    }
    
    if (param_res->priority != value_res->priority) {
        return 0;
    }
    
    if (g_strcmp0(param_res->status, value_res->status) != 0) {
        return 0;
    }
    
    return 1;
}
