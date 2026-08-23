/****************************************************************************
** Meta object code from reading C++ file 'widget.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../widget.h"
#include <QtGui/qtextcursor.h>
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
    "handleNewFile",
    "handleOpenFile",
    "handleSaveFile",
    "handleSaveFileAs",
    "on_cuboid_clicked",
    "on_sphere_clicked",
    "on_cylinder_clicked",
    "on_cone_clicked",
    "on_boolOperationButton_clicked",
    "handleHistoryItemClicked",
    "QListWidgetItem*",
    "item",
    "handleClearHistory",
    "onDeleteHistoryItem",
    "index",
    "on_expressionBtn_clicked",
    "on_extrude_clicked",
    "on_revolve_clicked",
    "on_fillet_clicked",
    "on_chamfer_clicked",
    "on_patternFeature_clicked",
    "handleHollow",
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
    "onExtrusionCancelPreviewRequested",
    "onExtrusionSelectionModeChanged",
    "mode",
    "onRevolveStartSelection",
    "onRevolveClearSelection",
    "onRevolvePreviewRequested",
    "onRevolveCancelPreviewRequested",
    "onRevolveSelectionModeChanged",
    "on_undoButton_clicked",
    "on_redoButton_clicked",
    "on_WindowpushButton_clicked",
    "createNewWindow",
    "showWindowLayoutDialog",
    "resetWindowLayout",
    "switchActiveWindow",
    "changeDisplayWidget",
    "onDockWidgetLocationChanged",
    "Qt::DockWidgetArea",
    "area",
    "on_workAxisButton_clicked",
    "on_pushButton_4_clicked",
    "on_datum_plane_Button_clicked",
    "on_pushButton_6_clicked",
    "on_createSketchButton_clicked",
    "on_pushButton_5_clicked",
    "on_pushButton_7_clicked",
    "on_pushButton_40_clicked",
    "on_pushButton_41_clicked",
    "on_pushButton_42_clicked",
    "on_pushButton_11_clicked",
    "on_pushButton_12_clicked",
    "on_pushButton_13_clicked",
    "on_pushButton_9_clicked",
    "on_pushButton_10_clicked",
    "startSketchQuickTrim",
    "startSketchQuickExtend",
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
      68,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    2,  422,    2, 0x06,    1 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       5,    0,  427,    2, 0x08,    4 /* Private */,
       6,    0,  428,    2, 0x08,    5 /* Private */,
       7,    0,  429,    2, 0x08,    6 /* Private */,
       8,    0,  430,    2, 0x08,    7 /* Private */,
       9,    0,  431,    2, 0x08,    8 /* Private */,
      10,    0,  432,    2, 0x08,    9 /* Private */,
      11,    0,  433,    2, 0x08,   10 /* Private */,
      12,    0,  434,    2, 0x08,   11 /* Private */,
      13,    0,  435,    2, 0x08,   12 /* Private */,
      14,    1,  436,    2, 0x08,   13 /* Private */,
      17,    0,  439,    2, 0x08,   15 /* Private */,
      18,    1,  440,    2, 0x08,   16 /* Private */,
      20,    0,  443,    2, 0x08,   18 /* Private */,
      21,    0,  444,    2, 0x08,   19 /* Private */,
      22,    0,  445,    2, 0x08,   20 /* Private */,
      23,    0,  446,    2, 0x08,   21 /* Private */,
      24,    0,  447,    2, 0x08,   22 /* Private */,
      25,    0,  448,    2, 0x08,   23 /* Private */,
      26,    0,  449,    2, 0x08,   24 /* Private */,
      27,    2,  450,    2, 0x08,   25 /* Private */,
      31,    1,  455,    2, 0x08,   28 /* Private */,
      33,    1,  458,    2, 0x08,   30 /* Private */,
      35,    1,  461,    2, 0x08,   32 /* Private */,
      36,    0,  464,    2, 0x08,   34 /* Private */,
      37,    0,  465,    2, 0x08,   35 /* Private */,
      38,    0,  466,    2, 0x08,   36 /* Private */,
      39,    0,  467,    2, 0x08,   37 /* Private */,
      40,    0,  468,    2, 0x08,   38 /* Private */,
      41,    0,  469,    2, 0x08,   39 /* Private */,
      42,    0,  470,    2, 0x08,   40 /* Private */,
      43,    0,  471,    2, 0x08,   41 /* Private */,
      44,    1,  472,    2, 0x08,   42 /* Private */,
      46,    0,  475,    2, 0x08,   44 /* Private */,
      47,    0,  476,    2, 0x08,   45 /* Private */,
      48,    0,  477,    2, 0x08,   46 /* Private */,
      49,    0,  478,    2, 0x08,   47 /* Private */,
      50,    1,  479,    2, 0x08,   48 /* Private */,
      51,    0,  482,    2, 0x08,   50 /* Private */,
      52,    0,  483,    2, 0x08,   51 /* Private */,
      53,    0,  484,    2, 0x08,   52 /* Private */,
      54,    0,  485,    2, 0x08,   53 /* Private */,
      55,    0,  486,    2, 0x08,   54 /* Private */,
      56,    0,  487,    2, 0x08,   55 /* Private */,
      57,    0,  488,    2, 0x08,   56 /* Private */,
      58,    0,  489,    2, 0x08,   57 /* Private */,
      59,    1,  490,    2, 0x08,   58 /* Private */,
      62,    0,  493,    2, 0x08,   60 /* Private */,
      63,    0,  494,    2, 0x08,   61 /* Private */,
      64,    0,  495,    2, 0x08,   62 /* Private */,
      65,    0,  496,    2, 0x08,   63 /* Private */,
      66,    0,  497,    2, 0x08,   64 /* Private */,
      67,    0,  498,    2, 0x08,   65 /* Private */,
      68,    0,  499,    2, 0x08,   66 /* Private */,
      69,    0,  500,    2, 0x08,   67 /* Private */,
      70,    0,  501,    2, 0x08,   68 /* Private */,
      71,    0,  502,    2, 0x08,   69 /* Private */,
      72,    0,  503,    2, 0x08,   70 /* Private */,
      73,    0,  504,    2, 0x08,   71 /* Private */,
      74,    0,  505,    2, 0x08,   72 /* Private */,
      75,    0,  506,    2, 0x08,   73 /* Private */,
      76,    0,  507,    2, 0x08,   74 /* Private */,
      77,    0,  508,    2, 0x08,   75 /* Private */,
      78,    0,  509,    2, 0x08,   76 /* Private */,
      79,    0,  510,    2, 0x0a,   77 /* Public */,
      80,    0,  511,    2, 0x0a,   78 /* Public */,
       3,    0,  512,    2, 0x10a,   79 /* Public | MethodIsConst  */,
       4,    0,  513,    2, 0x10a,   80 /* Public | MethodIsConst  */,

 // signals: parameters
    QMetaType::Void, QMetaType::Bool, QMetaType::Bool,    3,    4,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 15,   16,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   19,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Double, 0x80000000 | 29,   28,   30,
    QMetaType::Void, QMetaType::Double,   32,
    QMetaType::Void, QMetaType::Double,   34,
    QMetaType::Bool, QMetaType::Int,   19,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   45,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   45,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 60,   61,
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
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
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
        // method 'handleNewFile'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'handleOpenFile'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'handleSaveFile'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'handleSaveFileAs'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
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
        // method 'handleHistoryItemClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QListWidgetItem *, std::false_type>,
        // method 'handleClearHistory'
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
        // method 'on_chamfer_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_patternFeature_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'handleHollow'
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
        // method 'onExtrusionCancelPreviewRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onExtrusionSelectionModeChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onRevolveStartSelection'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onRevolveClearSelection'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onRevolvePreviewRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onRevolveCancelPreviewRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onRevolveSelectionModeChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'on_undoButton_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_redoButton_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_WindowpushButton_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'createNewWindow'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'showWindowLayoutDialog'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'resetWindowLayout'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'switchActiveWindow'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'changeDisplayWidget'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onDockWidgetLocationChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<Qt::DockWidgetArea, std::false_type>,
        // method 'on_workAxisButton_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_pushButton_4_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_datum_plane_Button_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_pushButton_6_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_createSketchButton_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_pushButton_5_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_pushButton_7_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_pushButton_40_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_pushButton_41_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_pushButton_42_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_pushButton_11_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_pushButton_12_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_pushButton_13_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_pushButton_9_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_pushButton_10_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'startSketchQuickTrim'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'startSketchQuickExtend'
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
        case 1: _t->handleNewFile(); break;
        case 2: _t->handleOpenFile(); break;
        case 3: _t->handleSaveFile(); break;
        case 4: _t->handleSaveFileAs(); break;
        case 5: _t->on_cuboid_clicked(); break;
        case 6: _t->on_sphere_clicked(); break;
        case 7: _t->on_cylinder_clicked(); break;
        case 8: _t->on_cone_clicked(); break;
        case 9: _t->on_boolOperationButton_clicked(); break;
        case 10: _t->handleHistoryItemClicked((*reinterpret_cast< std::add_pointer_t<QListWidgetItem*>>(_a[1]))); break;
        case 11: _t->handleClearHistory(); break;
        case 12: _t->onDeleteHistoryItem((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 13: _t->on_expressionBtn_clicked(); break;
        case 14: _t->on_extrude_clicked(); break;
        case 15: _t->on_revolve_clicked(); break;
        case 16: _t->on_fillet_clicked(); break;
        case 17: _t->on_chamfer_clicked(); break;
        case 18: _t->on_patternFeature_clicked(); break;
        case 19: _t->handleHollow(); break;
        case 20: _t->performRevolution((*reinterpret_cast< std::add_pointer_t<double>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<gp_Ax1>>(_a[2]))); break;
        case 21: _t->performFillet((*reinterpret_cast< std::add_pointer_t<double>>(_a[1]))); break;
        case 22: _t->performHollow((*reinterpret_cast< std::add_pointer_t<double>>(_a[1]))); break;
        case 23: { bool _r = _t->isValidShapeForOperation((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 24: _t->onBoolSelectionTarget(); break;
        case 25: _t->onBoolSelectionTool(); break;
        case 26: _t->onBoolOperationConfirmed(); break;
        case 27: _t->showAllModels(); break;
        case 28: _t->onExtrusionStartSelection(); break;
        case 29: _t->onExtrusionClearSelection(); break;
        case 30: _t->onExtrusionPreviewRequested(); break;
        case 31: _t->onExtrusionCancelPreviewRequested(); break;
        case 32: _t->onExtrusionSelectionModeChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 33: _t->onRevolveStartSelection(); break;
        case 34: _t->onRevolveClearSelection(); break;
        case 35: _t->onRevolvePreviewRequested(); break;
        case 36: _t->onRevolveCancelPreviewRequested(); break;
        case 37: _t->onRevolveSelectionModeChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 38: _t->on_undoButton_clicked(); break;
        case 39: _t->on_redoButton_clicked(); break;
        case 40: _t->on_WindowpushButton_clicked(); break;
        case 41: _t->createNewWindow(); break;
        case 42: _t->showWindowLayoutDialog(); break;
        case 43: _t->resetWindowLayout(); break;
        case 44: _t->switchActiveWindow(); break;
        case 45: _t->changeDisplayWidget(); break;
        case 46: _t->onDockWidgetLocationChanged((*reinterpret_cast< std::add_pointer_t<Qt::DockWidgetArea>>(_a[1]))); break;
        case 47: _t->on_workAxisButton_clicked(); break;
        case 48: _t->on_pushButton_4_clicked(); break;
        case 49: _t->on_datum_plane_Button_clicked(); break;
        case 50: _t->on_pushButton_6_clicked(); break;
        case 51: _t->on_createSketchButton_clicked(); break;
        case 52: _t->on_pushButton_5_clicked(); break;
        case 53: _t->on_pushButton_7_clicked(); break;
        case 54: _t->on_pushButton_40_clicked(); break;
        case 55: _t->on_pushButton_41_clicked(); break;
        case 56: _t->on_pushButton_42_clicked(); break;
        case 57: _t->on_pushButton_11_clicked(); break;
        case 58: _t->on_pushButton_12_clicked(); break;
        case 59: _t->on_pushButton_13_clicked(); break;
        case 60: _t->on_pushButton_9_clicked(); break;
        case 61: _t->on_pushButton_10_clicked(); break;
        case 62: _t->startSketchQuickTrim(); break;
        case 63: _t->startSketchQuickExtend(); break;
        case 64: _t->undo(); break;
        case 65: _t->redo(); break;
        case 66: { bool _r = _t->canUndo();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 67: { bool _r = _t->canRedo();
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
    return QMainWindow::qt_metacast(_clname);
}

int Widget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 68)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 68;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 68)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 68;
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
