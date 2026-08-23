/****************************************************************************
** Meta object code from reading C++ file 'revolvedialog.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../revolvedialog.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'revolvedialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.8.3. It"
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
struct qt_meta_tag_ZN13revolvedialogE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN13revolvedialogE = QtMocHelpers::stringData(
    "revolvedialog",
    "previewRequested",
    "",
    "cancelPreviewRequested",
    "startSelection",
    "clearSelection",
    "selectionModeChanged",
    "mode",
    "vectorSelectionRequested",
    "requestVectorMode",
    "modeIndex",
    "pointSelectionRequested",
    "pointSelectionRequestedWithSnap",
    "snapKind",
    "parametersChanged",
    "startBooleanTargetSelection",
    "booleanModeChanged",
    "on_geometrySelectorButton_clicked",
    "on_clearSelectionButton_clicked",
    "on_sectionTypeCombo_currentTextChanged",
    "text",
    "on_startCombo_currentTextChanged",
    "on_endCombo_currentTextChanged",
    "on_previewButton_clicked",
    "on_selectButton_clicked",
    "on_pushButton_clicked"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN13revolvedialogE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      20,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      12,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  134,    2, 0x06,    1 /* Public */,
       3,    0,  135,    2, 0x06,    2 /* Public */,
       4,    0,  136,    2, 0x06,    3 /* Public */,
       5,    0,  137,    2, 0x06,    4 /* Public */,
       6,    1,  138,    2, 0x06,    5 /* Public */,
       8,    0,  141,    2, 0x06,    7 /* Public */,
       9,    1,  142,    2, 0x06,    8 /* Public */,
      11,    0,  145,    2, 0x06,   10 /* Public */,
      12,    1,  146,    2, 0x06,   11 /* Public */,
      14,    0,  149,    2, 0x06,   13 /* Public */,
      15,    0,  150,    2, 0x06,   14 /* Public */,
      16,    1,  151,    2, 0x06,   15 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      17,    0,  154,    2, 0x08,   17 /* Private */,
      18,    0,  155,    2, 0x08,   18 /* Private */,
      19,    1,  156,    2, 0x08,   19 /* Private */,
      21,    1,  159,    2, 0x08,   21 /* Private */,
      22,    1,  162,    2, 0x08,   23 /* Private */,
      23,    0,  165,    2, 0x08,   25 /* Private */,
      24,    0,  166,    2, 0x08,   26 /* Private */,
      25,    0,  167,    2, 0x08,   27 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   10,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   13,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    7,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   20,
    QMetaType::Void, QMetaType::QString,   20,
    QMetaType::Void, QMetaType::QString,   20,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject revolvedialog::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_ZN13revolvedialogE.offsetsAndSizes,
    qt_meta_data_ZN13revolvedialogE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN13revolvedialogE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<revolvedialog, std::true_type>,
        // method 'previewRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'cancelPreviewRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'startSelection'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'clearSelection'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'selectionModeChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'vectorSelectionRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'requestVectorMode'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'pointSelectionRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'pointSelectionRequestedWithSnap'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'parametersChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'startBooleanTargetSelection'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'booleanModeChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'on_geometrySelectorButton_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_clearSelectionButton_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_sectionTypeCombo_currentTextChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'on_startCombo_currentTextChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'on_endCombo_currentTextChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'on_previewButton_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_selectButton_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_pushButton_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void revolvedialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<revolvedialog *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->previewRequested(); break;
        case 1: _t->cancelPreviewRequested(); break;
        case 2: _t->startSelection(); break;
        case 3: _t->clearSelection(); break;
        case 4: _t->selectionModeChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->vectorSelectionRequested(); break;
        case 6: _t->requestVectorMode((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 7: _t->pointSelectionRequested(); break;
        case 8: _t->pointSelectionRequestedWithSnap((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 9: _t->parametersChanged(); break;
        case 10: _t->startBooleanTargetSelection(); break;
        case 11: _t->booleanModeChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 12: _t->on_geometrySelectorButton_clicked(); break;
        case 13: _t->on_clearSelectionButton_clicked(); break;
        case 14: _t->on_sectionTypeCombo_currentTextChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 15: _t->on_startCombo_currentTextChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 16: _t->on_endCombo_currentTextChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 17: _t->on_previewButton_clicked(); break;
        case 18: _t->on_selectButton_clicked(); break;
        case 19: _t->on_pushButton_clicked(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (revolvedialog::*)();
            if (_q_method_type _q_method = &revolvedialog::previewRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (revolvedialog::*)();
            if (_q_method_type _q_method = &revolvedialog::cancelPreviewRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (revolvedialog::*)();
            if (_q_method_type _q_method = &revolvedialog::startSelection; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (revolvedialog::*)();
            if (_q_method_type _q_method = &revolvedialog::clearSelection; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _q_method_type = void (revolvedialog::*)(const QString & );
            if (_q_method_type _q_method = &revolvedialog::selectionModeChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _q_method_type = void (revolvedialog::*)();
            if (_q_method_type _q_method = &revolvedialog::vectorSelectionRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _q_method_type = void (revolvedialog::*)(int );
            if (_q_method_type _q_method = &revolvedialog::requestVectorMode; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _q_method_type = void (revolvedialog::*)();
            if (_q_method_type _q_method = &revolvedialog::pointSelectionRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _q_method_type = void (revolvedialog::*)(int );
            if (_q_method_type _q_method = &revolvedialog::pointSelectionRequestedWithSnap; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
        {
            using _q_method_type = void (revolvedialog::*)();
            if (_q_method_type _q_method = &revolvedialog::parametersChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 9;
                return;
            }
        }
        {
            using _q_method_type = void (revolvedialog::*)();
            if (_q_method_type _q_method = &revolvedialog::startBooleanTargetSelection; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 10;
                return;
            }
        }
        {
            using _q_method_type = void (revolvedialog::*)(int );
            if (_q_method_type _q_method = &revolvedialog::booleanModeChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 11;
                return;
            }
        }
    }
}

const QMetaObject *revolvedialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *revolvedialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN13revolvedialogE.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int revolvedialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 20)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 20;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 20)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 20;
    }
    return _id;
}

// SIGNAL 0
void revolvedialog::previewRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void revolvedialog::cancelPreviewRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void revolvedialog::startSelection()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void revolvedialog::clearSelection()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void revolvedialog::selectionModeChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void revolvedialog::vectorSelectionRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void revolvedialog::requestVectorMode(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void revolvedialog::pointSelectionRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void revolvedialog::pointSelectionRequestedWithSnap(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void revolvedialog::parametersChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void revolvedialog::startBooleanTargetSelection()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}

// SIGNAL 11
void revolvedialog::booleanModeChanged(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}
QT_WARNING_POP
