/****************************************************************************
** Meta object code from reading C++ file 'widget.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../widget.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'widget.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN6WidgetE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN6WidgetE = QtMocHelpers::stringData(
    "Widget",
    "undoStateChanged",
    "",
    "canUndo",
    "canRedo",
    "on_cuboid_clicked",
    "on_sphere_clicked",
    "on_cylinder_clicked",
    "on_cone_clicked",
    "on_boolOperationButton_clicked",
    "on_history_itemClicked",
    "QListWidgetItem*",
    "item",
    "on_clear_history_clicked",
    "onDeleteHistoryItem",
    "index",
    "on_expressionBtn_clicked",
    "on_extrude_clicked",
    "on_revolve_clicked",
    "on_fillet_clicked",
    "on_hollow_clicked",
    "performRevolution",
    "angle",
    "gp_Ax1",
    "axis",
    "performFillet",
    "radius",
    "performHollow",
    "thickness",
    "isValidShapeForOperation",
    "onBoolSelectionTarget",
    "onBoolSelectionTool",
    "onBoolOperationConfirmed",
    "showAllModels",
    "onExtrusionStartSelection",
    "onExtrusionClearSelection",
    "onExtrusionPreviewRequested",
    "on_undoButton_clicked",
    "on_redoButton_clicked",
    "undo",
    "redo"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN6WidgetE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      31,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    2,  200,    2, 0x06,    1 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       5,    0,  205,    2, 0x08,    4 /* Private */,
       6,    0,  206,    2, 0x08,    5 /* Private */,
       7,    0,  207,    2, 0x08,    6 /* Private */,
       8,    0,  208,    2, 0x08,    7 /* Private */,
       9,    0,  209,    2, 0x08,    8 /* Private */,
      10,    1,  210,    2, 0x08,    9 /* Private */,
      13,    0,  213,    2, 0x08,   11 /* Private */,
      14,    1,  214,    2, 0x08,   12 /* Private */,
      16,    0,  217,    2, 0x08,   14 /* Private */,
      17,    0,  218,    2, 0x08,   15 /* Private */,
      18,    0,  219,    2, 0x08,   16 /* Private */,
      19,    0,  220,    2, 0x08,   17 /* Private */,
      20,    0,  221,    2, 0x08,   18 /* Private */,
      21,    2,  222,    2, 0x08,   19 /* Private */,
      25,    1,  227,    2, 0x08,   22 /* Private */,
      27,    1,  230,    2, 0x08,   24 /* Private */,
      29,    1,  233,    2, 0x08,   26 /* Private */,
      30,    0,  236,    2, 0x08,   28 /* Private */,
      31,    0,  237,    2, 0x08,   29 /* Private */,
      32,    0,  238,    2, 0x08,   30 /* Private */,
      33,    0,  239,    2, 0x08,   31 /* Private */,
      34,    0,  240,    2, 0x08,   32 /* Private */,
      35,    0,  241,    2, 0x08,   33 /* Private */,
      36,    0,  242,    2, 0x08,   34 /* Private */,
      37,    0,  243,    2, 0x08,   35 /* Private */,
      38,    0,  244,    2, 0x08,   36 /* Private */,
      39,    0,  245,    2, 0x0a,   37 /* Public */,
      40,    0,  246,    2, 0x0a,   38 /* Public */,
       3,    0,  247,    2, 0x10a,   39 /* Public | MethodIsConst  */,
       4,    0,  248,    2, 0x10a,   40 /* Public | MethodIsConst  */,

 // signals: parameters
    QMetaType::Void, QMetaType::Bool, QMetaType::Bool,    3,    4,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 11,   12,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   15,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Double, 0x80000000 | 23,   22,   24,
    QMetaType::Void, QMetaType::Double,   26,
    QMetaType::Void, QMetaType::Double,   28,
    QMetaType::Bool, QMetaType::Int,   15,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Bool,
    QMetaType::Bool,

       0        // eod
};

Q_CONSTINIT const QMetaObject Widget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_ZN6WidgetE.offsetsAndSizes,
    qt_meta_data_ZN6WidgetE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN6WidgetE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<Widget, std::true_type>,
        // method 'undoStateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'on_cuboid_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_sphere_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_cylinder_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_cone_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_boolOperationButton_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_history_itemClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QListWidgetItem *, std::false_type>,
        // method 'on_clear_history_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onDeleteHistoryItem'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'on_expressionBtn_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_extrude_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_revolve_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_fillet_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_hollow_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'performRevolution'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        QtPrivate::TypeAndForceComplete<const gp_Ax1 &, std::false_type>,
        // method 'performFillet'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        // method 'performHollow'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        // method 'isValidShapeForOperation'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'onBoolSelectionTarget'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onBoolSelectionTool'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onBoolOperationConfirmed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'showAllModels'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onExtrusionStartSelection'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onExtrusionClearSelection'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onExtrusionPreviewRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_undoButton_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_redoButton_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'undo'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'redo'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'canUndo'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'canRedo'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>
    >,
    nullptr
} };

void Widget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<Widget *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->undoStateChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2]))); break;
        case 1: _t->on_cuboid_clicked(); break;
        case 2: _t->on_sphere_clicked(); break;
        case 3: _t->on_cylinder_clicked(); break;
        case 4: _t->on_cone_clicked(); break;
        case 5: _t->on_boolOperationButton_clicked(); break;
        case 6: _t->on_history_itemClicked((*reinterpret_cast< std::add_pointer_t<QListWidgetItem*>>(_a[1]))); break;
        case 7: _t->on_clear_history_clicked(); break;
        case 8: _t->onDeleteHistoryItem((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 9: _t->on_expressionBtn_clicked(); break;
        case 10: _t->on_extrude_clicked(); break;
        case 11: _t->on_revolve_clicked(); break;
        case 12: _t->on_fillet_clicked(); break;
        case 13: _t->on_hollow_clicked(); break;
        case 14: _t->performRevolution((*reinterpret_cast< std::add_pointer_t<double>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<gp_Ax1>>(_a[2]))); break;
        case 15: _t->performFillet((*reinterpret_cast< std::add_pointer_t<double>>(_a[1]))); break;
        case 16: _t->performHollow((*reinterpret_cast< std::add_pointer_t<double>>(_a[1]))); break;
        case 17: { bool _r = _t->isValidShapeForOperation((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 18: _t->onBoolSelectionTarget(); break;
        case 19: _t->onBoolSelectionTool(); break;
        case 20: _t->onBoolOperationConfirmed(); break;
        case 21: _t->showAllModels(); break;
        case 22: _t->onExtrusionStartSelection(); break;
        case 23: _t->onExtrusionClearSelection(); break;
        case 24: _t->onExtrusionPreviewRequested(); break;
        case 25: _t->on_undoButton_clicked(); break;
        case 26: _t->on_redoButton_clicked(); break;
        case 27: _t->undo(); break;
        case 28: _t->redo(); break;
        case 29: { bool _r = _t->canUndo();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 30: { bool _r = _t->canRedo();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (Widget::*)(bool , bool );
            if (_q_method_type _q_method = &Widget::undoStateChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject *Widget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Widget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN6WidgetE.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int Widget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 31)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 31;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 31)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 31;
    }
    return _id;
}

// SIGNAL 0
void Widget::undoStateChanged(bool _t1, bool _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
