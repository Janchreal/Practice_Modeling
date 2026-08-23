/****************************************************************************
** Meta object code from reading C++ file 'sketchtoolinputdialog.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../sketchtoolinputdialog.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'sketchtoolinputdialog.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN21SketchToolInputDialogE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN21SketchToolInputDialogE = QtMocHelpers::stringData(
    "SketchToolInputDialog",
    "closedByUser",
    "",
    "inputKindChanged",
    "SketchToolInputDialog::InputKind",
    "k",
    "objectKindChanged",
    "SketchToolInputDialog::ObjectKind",
    "valuesCommitted",
    "sketchButtonClicked",
    "onCloseClicked",
    "onModeCoordToggled",
    "checked",
    "onModeParamToggled",
    "onObjLineToggled",
    "onObjArcToggled",
    "onAnyEditingFinished",
    "onSketchClicked"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN21SketchToolInputDialogE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      12,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   86,    2, 0x06,    1 /* Public */,
       3,    1,   87,    2, 0x06,    2 /* Public */,
       6,    1,   90,    2, 0x06,    4 /* Public */,
       8,    0,   93,    2, 0x06,    6 /* Public */,
       9,    0,   94,    2, 0x06,    7 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      10,    0,   95,    2, 0x08,    8 /* Private */,
      11,    1,   96,    2, 0x08,    9 /* Private */,
      13,    1,   99,    2, 0x08,   11 /* Private */,
      14,    1,  102,    2, 0x08,   13 /* Private */,
      15,    1,  105,    2, 0x08,   15 /* Private */,
      16,    0,  108,    2, 0x08,   17 /* Private */,
      17,    0,  109,    2, 0x08,   18 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 4,    5,
    QMetaType::Void, 0x80000000 | 7,    5,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   12,
    QMetaType::Void, QMetaType::Bool,   12,
    QMetaType::Void, QMetaType::Bool,   12,
    QMetaType::Void, QMetaType::Bool,   12,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject SketchToolInputDialog::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_ZN21SketchToolInputDialogE.offsetsAndSizes,
    qt_meta_data_ZN21SketchToolInputDialogE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN21SketchToolInputDialogE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<SketchToolInputDialog, std::true_type>,
        // method 'closedByUser'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'inputKindChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<SketchToolInputDialog::InputKind, std::false_type>,
        // method 'objectKindChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<SketchToolInputDialog::ObjectKind, std::false_type>,
        // method 'valuesCommitted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'sketchButtonClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onCloseClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onModeCoordToggled'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onModeParamToggled'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onObjLineToggled'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onObjArcToggled'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onAnyEditingFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onSketchClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void SketchToolInputDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<SketchToolInputDialog *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->closedByUser(); break;
        case 1: _t->inputKindChanged((*reinterpret_cast< std::add_pointer_t<SketchToolInputDialog::InputKind>>(_a[1]))); break;
        case 2: _t->objectKindChanged((*reinterpret_cast< std::add_pointer_t<SketchToolInputDialog::ObjectKind>>(_a[1]))); break;
        case 3: _t->valuesCommitted(); break;
        case 4: _t->sketchButtonClicked(); break;
        case 5: _t->onCloseClicked(); break;
        case 6: _t->onModeCoordToggled((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 7: _t->onModeParamToggled((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 8: _t->onObjLineToggled((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 9: _t->onObjArcToggled((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 10: _t->onAnyEditingFinished(); break;
        case 11: _t->onSketchClicked(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (SketchToolInputDialog::*)();
            if (_q_method_type _q_method = &SketchToolInputDialog::closedByUser; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (SketchToolInputDialog::*)(SketchToolInputDialog::InputKind );
            if (_q_method_type _q_method = &SketchToolInputDialog::inputKindChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (SketchToolInputDialog::*)(SketchToolInputDialog::ObjectKind );
            if (_q_method_type _q_method = &SketchToolInputDialog::objectKindChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (SketchToolInputDialog::*)();
            if (_q_method_type _q_method = &SketchToolInputDialog::valuesCommitted; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _q_method_type = void (SketchToolInputDialog::*)();
            if (_q_method_type _q_method = &SketchToolInputDialog::sketchButtonClicked; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
    }
}

const QMetaObject *SketchToolInputDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SketchToolInputDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN21SketchToolInputDialogE.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int SketchToolInputDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
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
void SketchToolInputDialog::closedByUser()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void SketchToolInputDialog::inputKindChanged(SketchToolInputDialog::InputKind _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void SketchToolInputDialog::objectKindChanged(SketchToolInputDialog::ObjectKind _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void SketchToolInputDialog::valuesCommitted()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void SketchToolInputDialog::sketchButtonClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}
QT_WARNING_POP
