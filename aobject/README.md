#  Property Experiment

## The AObject Experiment

This was the verify first try after reading 
[André's e-mail](https://lists.qt-project.org/pipermail/development/2023-December/044807.html).

### Object Definition

An object definition looks like this:

``` C++
class AObjectTest : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString constant READ constant CONSTANT FINAL)
    Q_PROPERTY(QString notifying READ notifying NOTIFY notifyingChanged FINAL)
    Q_PROPERTY(QString writable READ writable WRITE setWritable NOTIFY writableChanged FINAL)

public:
    using QObject::QObject;

    void modifyNotifying() { notifying = u"I have been changed per method"_qs; }

signals:
    void notifyingChanged(QString notifying);
    void writableChanged(QString writable);

public:
    Property<QString>                     constant  = {u"I am constant"_qs};
    Notifying<&AObject::notifyingChanged> notifying = {this, u"I am observing"_qs};
    Notifying<&AObject::writableChanged>  writable  = {this, u"I am modifiable"_qs};
    const Setter<QString>              setWritable  = {&writable};
};
```

You'll quickly spot the moc declarations above. Wasn't the plan to get rid of moc?
This experiment mainly proofed feasibility. Time to move on to [MObject](../mobject/README.md).

---

Return to [the overview](../README.md) page.
