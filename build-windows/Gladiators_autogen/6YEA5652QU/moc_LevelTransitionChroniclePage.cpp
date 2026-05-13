/****************************************************************************
** Meta object code from reading C++ file 'LevelTransitionChroniclePage.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../include/LevelTransitionChroniclePage.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'LevelTransitionChroniclePage.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN28LevelTransitionChroniclePageE_t {};
} // unnamed namespace

template <> constexpr inline auto LevelTransitionChroniclePage::qt_create_metaobjectdata<qt_meta_tag_ZN28LevelTransitionChroniclePageE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "LevelTransitionChroniclePage",
        "continueRequested",
        "",
        "replayRequested",
        "level",
        "advanceAnimation",
        "handleContinueClicked",
        "handleAiSummaryReady",
        "ChronicleSummary",
        "summary"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'continueRequested'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'replayRequested'
        QtMocHelpers::SignalData<void(int)>(3, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 4 },
        }}),
        // Slot 'advanceAnimation'
        QtMocHelpers::SlotData<void()>(5, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleContinueClicked'
        QtMocHelpers::SlotData<void()>(6, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleAiSummaryReady'
        QtMocHelpers::SlotData<void(const ChronicleSummary &)>(7, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 8, 9 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<LevelTransitionChroniclePage, qt_meta_tag_ZN28LevelTransitionChroniclePageE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject LevelTransitionChroniclePage::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN28LevelTransitionChroniclePageE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN28LevelTransitionChroniclePageE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN28LevelTransitionChroniclePageE_t>.metaTypes,
    nullptr
} };

void LevelTransitionChroniclePage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<LevelTransitionChroniclePage *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->continueRequested(); break;
        case 1: _t->replayRequested((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 2: _t->advanceAnimation(); break;
        case 3: _t->handleContinueClicked(); break;
        case 4: _t->handleAiSummaryReady((*reinterpret_cast<std::add_pointer_t<ChronicleSummary>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (LevelTransitionChroniclePage::*)()>(_a, &LevelTransitionChroniclePage::continueRequested, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (LevelTransitionChroniclePage::*)(int )>(_a, &LevelTransitionChroniclePage::replayRequested, 1))
            return;
    }
}

const QMetaObject *LevelTransitionChroniclePage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *LevelTransitionChroniclePage::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN28LevelTransitionChroniclePageE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int LevelTransitionChroniclePage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void LevelTransitionChroniclePage::continueRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void LevelTransitionChroniclePage::replayRequested(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}
QT_WARNING_POP
