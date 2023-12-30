#include "experiment.h"

namespace experiment {

static_assert(requires { qobject_interface_iid<InterfaceOne *>(); });
static_assert(requires { qobject_interface_iid<InterfaceTwo *>(); });

} // namespace experiment

#include "moc_experiment.cpp"
