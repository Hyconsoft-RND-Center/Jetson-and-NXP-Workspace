/****************************************************************************
** Meta object code from reading C++ file 'FlexrayReceiver.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../Dashboard/src/FlexrayReceiver.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'FlexrayReceiver.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_FlexRayReceiver_t {
    QByteArrayData data[11];
    char stringdata0[123];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_FlexRayReceiver_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_FlexRayReceiver_t qt_meta_stringdata_FlexRayReceiver = {
    {
QT_MOC_LITERAL(0, 0, 15), // "FlexRayReceiver"
QT_MOC_LITERAL(1, 16, 17), // "speedDataReceived"
QT_MOC_LITERAL(2, 34, 0), // ""
QT_MOC_LITERAL(3, 35, 5), // "speed"
QT_MOC_LITERAL(4, 41, 15), // "rpmDataReceived"
QT_MOC_LITERAL(5, 57, 3), // "rpm"
QT_MOC_LITERAL(6, 61, 16), // "fuelDataReceived"
QT_MOC_LITERAL(7, 78, 4), // "fuel"
QT_MOC_LITERAL(8, 83, 16), // "tempDataReceived"
QT_MOC_LITERAL(9, 100, 4), // "temp"
QT_MOC_LITERAL(10, 105, 17) // "readflexrayPacket"

    },
    "FlexRayReceiver\0speedDataReceived\0\0"
    "speed\0rpmDataReceived\0rpm\0fuelDataReceived\0"
    "fuel\0tempDataReceived\0temp\0readflexrayPacket"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_FlexRayReceiver[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   39,    2, 0x06 /* Public */,
       4,    1,   42,    2, 0x06 /* Public */,
       6,    1,   45,    2, 0x06 /* Public */,
       8,    1,   48,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      10,    0,   51,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::Float,    3,
    QMetaType::Void, QMetaType::Float,    5,
    QMetaType::Void, QMetaType::Float,    7,
    QMetaType::Void, QMetaType::Float,    9,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

void FlexRayReceiver::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<FlexRayReceiver *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->speedDataReceived((*reinterpret_cast< float(*)>(_a[1]))); break;
        case 1: _t->rpmDataReceived((*reinterpret_cast< float(*)>(_a[1]))); break;
        case 2: _t->fuelDataReceived((*reinterpret_cast< float(*)>(_a[1]))); break;
        case 3: _t->tempDataReceived((*reinterpret_cast< float(*)>(_a[1]))); break;
        case 4: _t->readflexrayPacket(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (FlexRayReceiver::*)(float );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&FlexRayReceiver::speedDataReceived)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (FlexRayReceiver::*)(float );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&FlexRayReceiver::rpmDataReceived)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (FlexRayReceiver::*)(float );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&FlexRayReceiver::fuelDataReceived)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (FlexRayReceiver::*)(float );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&FlexRayReceiver::tempDataReceived)) {
                *result = 3;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject FlexRayReceiver::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_FlexRayReceiver.data,
    qt_meta_data_FlexRayReceiver,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *FlexRayReceiver::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *FlexRayReceiver::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_FlexRayReceiver.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int FlexRayReceiver::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void FlexRayReceiver::speedDataReceived(float _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void FlexRayReceiver::rpmDataReceived(float _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void FlexRayReceiver::fuelDataReceived(float _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void FlexRayReceiver::tempDataReceived(float _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
