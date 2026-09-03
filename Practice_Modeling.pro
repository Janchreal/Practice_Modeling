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
    $$files($$PWD/controller/*.cpp) \
    $$files($$PWD/core/*.cpp) \
    $$files($$PWD/model/*.cpp) \
    $$files($$PWD/view/*.cpp) \
    $$files($$PWD/3rdparty/SARibbon/*.cpp)

HEADERS += \
    $$files($$PWD/controller/*.h) \
    $$files($$PWD/core/*.h) \
    $$files($$PWD/model/*.h) \
    $$files($$PWD/view/*.h) \
    $$files($$PWD/3rdparty/SARibbon/*.h)

FORMS += \
    $$files($$PWD/view/*.ui)

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

QMAKE_PROJECT_DEPTH = 0

# 分层源码目录
INCLUDEPATH += $$PWD/model
INCLUDEPATH += $$PWD/controller
INCLUDEPATH += $$PWD/view
INCLUDEPATH += $$PWD/core

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
