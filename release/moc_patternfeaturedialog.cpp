/****************************************************************************
** Meta object code from reading C++ file 'patternfeaturedialog.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../patternfeaturedialog.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'patternfeaturedialog.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN20PatternFeatureDialogE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN20PatternFeatureDialogE = QtMocHelpers::stringData(
    "PatternFeatureDialog",
    "requestBodySelection",
    "",
    "requestClearBodies",
    "previewRequested",
    "applyRequested",
    "requestVectorMode",
    "directionIndex",
    "modeIndex",
    "requestOpenVectorDialog",
    "requestPointSelection",
    "requestPointSelectionWithSnap",
    "snapKind",
    "parametersChanged",
    "pitch1Changed",
    "pitch",
    "pitch2Changed"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN20PatternFeatureDialogE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      11,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      11,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   80,    2, 0x06,    1 /* Public */,
       3,    0,   81,    2, 0x06,    2 /* Public */,
       4,    0,   82,    2, 0x06,    3 /* Public */,
       5,    0,   83,    2, 0x06,    4 /* Public */,
       6,    2,   84,    2, 0x06,    5 /* Public */,
       9,    1,   89,    2, 0x06,    8 /* Public */,
      10,    0,   92,    2, 0x06,   10 /* Public */,
      11,    1,   93,    2, 0x06,   11 /* Public */,
      13,    0,   96,    2, 0x06,   13 /* Public */,
      14,    1,   97,    2, 0x06,   14 /* Public */,
      16,    1,  100,    2, 0x06,   16 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,    7,    8,
    QMetaType::Void, QMetaType::Int,    7,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   12,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Double,   15,
    QMetaType::Void, QMetaType::Double,   15,

       0        // eod
};

Q_CONSTINIT const QMetaObject PatternFeatureDialog::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_ZN20PatternFeatureDialogE.offsetsAndSizes,
    qt_meta_data_ZN20PatternFeatureDialogE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN20PatternFeatureDialogE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<PatternFeatureDialog, std::true_type>,
        // method 'requestBodySelection'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'requestClearBodies'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'previewRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'applyRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'requestVectorMode'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'requestOpenVectorDialog'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'requestPointSelection'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'requestPointSelectionWithSnap'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'parametersChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'pitch1Changed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        // method 'pitch2Changed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>
    >,
    nullptr
} };

void PatternFeatureDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<PatternFeatureDialog *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->requestBodySelection(); break;
        case 1: _t->requestClearBodies(); break;
        case 2: _t->previewRequested(); break;
        case 3: _t->applyRequested(); break;
        case 4: _t->requestVectorMode((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 5: _t->requestOpenVectorDialog((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 6: _t->requestPointSelection(); break;
        case 7: _t->requestPointSelectionWithSnap((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 8: _t->parametersChanged(); break;
        case 9: _t->pitch1Changed((*reinterpret_cast< std::add_pointer_t<double>>(_a[1]))); break;
        case 10: _t->pitch2Changed((*reinterpret_cast< std::add_pointer_t<double>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (PatternFeatureDialog::*)();
            if (_q_method_type _q_method = &PatternFeatureDialog::requestBodySelection; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (PatternFeatureDialog::*)();
            if (_q_method_type _q_method = &PatternFeatureDialog::requestClearBodies; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (PatternFeatureDialog::*)();
            if (_q_method_type _q_method = &PatternFeatureDialog::previewRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (PatternFeatureDialog::*)();
            if (_q_method_type _q_method = &PatternFeatureDialog::applyRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _q_method_type = void (PatternFeatureDialog::*)(int , int );
            if (_q_method_type _q_method = &PatternFeatureDialog::requestVectorMode; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _q_method_type = void (PatternFeatureDialog::*)(int );
            if (_q_method_type _q_method = &PatternFeatureDialog::requestOpenVectorDialog; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _q_method_type = void (PatternFeatureDialog::*)();
            if (_q_method_type _q_method = &PatternFeatureDialog::requestPointSelection; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _q_method_type = void (PatternFeatureDialog::*)(int );
            if (_q_method_type _q_method = &PatternFeatureDialog::requestPointSelectionWithSnap; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _q_method_type = void (PatternFeatureDialog::*)();
            if (_q_method_type _q_method = &PatternFeatureDialog::parametersChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
        {
            using _q_method_type = void (PatternFeatureDialog::*)(double );
            if (_q_method_type _q_method = &PatternFeatureDialog::pitch1Changed; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 9;
                return;
            }
        }
        {
            using _q_method_type = void (PatternFeatureDialog::*)(double );
            if (_q_method_type _q_method = &PatternFeatureDialog::pitch2Changed; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 10;
                return;
            }
        }
    }
}

const QMetaObject *PatternFeatureDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PatternFeatureDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN20PatternFeatureDialogE.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int PatternFeatureDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
void PatternFeatureDialog::requestBodySelection()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void PatternFeatureDialog::requestClearBodies()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void PatternFeatureDialog::previewRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void PatternFeatureDialog::applyRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void PatternFeatureDialog::requestVectorMode(int _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void PatternFeatureDialog::requestOpenVectorDialog(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void PatternFeatureDialog::requestPointSelection()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void PatternFeatureDialog::requestPointSelectionWithSnap(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void PatternFeatureDialog::parametersChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void PatternFeatureDialog::pitch1Changed(double _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void PatternFeatureDialog::pitch2Changed(double _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}
QT_WARNING_POP
