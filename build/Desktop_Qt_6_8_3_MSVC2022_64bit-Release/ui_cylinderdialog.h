/********************************************************************************
** Form generated from reading UI file 'cylinderdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CYLINDERDIALOG_H
#define UI_CYLINDERDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_CylinderDialog
{
public:
    QGroupBox *groupBox;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QLabel *heigh;
    QDoubleSpinBox *heightSpinBox;
    QHBoxLayout *horizontalLayout_2;
    QLabel *radius;
    QDoubleSpinBox *radiusSpinBox;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *CylinderDialog)
    {
        if (CylinderDialog->objectName().isEmpty())
            CylinderDialog->setObjectName("CylinderDialog");
        CylinderDialog->resize(400, 300);
        groupBox = new QGroupBox(CylinderDialog);
        groupBox->setObjectName("groupBox");
        groupBox->setGeometry(QRect(30, 20, 251, 171));
        verticalLayout = new QVBoxLayout(groupBox);
        verticalLayout->setObjectName("verticalLayout");
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        heigh = new QLabel(groupBox);
        heigh->setObjectName("heigh");

        horizontalLayout->addWidget(heigh);

        heightSpinBox = new QDoubleSpinBox(groupBox);
        heightSpinBox->setObjectName("heightSpinBox");
        heightSpinBox->setMinimum(0.100000000000000);
        heightSpinBox->setSingleStep(0.100000000000000);
        heightSpinBox->setValue(2.000000000000000);

        horizontalLayout->addWidget(heightSpinBox);


        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        radius = new QLabel(groupBox);
        radius->setObjectName("radius");

        horizontalLayout_2->addWidget(radius);

        radiusSpinBox = new QDoubleSpinBox(groupBox);
        radiusSpinBox->setObjectName("radiusSpinBox");
        radiusSpinBox->setMinimum(0.100000000000000);
        radiusSpinBox->setSingleStep(0.100000000000000);
        radiusSpinBox->setValue(2.000000000000000);

        horizontalLayout_2->addWidget(radiusSpinBox);


        verticalLayout->addLayout(horizontalLayout_2);

        buttonBox = new QDialogButtonBox(groupBox);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setStandardButtons(QDialogButtonBox::StandardButton::Cancel|QDialogButtonBox::StandardButton::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(CylinderDialog);

        QMetaObject::connectSlotsByName(CylinderDialog);
    } // setupUi

    void retranslateUi(QDialog *CylinderDialog)
    {
        CylinderDialog->setWindowTitle(QCoreApplication::translate("CylinderDialog", "Dialog", nullptr));
        groupBox->setTitle(QCoreApplication::translate("CylinderDialog", "\345\234\206\346\237\261\344\275\223", nullptr));
        heigh->setText(QCoreApplication::translate("CylinderDialog", "\351\253\230", nullptr));
        radius->setText(QCoreApplication::translate("CylinderDialog", "\345\215\212\345\276\204", nullptr));
    } // retranslateUi

};

namespace Ui {
    class CylinderDialog: public Ui_CylinderDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CYLINDERDIALOG_H
