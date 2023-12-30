#ifndef PROPERTYEXPERIMENT_H
#define PROPERTYEXPERIMENT_H

#include <QObject>

namespace experiment {

class ParentClass : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;
};

} // experiment

#endif // PROPERTYEXPERIMENT_H
