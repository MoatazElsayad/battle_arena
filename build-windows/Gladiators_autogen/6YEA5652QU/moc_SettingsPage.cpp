/****************************************************************************
** Meta object code from reading C++ file 'SettingsPage.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../include/SettingsPage.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'SettingsPage.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.10.0. It"
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
struct qt_meta_tag_ZN12SettingsPageE_t {};
} // unnamed namespace

template <> constexpr inline auto SettingsPage::qt_create_metaobjectdata<qt_meta_tag_ZN12SettingsPageE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "SettingsPage",
        "backClicked",
        "",
        "settingsChanged",
        "musicVolume",
        "sfxVolume",
        "DifficultyLevel",
        "difficulty",
        "onBackClicked",
        "onMusicVolumeChanged",
        "value",
        "onSfxVolumeChanged",
        "refreshInputStatus"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'backClicked'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'settingsChanged'
        QtMocHelpers::SignalData<void(int, int, DifficultyLevel)>(3, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 4 }, { QMetaType::Int, 5 }, { 0x80000000 | 6, 7 },
        }}),
        // Slot 'onBackClicked'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onMusicVolumeChanged'
        QtMocHelpers::SlotData<void(int)>(9, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 10 },
        }}),
        // Slot 'onSfxVolumeChanged'
        QtMocHelpers::SlotData<void(int)>(11, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 10 },
        }}),
        // Slot 'refreshInputStatus'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<SettingsPage, qt_meta_tag_ZN12SettingsPageE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject SettingsPage::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12SettingsPageE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12SettingsPageE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN12SettingsPageE_t>.metaTypes,
    nullptr
} };

void SettingsPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<SettingsPage *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->backClicked(); break;
        case 1: _t->settingsChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<DifficultyLevel>>(_a[3]))); break;
        case 2: _t->onBackClicked(); break;
        case 3: _t->onMusicVolumeChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 4: _t->onSfxVolumeChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 5: _t->refreshInputStatus(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (SettingsPage::*)()>(_a, &SettingsPage::backClicked, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsPage::*)(int , int , DifficultyLevel )>(_a, &SettingsPage::settingsChanged, 1))
            return;
    }
}

const QMetaObject *SettingsPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SettingsPage::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12SettingsPageE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int SettingsPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void SettingsPage::backClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void SettingsPage::settingsChanged(int _t1, int _t2, DifficultyLevel _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2, _t3);
}
QT_WARNING_POP
