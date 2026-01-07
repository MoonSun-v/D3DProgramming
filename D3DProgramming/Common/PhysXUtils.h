#pragma once

#define PX_RELEASE(x)   \
    if (x)              \
    {                   \
        x->release();   \
        x = nullptr;    \
    }