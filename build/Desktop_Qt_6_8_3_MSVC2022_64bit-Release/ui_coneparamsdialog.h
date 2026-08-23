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
    QWidget *widget;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QLabel *Radius;
    QDoubleSpinBox *RadiusSpinBox;
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
        widget = new QWidget(groupBox);
        widget->setObjectName("widget");
        widget->setGeometry(QRect(20, 20, 231, 141));
        verticalLayout = new QVBoxLayout(widget);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        Radius = new QLabel(widget);
        Radius->setObjectName("Radius");

        horizontalLayout->addWidget(Radius);

        RadiusSpinBox = new QDoubleSpinBox(widget);
        RadiusSpinBox->setObjectName("RadiusSpinBox");

        horizontalLayout->addWidget(RadiusSpinBox);


        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        Height = new QLabel(widget);
        Height->setObjectName("Height");

        horizontalLayout_2->addWidget(Height);

        HeightSpinBox = new QDoubleSpinBox(widget);
        HeightSpinBox->setObjectName("HeightSpinBox");

        horizontalLayout_2->addWidget(HeightSpinBox);


        verticalLayout->addLayout(horizontalLayout_2);

        okandcancel = new QDialogButtonBox(widget);
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
        Radius->setText(QCoreApplication::translate("ConeParamsDialog", "\345\215\212\345\276\204", nullptr));
        Height->setText(QCoreApplication::translate("ConeParamsDialog", "\351\253\230", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ConeParamsDialog: public Ui_ConeParamsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CONEPARAMSDIALOG_H
