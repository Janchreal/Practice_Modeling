/****************************************************************************
** Meta object code from reading C++ file 'extrusiondialog.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../extrusiondialog.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'extrusiondialog.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN15ExtrusionDialogE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN15ExtrusionDialogE = QtMocHelpers::stringData(
    "ExtrusionDialog",
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
    "parametersChanged",
    "startBooleanTargetSelection",
    "booleanModeChanged",
    "on_okButton_clicked",
    "on_cancelButton_clicked",
    "on_geometrySelectorButton_clicked",
    "on_clearSelectionButton_clicked",
    "on_sectionTypeCombo_currentTextChanged",
    "text",
    "on_startCombo_currentTextChanged",
    "on_endCombo_currentTextChanged",
    "on_previewButton_clicked",
    "on_selectButton_clicked"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN15ExtrusionDialogE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      19,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      10,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  128,    2, 0x06,    1 /* Public */,
       3,    0,  129,    2, 0x06,    2 /* Public */,
       4,    0,  130,    2, 0x06,    3 /* Public */,
       5,    0,  131,    2, 0x06,    4 /* Public */,
       6,    1,  132,    2, 0x06,    5 /* Public */,
       8,    0,  135,    2, 0x06,    7 /* Public */,
       9,    1,  136,    2, 0x06,    8 /* Public */,
      11,    0,  139,    2, 0x06,   10 /* Public */,
      12,    0,  140,    2, 0x06,   11 /* Public */,
      13,    1,  141,    2, 0x06,   12 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      14,    0,  144,    2, 0x08,   14 /* Private */,
      15,    0,  145,    2, 0x08,   15 /* Private */,
      16,    0,  146,    2, 0x08,   16 /* Private */,
      17,    0,  147,    2, 0x08,   17 /* Private */,
      18,    1,  148,    2, 0x08,   18 /* Private */,
      20,    1,  151,    2, 0x08,   20 /* Private */,
      21,    1,  154,    2, 0x08,   22 /* Private */,
      22,    0,  157,    2, 0x08,   24 /* Private */,
      23,    0,  158,    2, 0x08,   25 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   10,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    7,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   19,
    QMetaType::Void, QMetaType::QString,   19,
    QMetaType::Void, QMetaType::QString,   19,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject ExtrusionDialog::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_ZN15ExtrusionDialogE.offsetsAndSizes,
    qt_meta_data_ZN15ExtrusionDialogE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN15ExtrusionDialogE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<ExtrusionDialog, std::true_type>,
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
        // method 'parametersChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'startBooleanTargetSelection'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'booleanModeChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'on_okButton_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_cancelButton_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
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
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void ExtrusionDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ExtrusionDialog *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->previewRequested(); break;
        case 1: _t->cancelPreviewRequested(); break;
        case 2: _t->startSelection(); break;
        case 3: _t->clearSelection(); break;
        case 4: _t->selectionModeChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->vectorSelectionRequested(); break;
        case 6: _t->requestVectorMode((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 7: _t->parametersChanged(); break;
        case 8: _t->startBooleanTargetSelection(); break;
        case 9: _t->booleanModeChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 10: _t->on_okButton_clicked(); break;
        case 11: _t->on_cancelButton_clicked(); break;
        case 12: _t->on_geometrySelectorButton_clicked(); break;
        case 13: _t->on_clearSelectionButton_clicked(); break;
        case 14: _t->on_sectionTypeCombo_currentTextChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 15: _t->on_startCombo_currentTextChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 16: _t->on_endCombo_currentTextChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 17: _t->on_previewButton_clicked(); break;
        case 18: _t->on_selectButton_clicked(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (ExtrusionDialog::*)();
            if (_q_method_type _q_method = &ExtrusionDialog::previewRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (ExtrusionDialog::*)();
            if (_q_method_type _q_method = &ExtrusionDialog::cancelPreviewRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (ExtrusionDialog::*)();
            if (_q_method_type _q_method = &ExtrusionDialog::startSelection; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (ExtrusionDialog::*)();
            if (_q_method_type _q_method = &ExtrusionDialog::clearSelection; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _q_method_type = void (ExtrusionDialog::*)(const QString & );
            if (_q_method_type _q_method = &ExtrusionDialog::selectionModeChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _q_method_type = void (ExtrusionDialog::*)();
            if (_q_method_type _q_method = &ExtrusionDialog::vectorSelectionRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _q_method_type = void (ExtrusionDialog::*)(int );
            if (_q_method_type _q_method = &ExtrusionDialog::requestVectorMode; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _q_method_type = void (ExtrusionDialog::*)();
            if (_q_method_type _q_method = &ExtrusionDialog::parametersChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _q_method_type = void (ExtrusionDialog::*)();
            if (_q_method_type _q_method = &ExtrusionDialog::startBooleanTargetSelection; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
        {
            using _q_method_type = void (ExtrusionDialog::*)(int );
            if (_q_method_type _q_method = &ExtrusionDialog::booleanModeChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 9;
                return;
            }
        }
    }
}

const QMetaObject *ExtrusionDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ExtrusionDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN15ExtrusionDialogE.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int ExtrusionDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 19)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 19;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 19)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 19;
    }
    return _id;
}

// SIGNAL 0
void ExtrusionDialog::previewRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void ExtrusionDialog::cancelPreviewRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void ExtrusionDialog::startSelection()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void ExtrusionDialog::clearSelection()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void ExtrusionDialog::selectionModeChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void ExtrusionDialog::vectorSelectionRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void ExtrusionDialog::requestVectorMode(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void ExtrusionDialog::parametersChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void ExtrusionDialog::startBooleanTargetSelection()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void ExtrusionDialog::booleanModeChanged(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}
QT_WARNING_POP
