#include "nproperty.h"

#include <private/qmetaobjectbuilder_p.h>

namespace nproperty::detail {

static_assert(canonical(Feature::Read)   == (Feature::Read));
static_assert(canonical(Feature::Notify) == (Feature::Read | Feature::Notify));
static_assert(canonical(Feature::Reset)  == (Feature::Read | Feature::Notify | Feature::Reset));
static_assert(canonical(Feature::Write)  == (Feature::Read | Feature::Notify | Feature::Write));

QMetaObject MetaObjectBuilder::makeMetaObject(const QMetaType &metaType,
                                              const QMetaObject *superClass,
                                              const std::vector<MemberInfo> &members)
{
    auto metaObject = QMetaObjectBuilder{};

    metaObject.setFlags(PropertyAccessInStaticMetaCall);
    metaObject.setClassName(metaType.name());
    metaObject.setSuperClass(superClass);

    for (const auto &m: members) {
        switch (m.type) {
        case MemberInfo::Type::Property:
            makeProperty(metaObject, m);
            break;

        case MemberInfo::Type::Setter:
        case MemberInfo::Type::Method:
        case MemberInfo::Type::Signal:
        case MemberInfo::Type::Slot:
        case MemberInfo::Type::Constructor:
        case MemberInfo::Type::Invalid:
            break;
        }
    }

    return *metaObject.toMetaObject(); // FIXME: memory leak
}

void MetaObjectBuilder::makeProperty(QMetaObjectBuilder &metaObject,
                                     const MemberInfo   &property)
{
    const auto features = canonical(property.features);
    const auto type     = QMetaType{property.valueType};
    auto metaProperty   = metaObject.addProperty(property.name, type.name(), type);

    metaProperty.setReadable  (features & Feature::Read);
    metaProperty.setWritable  (features & Feature::Write);
    metaProperty.setResettable(features & Feature::Reset);
    metaProperty.setDesignable(true);
    metaProperty.setScriptable(true);
    metaProperty.setStored    (true);
    metaProperty.setStdCppSet (features & Feature::Write); // QTBUG-120378
    metaProperty.setFinal     (true);

    if (features & Feature::Notify) {
        auto signature = metaProperty.name() + "Changed(" + type.name() + ")";
        auto metaSignal = metaObject.addSignal(std::move(signature));
        metaSignal.setParameterNames({metaProperty.name()});
        metaProperty.setNotifySignal(std::move(metaSignal));
    } else {
        metaProperty.setConstant(true);
    }
}

} // namespace nproperty::detail
