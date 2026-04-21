/****************************************************************************
** Meta object code from reading C++ file 'ChromackControl.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/ChromackControl.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ChromackControl.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.11.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN15ChromackControlE_t {};
} // unnamed namespace

template <> constexpr inline auto ChromackControl::qt_create_metaobjectdata<qt_meta_tag_ZN15ChromackControlE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ChromackControl",
        "D-Bus Interface",
        "org.dxle.chromack.Control",
        "reloadRequested",
        "",
        "openRequested",
        "closeRequested",
        "toggleRequested",
        "setColorRequested",
        "value",
        "Reload",
        "Open",
        "Close",
        "Toggle",
        "SetColor",
        "ActiveColor",
        "UpdateActiveColor"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'reloadRequested'
        QtMocHelpers::SignalData<void()>(3, 4, QMC::AccessPublic, QMetaType::Void),
        // Signal 'openRequested'
        QtMocHelpers::SignalData<void()>(5, 4, QMC::AccessPublic, QMetaType::Void),
        // Signal 'closeRequested'
        QtMocHelpers::SignalData<void()>(6, 4, QMC::AccessPublic, QMetaType::Void),
        // Signal 'toggleRequested'
        QtMocHelpers::SignalData<void()>(7, 4, QMC::AccessPublic, QMetaType::Void),
        // Signal 'setColorRequested'
        QtMocHelpers::SignalData<void(const QString &)>(8, 4, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 9 },
        }}),
        // Slot 'Reload'
        QtMocHelpers::SlotData<void()>(10, 4, QMC::AccessPublic, QMetaType::Void),
        // Slot 'Open'
        QtMocHelpers::SlotData<void()>(11, 4, QMC::AccessPublic, QMetaType::Void),
        // Slot 'Close'
        QtMocHelpers::SlotData<void()>(12, 4, QMC::AccessPublic, QMetaType::Void),
        // Slot 'Toggle'
        QtMocHelpers::SlotData<void()>(13, 4, QMC::AccessPublic, QMetaType::Void),
        // Slot 'SetColor'
        QtMocHelpers::SlotData<void(const QString &)>(14, 4, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 9 },
        }}),
        // Slot 'ActiveColor'
        QtMocHelpers::SlotData<QString() const>(15, 4, QMC::AccessPublic, QMetaType::QString),
        // Slot 'UpdateActiveColor'
        QtMocHelpers::SlotData<void(const QString &)>(16, 4, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 9 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    QtMocHelpers::UintData qt_constructors {};
    QtMocHelpers::ClassInfos qt_classinfo({
            {    1,    2 },
    });
    return QtMocHelpers::metaObjectData<ChromackControl, qt_meta_tag_ZN15ChromackControlE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums, qt_constructors, qt_classinfo);
}
Q_CONSTINIT const QMetaObject ChromackControl::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15ChromackControlE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15ChromackControlE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN15ChromackControlE_t>.metaTypes,
    nullptr
} };

void ChromackControl::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ChromackControl *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->reloadRequested(); break;
        case 1: _t->openRequested(); break;
        case 2: _t->closeRequested(); break;
        case 3: _t->toggleRequested(); break;
        case 4: _t->setColorRequested((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->Reload(); break;
        case 6: _t->Open(); break;
        case 7: _t->Close(); break;
        case 8: _t->Toggle(); break;
        case 9: _t->SetColor((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 10: { QString _r = _t->ActiveColor();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 11: _t->UpdateActiveColor((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (ChromackControl::*)()>(_a, &ChromackControl::reloadRequested, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (ChromackControl::*)()>(_a, &ChromackControl::openRequested, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (ChromackControl::*)()>(_a, &ChromackControl::closeRequested, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (ChromackControl::*)()>(_a, &ChromackControl::toggleRequested, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (ChromackControl::*)(const QString & )>(_a, &ChromackControl::setColorRequested, 4))
            return;
    }
}

const QMetaObject *ChromackControl::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ChromackControl::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15ChromackControlE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int ChromackControl::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 12)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 12;
    }
    return _id;
}

// SIGNAL 0
void ChromackControl::reloadRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void ChromackControl::openRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void ChromackControl::closeRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void ChromackControl::toggleRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void ChromackControl::setColorRequested(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}
QT_WARNING_POP
