/****************************************************************************
** Meta object code from reading C++ file 'cylinderdialog.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../cylinderdialog.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'cylinderdialog.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN14CylinderDialogE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN14CylinderDialogE = QtMocHelpers::stringData(
    "CylinderDialog",
    "requestPointSelection",
    "",
    "requestPointSelectionWithSnap",
    "snapKind",
    "applyRequested",
    "requestVectorMode",
    "modeIndex",
    "previewRequested",
    "cancelPreviewRequested",
    "on_buttonBox_accepted",
    "on_buttonBox_rejected",
    "on_designated_point_clicked",
    "on_applyButton_clicked",
    "onShowResultButtonClicked"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN14CylinderDialogE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      11,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   80,    2, 0x06,    1 /* Public */,
       3,    1,   81,    2, 0x06,    2 /* Public */,
       5,    0,   84,    2, 0x06,    4 /* Public */,
       6,    1,   85,    2, 0x06,    5 /* Public */,
       8,    0,   88,    2, 0x06,    7 /* Public */,
       9,    0,   89,    2, 0x06,    8 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      10,    0,   90,    2, 0x08,    9 /* Private */,
      11,    0,   91,    2, 0x08,   10 /* Private */,
      12,    0,   92,    2, 0x08,   11 /* Private */,
      13,    0,   93,    2, 0x08,   12 /* Private */,
      14,    0,   94,    2, 0x08,   13 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    4,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    7,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject CylinderDialog::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_ZN14CylinderDialogE.offsetsAndSizes,
    qt_meta_data_ZN14CylinderDialogE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN14CylinderDialogE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<CylinderDialog, std::true_type>,
        // method 'requestPointSelection'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'requestPointSelectionWithSnap'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'applyRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'requestVectorMode'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'previewRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'cancelPreviewRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_buttonBox_accepted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_buttonBox_rejected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_designated_point_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_applyButton_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onShowResultButtonClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void CylinderDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<CylinderDialog *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->requestPointSelection(); break;
        case 1: _t->requestPointSelectionWithSnap((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 2: _t->applyRequested(); break;
        case 3: _t->requestVectorMode((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 4: _t->previewRequested(); break;
        case 5: _t->cancelPreviewRequested(); break;
        case 6: _t->on_buttonBox_accepted(); break;
        case 7: _t->on_buttonBox_rejected(); break;
        case 8: _t->on_designated_point_clicked(); break;
        case 9: _t->on_applyButton_clicked(); break;
        case 10: _t->onShowResultButtonClicked(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (CylinderDialog::*)();
            if (_q_method_type _q_method = &CylinderDialog::requestPointSelection; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (CylinderDialog::*)(int );
            if (_q_method_type _q_method = &CylinderDialog::requestPointSelectionWithSnap; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (CylinderDialog::*)();
            if (_q_method_type _q_method = &CylinderDialog::applyRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (CylinderDialog::*)(int );
            if (_q_method_type _q_method = &CylinderDialog::requestVectorMode; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _q_method_type = void (CylinderDialog::*)();
            if (_q_method_type _q_method = &CylinderDialog::previewRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _q_method_type = void (CylinderDialog::*)();
            if (_q_method_type _q_method = &CylinderDialog::cancelPreviewRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
    }
}

const QMetaObject *CylinderDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CylinderDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN14CylinderDialogE.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int CylinderDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
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
void CylinderDialog::requestPointSelection()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void CylinderDialog::requestPointSelectionWithSnap(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void CylinderDialog::applyRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void CylinderDialog::requestVectorMode(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void CylinderDialog::previewRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void CylinderDialog::cancelPreviewRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}
QT_WARNING_POP
