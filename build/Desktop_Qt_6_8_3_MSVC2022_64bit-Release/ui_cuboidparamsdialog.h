/********************************************************************************
** Form generated from reading UI file 'cuboidparamsdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CUBOIDPARAMSDIALOG_H
#define UI_CUBOIDPARAMSDIALOG_H

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

class Ui_CuboidParamsDialog
{
public:
    QGroupBox *groupBox;
    QWidget *widget;
    QHBoxLayout *horizontalLayout;
    QPushButton *okButton;
    QPushButton *cancelButton;
    QWidget *widget1;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout_4;
    QLabel *label;
    QDoubleSpinBox *lengthSpinBox;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label_2;
    QDoubleSpinBox *widthSpinBox;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_3;
    QDoubleSpinBox *heightSpinBox;

    void setupUi(QDialog *CuboidParamsDialog)
    {
        if (CuboidParamsDialog->objectName().isEmpty())
            CuboidParamsDialog->setObjectName("CuboidParamsDialog");
        CuboidParamsDialog->resize(400, 300);
        groupBox = new QGroupBox(CuboidParamsDialog);
        groupBox->setObjectName("groupBox");
        groupBox->setGeometry(QRect(40, 50, 281, 221));
        widget = new QWidget(groupBox);
        widget->setObjectName("widget");
        widget->setGeometry(QRect(140, 160, 118, 20));
        horizontalLayout = new QHBoxLayout(widget);
        horizontalLayout->setObjectName("horizontalLayout");
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        okButton = new QPushButton(widget);
        okButton->setObjectName("okButton");

        horizontalLayout->addWidget(okButton);

        cancelButton = new QPushButton(widget);
        cancelButton->setObjectName("cancelButton");

        horizontalLayout->addWidget(cancelButton);

        widget1 = new QWidget(groupBox);
        widget1->setObjectName("widget1");
        widget1->setGeometry(QRect(7, 18, 261, 131));
        verticalLayout = new QVBoxLayout(widget1);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName("horizontalLayout_4");
        label = new QLabel(widget1);
        label->setObjectName("label");

        horizontalLayout_4->addWidget(label);

        lengthSpinBox = new QDoubleSpinBox(widget1);
        lengthSpinBox->setObjectName("lengthSpinBox");
        lengthSpinBox->setMinimum(0.100000000000000);
        lengthSpinBox->setSingleStep(0.100000000000000);
        lengthSpinBox->setValue(2.000000000000000);

        horizontalLayout_4->addWidget(lengthSpinBox);


        verticalLayout->addLayout(horizontalLayout_4);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        label_2 = new QLabel(widget1);
        label_2->setObjectName("label_2");

        horizontalLayout_3->addWidget(label_2);

        widthSpinBox = new QDoubleSpinBox(widget1);
        widthSpinBox->setObjectName("widthSpinBox");
        widthSpinBox->setMinimum(0.100000000000000);
        widthSpinBox->setSingleStep(0.100000000000000);
        widthSpinBox->setValue(2.000000000000000);

        horizontalLayout_3->addWidget(widthSpinBox);


        verticalLayout->addLayout(horizontalLayout_3);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        label_3 = new QLabel(widget1);
        label_3->setObjectName("label_3");

        horizontalLayout_2->addWidget(label_3);

        heightSpinBox = new QDoubleSpinBox(widget1);
        heightSpinBox->setObjectName("heightSpinBox");
        heightSpinBox->setMinimum(0.100000000000000);
        heightSpinBox->setSingleStep(0.100000000000000);
        heightSpinBox->setValue(2.000000000000000);

        horizontalLayout_2->addWidget(heightSpinBox);


        verticalLayout->addLayout(horizontalLayout_2);


        retranslateUi(CuboidParamsDialog);

        QMetaObject::connectSlotsByName(CuboidParamsDialog);
    } // setupUi

    void retranslateUi(QDialog *CuboidParamsDialog)
    {
        CuboidParamsDialog->setWindowTitle(QCoreApplication::translate("CuboidParamsDialog", "Dialog", nullptr));
        groupBox->setTitle(QCoreApplication::translate("CuboidParamsDialog", "\351\225\277\346\226\271\344\275\223\345\210\233\345\273\272", nullptr));
        okButton->setText(QCoreApplication::translate("CuboidParamsDialog", "\347\241\256\350\256\244", nullptr));
        cancelButton->setText(QCoreApplication::translate("CuboidParamsDialog", "\345\217\226\346\266\210", nullptr));
        label->setText(QCoreApplication::translate("CuboidParamsDialog", "\351\225\277\345\272\246", nullptr));
        label_2->setText(QCoreApplication::translate("CuboidParamsDialog", "\345\256\275\345\272\246", nullptr));
        label_3->setText(QCoreApplication::translate("CuboidParamsDialog", "\351\253\230\345\272\246", nullptr));
    } // retranslateUi

};

namespace Ui {
    class CuboidParamsDialog: public Ui_CuboidParamsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CUBOIDPARAMSDIALOG_H
