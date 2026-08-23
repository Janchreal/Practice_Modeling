#ifndef MODELINGHISTORY_H
#define MODELINGHISTORY_H

#include "axisdirection.h"
#include "modeltype.h"

#include "featurerecipe.h"

#include <QColor>
#include <QDateTime>
#include <QString>

#include <gp_Dir.hxx>
#include <TopoDS_Shape.hxx>

#include <IVtkOCC_Shape.hxx>
#include <IVtkTools_ShapeDataSource.hxx>
#include <IVtkTools_DisplayModeFilter.hxx>
#include <IVtkTools_SubPolyDataFilter.hxx>
#include <IVtk_Types.hxx>

#include <vtkActor.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

// 建模历史记录结构（与视图/命令共享的领域数据）
struct ModelingHistory {
    ModelType type = CUBOID;
    QString name;
    QDateTime timestamp;
    class vtkActor* actor = nullptr;
    vtkSmartPointer<vtkActor> outlineActor;
    /** 框架管线边几何（当前视角过滤后），供 outlineActor 持有 */
    vtkSmartPointer<vtkPolyData> outlinePolyData;
    /** 完整拓扑边（含 SUBSHAPE_IDS），视角变化时只做过滤、不 remesh */
    vtkSmartPointer<vtkPolyData> outlineSourcePolyData;
    QColor color;
    double param1 = 0.0;
    double param2 = 0.0;
    double param3 = 0.0;
    vtkSmartPointer<vtkPolyData> polyData;
    TopoDS_Shape occShape;
    Handle(IVtkOCC_Shape) shapeWrapper;
    vtkSmartPointer<IVtkTools_ShapeDataSource> shapeDataSource;
    vtkSmartPointer<IVtkTools_DisplayModeFilter> solidDisplayFilter;
    vtkSmartPointer<IVtkTools_DisplayModeFilter> edgeDisplayFilter;
    vtkSmartPointer<IVtkTools_SubPolyDataFilter> highlightFilter;
    vtkSmartPointer<vtkActor> highlightActor;
    int booleanTargetIndex = -1;
    QList<int> booleanToolIndices;
    int booleanOperationType = -1;
    bool booleanKeepTarget = false;
    bool booleanKeepTool = false;

    FeatureRecipe recipe;
    bool featureRegenerateFailed = false;

    bool hasOrigin = false;
    double originX = 0.0, originY = 0.0, originZ = 0.0;
    AxisDirection axisDirection = AxisDirection::Z;
    bool axisReversed = false;

    bool hasCustomVectorDir = false;
    gp_Dir customVectorDir = gp_Dir(0, 0, 1);
};

#endif
