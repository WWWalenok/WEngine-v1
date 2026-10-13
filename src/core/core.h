#pragma once

#define WENGINE_CLASS(...)
#define WENGINE_FIELD(...)
#define WENGINE_FUNCTION(...)

#include "VECP/include/VECP.h"
#include "WLoger/include/WLoger.h"

#include "imp/lockfreelist.h"
#include "imp/thread_pool.h"

#include "imp/nlohmannjson.hpp"
#include "imp/data.h"
#include "imp/event.h"
#include "imp/wmath.h"
#include "imp/Property.h"
#include "imp/rtree.h"

#include "imp/ISystem.h"

class CoreObject : public RefCounted
{

};