/****************************************************************************
** Meta object code from reading C++ file 'NotificationCenterPanel.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/NotificationCenterPanel.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'NotificationCenterPanel.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN23NotificationCenterPanelE_t {};
} // unnamed namespace

template <> constexpr inline auto NotificationCenterPanel::qt_create_metaobjectdata<qt_meta_tag_ZN23NotificationCenterPanelE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "NotificationCenterPanel",
        "notificationClosed",
        "",
        "id",
        "reason",
        "actionInvoked",
        "actionKey",
        "showNotification",
        "appName",
        "summary",
        "body",
        "iconName",
        "timeoutMs",
        "actions",
        "QVariantMap",
        "hints",
        "closeNotification",
        "reloadConfiguration",
        "openPanel",
        "closePanel",
        "togglePanel",
        "applyConfig",
        "WardNcConfig",
        "config",
        "applyStyle",
        "styleSheet",
        "QHash<QString,QString>",
        "styleVariables",
        "handleStateFileChanged",
        "path",
        "handleStateDirectoryChanged",
        "handleHistoryFileChanged",
        "handleHistoryDirectoryChanged"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'notificationClosed'
        QtMocHelpers::SignalData<void(uint, uint)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::UInt, 3 }, { QMetaType::UInt, 4 },
        }}),
        // Signal 'actionInvoked'
        QtMocHelpers::SignalData<void(uint, const QString &)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::UInt, 3 }, { QMetaType::QString, 6 },
        }}),
        // Slot 'showNotification'
        QtMocHelpers::SlotData<void(uint, const QString &, const QString &, const QString &, const QString &, int, const QStringList &, const QVariantMap &)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::UInt, 3 }, { QMetaType::QString, 8 }, { QMetaType::QString, 9 }, { QMetaType::QString, 10 },
            { QMetaType::QString, 11 }, { QMetaType::Int, 12 }, { QMetaType::QStringList, 13 }, { 0x80000000 | 14, 15 },
        }}),
        // Slot 'closeNotification'
        QtMocHelpers::SlotData<void(uint, uint)>(16, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::UInt, 3 }, { QMetaType::UInt, 4 },
        }}),
        // Slot 'closeNotification'
        QtMocHelpers::SlotData<void(uint)>(16, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Void, {{
            { QMetaType::UInt, 3 },
        }}),
        // Slot 'reloadConfiguration'
        QtMocHelpers::SlotData<void()>(17, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'openPanel'
        QtMocHelpers::SlotData<void()>(18, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'closePanel'
        QtMocHelpers::SlotData<void()>(19, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'togglePanel'
        QtMocHelpers::SlotData<void()>(20, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'applyConfig'
        QtMocHelpers::SlotData<void(const WardNcConfig &)>(21, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 22, 23 },
        }}),
        // Slot 'applyStyle'
        QtMocHelpers::SlotData<void(const QString &, const QHash<QString,QString> &)>(24, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 25 }, { 0x80000000 | 26, 27 },
        }}),
        // Slot 'handleStateFileChanged'
        QtMocHelpers::SlotData<void(const QString &)>(28, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 29 },
        }}),
        // Slot 'handleStateDirectoryChanged'
        QtMocHelpers::SlotData<void(const QString &)>(30, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 29 },
        }}),
        // Slot 'handleHistoryFileChanged'
        QtMocHelpers::SlotData<void(const QString &)>(31, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 29 },
        }}),
        // Slot 'handleHistoryDirectoryChanged'
        QtMocHelpers::SlotData<void(const QString &)>(32, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 29 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<NotificationCenterPanel, qt_meta_tag_ZN23NotificationCenterPanelE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject NotificationCenterPanel::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN23NotificationCenterPanelE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN23NotificationCenterPanelE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN23NotificationCenterPanelE_t>.metaTypes,
    nullptr
} };

void NotificationCenterPanel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<NotificationCenterPanel *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->notificationClosed((*reinterpret_cast<std::add_pointer_t<uint>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<uint>>(_a[2]))); break;
        case 1: _t->actionInvoked((*reinterpret_cast<std::add_pointer_t<uint>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 2: _t->showNotification((*reinterpret_cast<std::add_pointer_t<uint>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[4])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[5])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[6])),(*reinterpret_cast<std::add_pointer_t<QStringList>>(_a[7])),(*reinterpret_cast<std::add_pointer_t<QVariantMap>>(_a[8]))); break;
        case 3: _t->closeNotification((*reinterpret_cast<std::add_pointer_t<uint>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<uint>>(_a[2]))); break;
        case 4: _t->closeNotification((*reinterpret_cast<std::add_pointer_t<uint>>(_a[1]))); break;
        case 5: _t->reloadConfiguration(); break;
        case 6: _t->openPanel(); break;
        case 7: _t->closePanel(); break;
        case 8: _t->togglePanel(); break;
        case 9: _t->applyConfig((*reinterpret_cast<std::add_pointer_t<WardNcConfig>>(_a[1]))); break;
        case 10: _t->applyStyle((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QHash<QString,QString>>>(_a[2]))); break;
        case 11: _t->handleStateFileChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 12: _t->handleStateDirectoryChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 13: _t->handleHistoryFileChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 14: _t->handleHistoryDirectoryChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (NotificationCenterPanel::*)(uint , uint )>(_a, &NotificationCenterPanel::notificationClosed, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (NotificationCenterPanel::*)(uint , const QString & )>(_a, &NotificationCenterPanel::actionInvoked, 1))
            return;
    }
}

const QMetaObject *NotificationCenterPanel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *NotificationCenterPanel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN23NotificationCenterPanelE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int NotificationCenterPanel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 15)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 15;
    }
    return _id;
}

// SIGNAL 0
void NotificationCenterPanel::notificationClosed(uint _t1, uint _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2);
}

// SIGNAL 1
void NotificationCenterPanel::actionInvoked(uint _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2);
}
QT_WARNING_POP
