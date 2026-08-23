QT       += core gui
QT       += openglwidgets
QT       += core gui opengl
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QT       += svg

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# SARibbon 静态嵌入：必须 moc SARibbon.h（含大量 Q_OBJECT），否则 LNK2001 metaObject
DEFINES += SA_RIBBON_BAR_NO_EXPORT
DEFINES += SARIBBON_USE_3RDPARTY_FRAMELESSHELPER=0

SOURCES += \
    addmodelcommand.cpp \
    booleancommand.cpp \
    booloperationdialog.cpp \
    chamferdialog.cpp \
    coneparamsdialog.cpp \
    creategeometrycommand.cpp \
    cuboidparamsdialog.cpp \
    cylinderdialog.cpp \
    datum_plane.cpp \
    datum_axis.cpp \
    widget_datum.cpp \
    deletemodelcommand.cpp \
    expressiondialog.cpp \
    extrusioncommand.cpp \
    extrusiondialog.cpp \
    featurerecipe_io.cpp \
    filletdialog.cpp \
    historylistitem.cpp \
    main.cpp \
    occvtkconverter.cpp \
    regeneratemodelcommand.cpp \
    revolvedialog.cpp \
    sketch.cpp \
    sketchcreatedialog.cpp \
    sketchtoolinputdialog.cpp \
    sketchmodedialogs.cpp \
    sketchconicdialog.cpp \
    sketchpolygondialog.cpp \
    sketchellipsedialog.cpp \
    sphereparamsdialog.cpp \
    sweepoperation.cpp \
    sweeppath.cpp \
    updatemodelcommand.cpp \
    vectordialog.cpp \
    widget.cpp \
    widget_ribbon.cpp \
    widget_document.cpp \
    widget_vtk_view.cpp \
    widget_reference_csys.cpp \
    widget_sketch.cpp \
    widget_mouse_interactor.cpp \
    widget_vtk_click.cpp \
    widget_extrusion_face.cpp \
    widget_extrusion.cpp \
    widget_revolve.cpp \
    widget_extrude_revolve_interactive.cpp \
    widget_vector_snap.cpp \
    widget_feature_tree.cpp \
    widget_boolean.cpp \
    feature_topology.cpp \
    widget_feature_regenerate.cpp \
    widget_fillet_chamfer.cpp \
    widget_model_regenerate.cpp \
    widget_undo_restore.cpp \
    widget_history_snapshot.cpp \
    widget_mirror_globals.cpp \
    widget_mirror_window.cpp \
    widget_events.cpp \
    widget_history_highlight.cpp \
    widget_primitives.cpp \
    widget_cuboid_interactive.cpp \
    patternfeaturedialog.cpp \
    patterncommand.cpp \
    widget_pattern.cpp \
    widget_pattern_interactive.cpp \
    widget_occ_shape.cpp \
    nx_main_menu_builder.cpp \
    handle_spec.cpp \
    handle_geometry.cpp \
    3rdparty/SARibbon/SARibbon.cpp

HEADERS += \
    widget_mirror_types.h \
    widget_mirror_globals.h \
    widget_mirror_window.h \
    3rdparty/SARibbon/SARibbon.h \
    addmodelcommand.h \
    axisdirection.h \
    booleancommand.h \
    booloperationdialog.h \
    chamferdialog.h \
    command.h \
    coneparamsdialog.h \
    creategeometrycommand.h \
    cuboidparamsdialog.h \
    patternfeaturedialog.h \
    patterncommand.h \
    regeneratemodelcommand.h \
    cylinderdialog.h \
    datum_plane.h \
    datum_axis.h \
    deletemodelcommand.h \
    expressiondialog.h \
    extrusioncommand.h \
    extrusiondialog.h \
    featurerecipe.h \
    featurerecipe_io.h \
    feature_topology.h \
    filletdialog.h \
    historylistitem.h \
    modeltype.h \
    modelhistorysnapshot.h \
    occvtkconverter.h \
    revolvedialog.h \
    sketch.h \
    sketchcreatedialog.h \
    sketchtoolinputdialog.h \
    sketchmodedialogs.h \
    sketchconicdialog.h \
    sketchpolygondialog.h \
    sketchellipsedialog.h \
    sphereparamsdialog.h \
    sweepoperation.h \
    sweeppath.h \
    updatemodelcommand.h \
    vectordialog.h \
    widget.h \
    handle_spec.h \
    handle_geometry.h \
    nx_main_menu_builder.h

FORMS += \
    booloperationdialog.ui \
    chamferdialog.ui \
    coneparamsdialog.ui \
    cuboidparamsdialog.ui \
    cylinderdialog.ui \
    datum_plane.ui \
    expressiondialog.ui \
    extrusiondialog.ui \
    filletdialog.ui \
    historylistitem.ui \
    revolvedialog.ui \
    sketchcreatedialog.ui \
    sketchtoolinputdialog.ui \
    sphereparamsdialog.ui \
    vectordialog.ui \
    widget.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

QMAKE_PROJECT_DEPTH = 0

# SARibbon 静态嵌入
INCLUDEPATH += $$PWD/3rdparty/SARibbon

win32{
    VTK_DIR = D:/VTK9.4.2/install
    INCLUDEPATH += $${VTK_DIR}/include/vtk-9.4
    LIBS += $${VTK_DIR}/lib/vtk*.lib

# OpenCASCADE 配置
    OCC_DIR = D:/OCCT7.7.0/Install

    INCLUDEPATH += $${OCC_DIR}/inc

    LIBS += -L$${OCC_DIR}/win64/vc14/lib

    # 必需的OpenCASCADE库
    LIBS += -lTKernel \
            -lTKMath \
            -lTKGeomBase \
            -lTKBRep \
            -lTKPrim \
            -lTKShHealing \
            -lTKTopAlgo \
            -lTKGeomAlgo \
            -lTKG2d \
            -lTKG3d \
            -lTKBO \
            -lTKFillet \
            -lTKOffset \
            -lTKHLR \
            -lTKMesh \
            -lTKService \
            -lTKV3d \
            -lTKOpenGl \
            -lTKIVtk \
            -lTKIVtkDraw


    # 编译器设置
    QMAKE_CXXFLAGS += /MD
    CONFIG += debug_and_release

    # 预处理器定义
    DEFINES += OCC_VERSION_MAJOR=7 OCC_VERSION_MINOR=7

    # 链接器设置
    QMAKE_LFLAGS_RELEASE = /INCREMENTAL:NO
    QMAKE_LFLAGS_DEBUG = /INCREMENTAL:NO
}
# 禁用数学常量重定义警告
win32-msvc* {
    # 在包含任何头文件之前定义 _USE_MATH_DEFINES
    DEFINES += _USE_MATH_DEFINES

    # 禁用特定警告
    QMAKE_CXXFLAGS += -wd4005
}

RESOURCES += \
    resource/resources.qrc

DISTFILES += \
    resource/Contour.png \
    resource/sketch_arc.png \
    resource/sketch_circle.png \
    resource/sketch_cuboid.png \
    resource/sketch_line.png \
    resource/sketch_point.png
