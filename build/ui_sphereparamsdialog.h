/********************************************************************************
** Form generated from reading UI file 'sphereparamsdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SPHEREPARAMSDIALOG_H
#define UI_SPHEREPARAMSDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_SphereParamsDialog
{
public:
    QGroupBox *groupBox;
    QWidget *widget;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QLabel *Radius;
    QDoubleSpinBox *radiusSpinBox;
    QHBoxLayout *horizontalLayout_4;
    QLabel *thetaResolution;
    QDoubleSpinBox *thetaResolutionSpinBox;
    QHBoxLayout *horizontalLayout_2;
    QLabel *phiResolution;
    QDoubleSpinBox *phiResolutionSpinBox;
    QWidget *widget1;
    QHBoxLayout *horizontalLayout_3;
    QPushButton *okButton;
    QPushButton *cancleButton;

    void setupUi(QDialog *SphereParamsDialog)
    {
        if (SphereParamsDialog->objectName().isEmpty())
            SphereParamsDialog->setObjectName("SphereParamsDialog");
        SphereParamsDialog->resize(400, 300);
        groupBox = new QGroupBox(SphereParamsDialog);
        groupBox->setObjectName("groupBox");
        groupBox->setGeometry(QRect(70, 20, 301, 211));
        widget = new QWidget(groupBox);
        widget->setObjectName("widget");
        widget->setGeometry(QRect(7, 18, 281, 151));
        verticalLayout = new QVBoxLayout(widget);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        Radius = new QLabel(widget);
        Radius->setObjectName("Radius");

        horizontalLayout->addWidget(Radius);

        radiusSpinBox = new QDoubleSpinBox(widget);
        radiusSpinBox->setObjectName("radiusSpinBox");
        radiusSpinBox->setMinimum(0.100000000000000);
        radiusSpinBox->setSingleStep(0.100000000000000);
        radiusSpinBox->setValue(2.000000000000000);

        horizontalLayout->addWidget(radiusSpinBox);


        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName("horizontalLayout_4");
        thetaResolution = new QLabel(widget);
        thetaResolution->setObjectName("thetaResolution");

        horizontalLayout_4->addWidget(thetaResolution);

        thetaResolutionSpinBox = new QDoubleSpinBox(widget);
        thetaResolutionSpinBox->setObjectName("thetaResolutionSpinBox");
        thetaResolutionSpinBox->setMinimum(0.100000000000000);
        thetaResolutionSpinBox->setSingleStep(0.100000000000000);
        thetaResolutionSpinBox->setValue(0.100000000000000);

        horizontalLayout_4->addWidget(thetaResolutionSpinBox);


        verticalLayout->addLayout(horizontalLayout_4);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        phiResolution = new QLabel(widget);
        phiResolution->setObjectName("phiResolution");

        horizontalLayout_2->addWidget(phiResolution);

        phiResolutionSpinBox = new QDoubleSpinBox(widget);
        phiResolutionSpinBox->setObjectName("phiResolutionSpinBox");
        phiResolutionSpinBox->setMinimum(0.100000000000000);
        phiResolutionSpinBox->setSingleStep(0.100000000000000);
        phiResolutionSpinBox->setValue(0.100000000000000);

        horizontalLayout_2->addWidget(phiResolutionSpinBox);


        verticalLayout->addLayout(horizontalLayout_2);

        widget1 = new QWidget(groupBox);
        widget1->setObjectName("widget1");
        widget1->setGeometry(QRect(170, 170, 118, 20));
        horizontalLayout_3 = new QHBoxLayout(widget1);
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        horizontalLayout_3->setContentsMargins(0, 0, 0, 0);
        okButton = new QPushButton(widget1);
        okButton->setObjectName("okButton");

        horizontalLayout_3->addWidget(okButton);

        cancleButton = new QPushButton(widget1);
        cancleButton->setObjectName("cancleButton");

        horizontalLayout_3->addWidget(cancleButton);


        retranslateUi(SphereParamsDialog);

        QMetaObject::connectSlotsByName(SphereParamsDialog);
    } // setupUi

    void retranslateUi(QDialog *SphereParamsDialog)
    {
        SphereParamsDialog->setWindowTitle(QCoreApplication::translate("SphereParamsDialog", "Dialog", nullptr));
        groupBox->setTitle(QCoreApplication::translate("SphereParamsDialog", "\345\234\206\346\237\261\344\275\223\345\210\233\345\273\272", nullptr));
        Radius->setText(QCoreApplication::translate("SphereParamsDialog", "\345\215\212\345\276\204", nullptr));
        thetaResolution->setText(QCoreApplication::translate("SphereParamsDialog", "\347\273\217\345\272\246\345\210\206\350\276\250\347\216\207", nullptr));
        phiResolution->setText(QCoreApplication::translate("SphereParamsDialog", "\347\272\254\345\272\246\345\210\206\350\276\250\347\216\207", nullptr));
        okButton->setText(QCoreApplication::translate("SphereParamsDialog", "\347\241\256\350\256\244", nullptr));
        cancleButton->setText(QCoreApplication::translate("SphereParamsDialog", "\345\217\226\346\266\210", nullptr));
    } // retranslateUi

};

namespace Ui {
    class SphereParamsDialog: public Ui_SphereParamsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SPHEREPARAMSDIALOG_H
