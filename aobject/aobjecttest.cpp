#include "aobjecttest.h"

static_assert(sizeof(aproperty::AObject) >= sizeof(QObject));

#include "moc_aobjecttest.cpp"
