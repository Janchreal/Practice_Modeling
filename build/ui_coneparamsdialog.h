/********************************************************************************
** Form generated from reading UI file 'coneparamsdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CONEPARAMSDIALOG_H
#define UI_CONEPARAMSDIALOG_H

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
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ConeParamsDialog
{
public:
    QGroupBox *groupBox;
    QWidget *layoutWidget;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout_3;
    QLabel *radius1;
    QDoubleSpinBox *Radius1SpinBox;
    QHBoxLayout *horizontalLayout;
    QLabel *Radius2;
    QDoubleSpinBox *Radius2SpinBox;
    QHBoxLayout *horizontalLayout_2;
    QLabel *Height;
    QDoubleSpinBox *HeightSpinBox;
    QDialogButtonBox *okandcancel;

    void setupUi(QDialog *ConeParamsDialog)
    {
        if (ConeParamsDialog->objectName().isEmpty())
            ConeParamsDialog->setObjectName("ConeParamsDialog");
        ConeParamsDialog->resize(400, 300);
        groupBox = new QGroupBox(ConeParamsDialog);
        groupBox->setObjectName("groupBox");
        groupBox->setGeometry(QRect(40, 40, 261, 171));
        layoutWidget = new QWidget(groupBox);
        layoutWidget->setObjectName("layoutWidget");
        layoutWidget->setGeometry(QRect(20, 20, 231, 141));
        verticalLayout = new QVBoxLayout(layoutWidget);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        radius1 = new QLabel(layoutWidget);
        radius1->setObjectName("radius1");

        horizontalLayout_3->addWidget(radius1);

        Radius1SpinBox = new QDoubleSpinBox(layoutWidget);
        Radius1SpinBox->setObjectName("Radius1SpinBox");

        horizontalLayout_3->addWidget(Radius1SpinBox);


        verticalLayout->addLayout(horizontalLayout_3);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        Radius2 = new QLabel(layoutWidget);
        Radius2->setObjectName("Radius2");

        horizontalLayout->addWidget(Radius2);

        Radius2SpinBox = new QDoubleSpinBox(layoutWidget);
        Radius2SpinBox->setObjectName("Radius2SpinBox");

        horizontalLayout->addWidget(Radius2SpinBox);


        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        Height = new QLabel(layoutWidget);
        Height->setObjectName("Height");

        horizontalLayout_2->addWidget(Height);

        HeightSpinBox = new QDoubleSpinBox(layoutWidget);
        HeightSpinBox->setObjectName("HeightSpinBox");

        horizontalLayout_2->addWidget(HeightSpinBox);


        verticalLayout->addLayout(horizontalLayout_2);

        okandcancel = new QDialogButtonBox(layoutWidget);
        okandcancel->setObjectName("okandcancel");
        okandcancel->setStandardButtons(QDialogButtonBox::StandardButton::Cancel|QDialogButtonBox::StandardButton::Ok);

        verticalLayout->addWidget(okandcancel);


        retranslateUi(ConeParamsDialog);

        QMetaObject::connectSlotsByName(ConeParamsDialog);
    } // setupUi

    void retranslateUi(QDialog *ConeParamsDialog)
    {
        ConeParamsDialog->setWindowTitle(QCoreApplication::translate("ConeParamsDialog", "Dialog", nullptr));
        groupBox->setTitle(QCoreApplication::translate("ConeParamsDialog", "\345\234\206\351\224\245\345\210\233\345\273\272", nullptr));
        radius1->setText(QCoreApplication::translate("ConeParamsDialog", "\345\272\225\351\203\250\345\215\212\345\276\204", nullptr));
        Radius2->setText(QCoreApplication::translate("ConeParamsDialog", "\351\241\266\351\203\250\345\215\212\345\276\204", nullptr));
        Height->setText(QCoreApplication::translate("ConeParamsDialog", "\351\253\230", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ConeParamsDialog: public Ui_ConeParamsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CONEPARAMSDIALOG_H
