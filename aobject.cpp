#include "aobject.h"

static_assert(sizeof(aproperty::AObject) >= sizeof(QObject));

#include "moc_aobject.cpp"
