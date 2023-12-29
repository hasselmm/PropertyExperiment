#include "experiment.h"

namespace experiment {

static_assert(qobject_interface_iid<InterfaceOne *>());
static_assert(qobject_interface_iid<InterfaceTwo *>());

} // namespace experiment

#include "moc_experiment.cpp"
