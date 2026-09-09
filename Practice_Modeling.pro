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
    $$files($$PWD/src/*.cpp, true) \
    $$files($$PWD/3rdparty/SARibbon/*.cpp)

HEADERS += \
    $$files($$PWD/src/*.h, true) \
    $$files($$PWD/3rdparty/SARibbon/*.h)

FORMS += \
    $$files($$PWD/src/*.ui, true)

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

QMAKE_PROJECT_DEPTH = 0

# 分层源码目录。保留各模块目录在 include path 中，兼容现有的短文件名 include，
# 后续可逐步迁移为 src/<module>/... 的显式 include。
INCLUDEPATH += \
    $$PWD/src \
    $$PWD/src/app \
    $$PWD/src/application \
    $$PWD/src/application/command \
    $$PWD/src/application/commands \
    $$PWD/src/application/history \
    $$PWD/src/application/ports \
    $$PWD/src/common \
    $$PWD/src/domain \
    $$PWD/src/domain/features \
    $$PWD/src/domain/sketch \
    $$PWD/src/geometry \
    $$PWD/src/geometry/math \
    $$PWD/src/geometry/primitives \
    $$PWD/src/geometry/boolean \
    $$PWD/src/geometry/extrusion \
    $$PWD/src/geometry/revolution \
    $$PWD/src/geometry/pattern \
    $$PWD/src/geometry/modification \
    $$PWD/src/geometry/placement \
    $$PWD/src/geometry/topology \
    $$PWD/src/geometry/sketch \
    $$PWD/src/geometry/runtime \
    $$PWD/src/interaction \
    $$PWD/src/interaction/coordinates \
    $$PWD/src/interaction/handles \
    $$PWD/src/interaction/tools \
    $$PWD/src/interaction/selection \
    $$PWD/src/rendering \
    $$PWD/src/rendering/adapters \
    $$PWD/src/rendering/pipeline \
    $$PWD/src/rendering/model \
    $$PWD/src/rendering/handles \
    $$PWD/src/viewport \
    $$PWD/src/viewport/coordinates \
    $$PWD/src/viewport/main_view \
    $$PWD/src/viewport/mirror \
    $$PWD/src/presentation \
    $$PWD/src/presentation/dialogs \
    $$PWD/src/presentation/dialogs/boolean \
    $$PWD/src/presentation/dialogs/common \
    $$PWD/src/presentation/dialogs/coordinates \
    $$PWD/src/presentation/dialogs/extrude_revolve \
    $$PWD/src/presentation/dialogs/history \
    $$PWD/src/presentation/dialogs/modification \
    $$PWD/src/presentation/dialogs/pattern \
    $$PWD/src/presentation/dialogs/primitives \
    $$PWD/src/presentation/dialogs/sketch \
    $$PWD/src/presentation/dialogs/tools \
    $$PWD/src/presentation/features \
    $$PWD/src/presentation/features/primitives \
    $$PWD/src/presentation/features/extrude_revolve \
    $$PWD/src/presentation/features/modification \
    $$PWD/src/presentation/features/boolean \
    $$PWD/src/presentation/features/pattern \
    $$PWD/src/presentation/main_window \
    $$PWD/src/infrastructure \
    $$PWD/src/infrastructure/serialization

# SARibbon 静态嵌入
INCLUDEPATH += $$PWD/3rdparty/SARibbon

win32{
    VTK_DIR = D:/VTK9.4.2/install
    INCLUDEPATH += $${VTK_DIR}/include/vtk-9.4
    LIBS += $$files($${VTK_DIR}/lib/vtk*.lib)

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
