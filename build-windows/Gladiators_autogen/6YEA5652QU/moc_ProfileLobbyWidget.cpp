/****************************************************************************
** Meta object code from reading C++ file 'ProfileLobbyWidget.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../include/ProfileLobbyWidget.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ProfileLobbyWidget.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN18ProfileLobbyWidgetE_t {};
} // unnamed namespace

template <> constexpr inline auto ProfileLobbyWidget::qt_create_metaobjectdata<qt_meta_tag_ZN18ProfileLobbyWidgetE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ProfileLobbyWidget",
        "enterArenaClicked",
        "",
        "modeName",
        "changeCharacterClicked",
        "settingsActionTriggered",
        "actionName",
        "usernameEditRequested",
        "setUserProfile",
        "UserProfile",
        "profile",
        "setSelectedCharacter",
        "Character",
        "character",
        "setSelectedMode",
        "setDuelSetup",
        "DuelSetup",
        "setup",
        "updateProgression",
        "PlayerProgression",
        "stats",
        "showRankUpgradePopup",
        "previousRank",
        "newRank",
        "showCharacterUnlockPopup",
        "characterName",
        "rankName",
        "imagePath"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'enterArenaClicked'
        QtMocHelpers::SignalData<void(const QString &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Signal 'changeCharacterClicked'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'settingsActionTriggered'
        QtMocHelpers::SignalData<void(const QString &)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 6 },
        }}),
        // Signal 'usernameEditRequested'
        QtMocHelpers::SignalData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'setUserProfile'
        QtMocHelpers::SlotData<void(const UserProfile &)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 9, 10 },
        }}),
        // Slot 'setSelectedCharacter'
        QtMocHelpers::SlotData<void(const Character &)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 12, 13 },
        }}),
        // Slot 'setSelectedMode'
        QtMocHelpers::SlotData<void(const QString &)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Slot 'setDuelSetup'
        QtMocHelpers::SlotData<void(const DuelSetup &)>(15, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 16, 17 },
        }}),
        // Slot 'updateProgression'
        QtMocHelpers::SlotData<void(const PlayerProgression &)>(18, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 19, 20 },
        }}),
        // Slot 'showRankUpgradePopup'
        QtMocHelpers::SlotData<void(const QString &, const QString &)>(21, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 22 }, { QMetaType::QString, 23 },
        }}),
        // Slot 'showCharacterUnlockPopup'
        QtMocHelpers::SlotData<void(const QString &, const QString &, const QString &)>(24, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 25 }, { QMetaType::QString, 26 }, { QMetaType::QString, 27 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<ProfileLobbyWidget, qt_meta_tag_ZN18ProfileLobbyWidgetE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ProfileLobbyWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN18ProfileLobbyWidgetE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN18ProfileLobbyWidgetE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN18ProfileLobbyWidgetE_t>.metaTypes,
    nullptr
} };

void ProfileLobbyWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ProfileLobbyWidget *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->enterArenaClicked((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->changeCharacterClicked(); break;
        case 2: _t->settingsActionTriggered((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->usernameEditRequested(); break;
        case 4: _t->setUserProfile((*reinterpret_cast<std::add_pointer_t<UserProfile>>(_a[1]))); break;
        case 5: _t->setSelectedCharacter((*reinterpret_cast<std::add_pointer_t<Character>>(_a[1]))); break;
        case 6: _t->setSelectedMode((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 7: _t->setDuelSetup((*reinterpret_cast<std::add_pointer_t<DuelSetup>>(_a[1]))); break;
        case 8: _t->updateProgression((*reinterpret_cast<std::add_pointer_t<PlayerProgression>>(_a[1]))); break;
        case 9: _t->showRankUpgradePopup((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 10: _t->showCharacterUnlockPopup((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (ProfileLobbyWidget::*)(const QString & )>(_a, &ProfileLobbyWidget::enterArenaClicked, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (ProfileLobbyWidget::*)()>(_a, &ProfileLobbyWidget::changeCharacterClicked, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (ProfileLobbyWidget::*)(const QString & )>(_a, &ProfileLobbyWidget::settingsActionTriggered, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (ProfileLobbyWidget::*)()>(_a, &ProfileLobbyWidget::usernameEditRequested, 3))
            return;
    }
}

const QMetaObject *ProfileLobbyWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ProfileLobbyWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN18ProfileLobbyWidgetE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int ProfileLobbyWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 11)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 11;
    }
    return _id;
}

// SIGNAL 0
void ProfileLobbyWidget::enterArenaClicked(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void ProfileLobbyWidget::changeCharacterClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void ProfileLobbyWidget::settingsActionTriggered(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void ProfileLobbyWidget::usernameEditRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}
QT_WARNING_POP
