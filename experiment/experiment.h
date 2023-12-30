#ifndef PROPERTYEXPERIMENT_H
#define PROPERTYEXPERIMENT_H

#include <QObject>

namespace experiment {

class InterfaceOne
{
public:
    virtual const char *firstInterfaceCall() const = 0;
};

class InterfaceTwo
{
public:
    virtual const char *secondInterfaceCall() const = 0;
};

class ParentClass : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;
};

} // experiment

Q_DECLARE_INTERFACE(experiment::InterfaceOne, "experiment/InterfaceOne/1.0")
Q_DECLARE_INTERFACE(experiment::InterfaceTwo, "experiment/InterfaceTwo/1.0")

#endif // PROPERTYEXPERIMENT_H
