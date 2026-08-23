/********************************************************************************
** Form generated from reading UI file 'sketchtoolinputdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SKETCHTOOLINPUTDIALOG_H
#define UI_SKETCHTOOLINPUTDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_SketchToolInputDialog
{
public:
    QVBoxLayout *verticalLayout_main;
    QWidget *titleBar;
    QHBoxLayout *horizontalLayout_title;
    QToolButton *toolButton_settings;
    QLabel *label_title;
    QSpacerItem *horizontalSpacer_title;
    QToolButton *toolButton_close;
    QWidget *contentWidget;
    QVBoxLayout *verticalLayout_content;
    QLabel *label_objectType;
    QHBoxLayout *horizontalLayout_objectType;
    QToolButton *toolButton_objLine;
    QToolButton *toolButton_objArc;
    QSpacerItem *horizontalSpacer_objectType;
    QLabel *label_inputMode;
    QHBoxLayout *horizontalLayout_modes;
    QToolButton *toolButton_modeCoord;
    QToolButton *toolButton_modeParam;
    QSpacerItem *horizontalSpacer_modes;
    QToolButton *toolButton_sketch;
    QStackedWidget *stackedWidget;
    QWidget *page_coord;
    QGridLayout *gridLayout_coord;
    QLabel *label_xc;
    QLineEdit *lineEdit_xc;
    QLabel *label_yc;
    QLineEdit *lineEdit_yc;
    QWidget *page_line_param;
    QGridLayout *gridLayout_lineParam;
    QLabel *label_length;
    QLineEdit *lineEdit_length;
    QLabel *label_angle;
    QLineEdit *lineEdit_angle;
    QWidget *page_arc_param;
    QGridLayout *gridLayout_arcParam;
    QLabel *label_radius;
    QLineEdit *lineEdit_radius;
    QLabel *label_sweep;
    QLineEdit *lineEdit_sweep;

    void setupUi(QDialog *SketchToolInputDialog)
    {
        if (SketchToolInputDialog->objectName().isEmpty())
            SketchToolInputDialog->setObjectName("SketchToolInputDialog");
        SketchToolInputDialog->resize(280, 200);
        verticalLayout_main = new QVBoxLayout(SketchToolInputDialog);
        verticalLayout_main->setSpacing(0);
        verticalLayout_main->setObjectName("verticalLayout_main");
        verticalLayout_main->setContentsMargins(0, 0, 0, 8);
        titleBar = new QWidget(SketchToolInputDialog);
        titleBar->setObjectName("titleBar");
        titleBar->setMinimumSize(QSize(0, 32));
        titleBar->setStyleSheet(QString::fromUtf8("background-color: #2aa89a;"));
        horizontalLayout_title = new QHBoxLayout(titleBar);
        horizontalLayout_title->setObjectName("horizontalLayout_title");
        horizontalLayout_title->setContentsMargins(8, 4, 4, 4);
        toolButton_settings = new QToolButton(titleBar);
        toolButton_settings->setObjectName("toolButton_settings");
        toolButton_settings->setAutoRaise(true);

        horizontalLayout_title->addWidget(toolButton_settings);

        label_title = new QLabel(titleBar);
        label_title->setObjectName("label_title");
        label_title->setStyleSheet(QString::fromUtf8("color: white; font-weight: bold;"));

        horizontalLayout_title->addWidget(label_title);

        horizontalSpacer_title = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_title->addItem(horizontalSpacer_title);

        toolButton_close = new QToolButton(titleBar);
        toolButton_close->setObjectName("toolButton_close");
        toolButton_close->setAutoRaise(true);

        horizontalLayout_title->addWidget(toolButton_close);


        verticalLayout_main->addWidget(titleBar);

        contentWidget = new QWidget(SketchToolInputDialog);
        contentWidget->setObjectName("contentWidget");
        verticalLayout_content = new QVBoxLayout(contentWidget);
        verticalLayout_content->setObjectName("verticalLayout_content");
        verticalLayout_content->setContentsMargins(10, 8, 10, 4);
        label_objectType = new QLabel(contentWidget);
        label_objectType->setObjectName("label_objectType");

        verticalLayout_content->addWidget(label_objectType);

        horizontalLayout_objectType = new QHBoxLayout();
        horizontalLayout_objectType->setObjectName("horizontalLayout_objectType");
        toolButton_objLine = new QToolButton(contentWidget);
        toolButton_objLine->setObjectName("toolButton_objLine");
        toolButton_objLine->setMinimumSize(QSize(48, 48));
        toolButton_objLine->setCheckable(true);
        toolButton_objLine->setChecked(true);

        horizontalLayout_objectType->addWidget(toolButton_objLine);

        toolButton_objArc = new QToolButton(contentWidget);
        toolButton_objArc->setObjectName("toolButton_objArc");
        toolButton_objArc->setMinimumSize(QSize(48, 48));
        toolButton_objArc->setCheckable(true);

        horizontalLayout_objectType->addWidget(toolButton_objArc);

        horizontalSpacer_objectType = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_objectType->addItem(horizontalSpacer_objectType);


        verticalLayout_content->addLayout(horizontalLayout_objectType);

        label_inputMode = new QLabel(contentWidget);
        label_inputMode->setObjectName("label_inputMode");

        verticalLayout_content->addWidget(label_inputMode);

        horizontalLayout_modes = new QHBoxLayout();
        horizontalLayout_modes->setObjectName("horizontalLayout_modes");
        toolButton_modeCoord = new QToolButton(contentWidget);
        toolButton_modeCoord->setObjectName("toolButton_modeCoord");
        toolButton_modeCoord->setMinimumSize(QSize(48, 48));
        toolButton_modeCoord->setCheckable(true);
        toolButton_modeCoord->setChecked(true);

        horizontalLayout_modes->addWidget(toolButton_modeCoord);

        toolButton_modeParam = new QToolButton(contentWidget);
        toolButton_modeParam->setObjectName("toolButton_modeParam");
        toolButton_modeParam->setMinimumSize(QSize(48, 48));
        toolButton_modeParam->setCheckable(true);

        horizontalLayout_modes->addWidget(toolButton_modeParam);

        horizontalSpacer_modes = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_modes->addItem(horizontalSpacer_modes);

        toolButton_sketch = new QToolButton(contentWidget);
        toolButton_sketch->setObjectName("toolButton_sketch");
        toolButton_sketch->setMinimumSize(QSize(48, 48));

        horizontalLayout_modes->addWidget(toolButton_sketch);


        verticalLayout_content->addLayout(horizontalLayout_modes);

        stackedWidget = new QStackedWidget(contentWidget);
        stackedWidget->setObjectName("stackedWidget");
        page_coord = new QWidget();
        page_coord->setObjectName("page_coord");
        gridLayout_coord = new QGridLayout(page_coord);
        gridLayout_coord->setObjectName("gridLayout_coord");
        label_xc = new QLabel(page_coord);
        label_xc->setObjectName("label_xc");

        gridLayout_coord->addWidget(label_xc, 0, 0, 1, 1);

        lineEdit_xc = new QLineEdit(page_coord);
        lineEdit_xc->setObjectName("lineEdit_xc");

        gridLayout_coord->addWidget(lineEdit_xc, 0, 1, 1, 1);

        label_yc = new QLabel(page_coord);
        label_yc->setObjectName("label_yc");

        gridLayout_coord->addWidget(label_yc, 1, 0, 1, 1);

        lineEdit_yc = new QLineEdit(page_coord);
        lineEdit_yc->setObjectName("lineEdit_yc");

        gridLayout_coord->addWidget(lineEdit_yc, 1, 1, 1, 1);

        stackedWidget->addWidget(page_coord);
        page_line_param = new QWidget();
        page_line_param->setObjectName("page_line_param");
        gridLayout_lineParam = new QGridLayout(page_line_param);
        gridLayout_lineParam->setObjectName("gridLayout_lineParam");
        label_length = new QLabel(page_line_param);
        label_length->setObjectName("label_length");

        gridLayout_lineParam->addWidget(label_length, 0, 0, 1, 1);

        lineEdit_length = new QLineEdit(page_line_param);
        lineEdit_length->setObjectName("lineEdit_length");

        gridLayout_lineParam->addWidget(lineEdit_length, 0, 1, 1, 1);

        label_angle = new QLabel(page_line_param);
        label_angle->setObjectName("label_angle");

        gridLayout_lineParam->addWidget(label_angle, 1, 0, 1, 1);

        lineEdit_angle = new QLineEdit(page_line_param);
        lineEdit_angle->setObjectName("lineEdit_angle");

        gridLayout_lineParam->addWidget(lineEdit_angle, 1, 1, 1, 1);

        stackedWidget->addWidget(page_line_param);
        page_arc_param = new QWidget();
        page_arc_param->setObjectName("page_arc_param");
        gridLayout_arcParam = new QGridLayout(page_arc_param);
        gridLayout_arcParam->setObjectName("gridLayout_arcParam");
        label_radius = new QLabel(page_arc_param);
        label_radius->setObjectName("label_radius");

        gridLayout_arcParam->addWidget(label_radius, 0, 0, 1, 1);

        lineEdit_radius = new QLineEdit(page_arc_param);
        lineEdit_radius->setObjectName("lineEdit_radius");

        gridLayout_arcParam->addWidget(lineEdit_radius, 0, 1, 1, 1);

        label_sweep = new QLabel(page_arc_param);
        label_sweep->setObjectName("label_sweep");

        gridLayout_arcParam->addWidget(label_sweep, 1, 0, 1, 1);

        lineEdit_sweep = new QLineEdit(page_arc_param);
        lineEdit_sweep->setObjectName("lineEdit_sweep");

        gridLayout_arcParam->addWidget(lineEdit_sweep, 1, 1, 1, 1);

        stackedWidget->addWidget(page_arc_param);

        verticalLayout_content->addWidget(stackedWidget);


        verticalLayout_main->addWidget(contentWidget);


        retranslateUi(SketchToolInputDialog);

        stackedWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(SketchToolInputDialog);
    } // setupUi

    void retranslateUi(QDialog *SketchToolInputDialog)
    {
        SketchToolInputDialog->setWindowTitle(QCoreApplication::translate("SketchToolInputDialog", "\350\275\256\345\273\223", nullptr));
        toolButton_settings->setText(QCoreApplication::translate("SketchToolInputDialog", "\342\232\231", nullptr));
        label_title->setText(QCoreApplication::translate("SketchToolInputDialog", "\350\275\256\345\273\223", nullptr));
        toolButton_close->setText(QCoreApplication::translate("SketchToolInputDialog", "\303\227", nullptr));
        label_objectType->setText(QCoreApplication::translate("SketchToolInputDialog", "\345\257\271\350\261\241\347\261\273\345\236\213", nullptr));
        toolButton_objLine->setText(QCoreApplication::translate("SketchToolInputDialog", "\347\233\264\347\272\277", nullptr));
        toolButton_objArc->setText(QCoreApplication::translate("SketchToolInputDialog", "\345\234\206\345\274\247", nullptr));
        label_inputMode->setText(QCoreApplication::translate("SketchToolInputDialog", "\350\276\223\345\205\245\346\250\241\345\274\217", nullptr));
        toolButton_modeCoord->setText(QCoreApplication::translate("SketchToolInputDialog", "XY", nullptr));
        toolButton_modeParam->setText(QCoreApplication::translate("SketchToolInputDialog", "\342\210\240", nullptr));
        toolButton_sketch->setText(QCoreApplication::translate("SketchToolInputDialog", "\350\215\211\345\233\276", nullptr));
        label_xc->setText(QCoreApplication::translate("SketchToolInputDialog", "XC", nullptr));
        label_yc->setText(QCoreApplication::translate("SketchToolInputDialog", "YC", nullptr));
        label_length->setText(QCoreApplication::translate("SketchToolInputDialog", "\351\225\277\345\272\246", nullptr));
        label_angle->setText(QCoreApplication::translate("SketchToolInputDialog", "\350\247\222\345\272\246", nullptr));
        label_radius->setText(QCoreApplication::translate("SketchToolInputDialog", "\345\215\212\345\276\204", nullptr));
        label_sweep->setText(QCoreApplication::translate("SketchToolInputDialog", "\346\211\253\346\216\240\350\247\222", nullptr));
    } // retranslateUi

};

namespace Ui {
    class SketchToolInputDialog: public Ui_SketchToolInputDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SKETCHTOOLINPUTDIALOG_H
