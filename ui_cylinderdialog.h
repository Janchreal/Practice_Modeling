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
#include <QtGui/QIcon>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_CylinderDialog
{
public:
    QGridLayout *gridLayout_2;
    QGroupBox *groupBox_2;
    QGridLayout *gridLayout;
    QSpacerItem *horizontalSpacer;
    QLabel *label_2;
    QToolButton *toolButton;
    QToolButton *originSnapToolButton;
    QPushButton *pushButton;
    QPushButton *pushButton_2;
    QPushButton *designated_point;
    QPushButton *pushButton_4;
    QGroupBox *groupBox_3;
    QGridLayout *gridLayout_3;
    QVBoxLayout *verticalLayout_2;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label;
    QComboBox *comboBox_2;
    QPushButton *pushButton_3;
    QLabel *label_3;
    QGroupBox *groupBox;
    QVBoxLayout *verticalLayout;
    QLabel *label_4;
    QHBoxLayout *horizontalLayout;
    QLabel *heigh;
    QDoubleSpinBox *heightSpinBox;
    QHBoxLayout *horizontalLayout_2;
    QLabel *radius;
    QDoubleSpinBox *radiusSpinBox;
    QHBoxLayout *horizontalLayout_4;
    QDialogButtonBox *buttonBox;
    QPushButton *applyButton;

    void setupUi(QDialog *CylinderDialog)
    {
        if (CylinderDialog->objectName().isEmpty())
            CylinderDialog->setObjectName("CylinderDialog");
        CylinderDialog->resize(350, 400);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/icons/cylinder_color.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        CylinderDialog->setWindowIcon(icon);
        gridLayout_2 = new QGridLayout(CylinderDialog);
        gridLayout_2->setObjectName("gridLayout_2");
        groupBox_2 = new QGroupBox(CylinderDialog);
        groupBox_2->setObjectName("groupBox_2");
        groupBox_2->setFlat(true);
        gridLayout = new QGridLayout(groupBox_2);
        gridLayout->setObjectName("gridLayout");
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout->addItem(horizontalSpacer, 1, 1, 1, 1);

        label_2 = new QLabel(groupBox_2);
        label_2->setObjectName("label_2");
        QFont font;
        font.setPointSize(12);
        label_2->setFont(font);
        label_2->setStyleSheet(QString::fromUtf8("color: rgb(85, 160, 185);"));

        gridLayout->addWidget(label_2, 0, 0, 1, 1);

        toolButton = new QToolButton(groupBox_2);
        toolButton->setObjectName("toolButton");

        gridLayout->addWidget(toolButton, 1, 4, 1, 1);

        originSnapToolButton = new QToolButton(groupBox_2);
        originSnapToolButton->setObjectName("originSnapToolButton");
        originSnapToolButton->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);

        gridLayout->addWidget(originSnapToolButton, 2, 4, 1, 1);

        pushButton = new QPushButton(groupBox_2);
        pushButton->setObjectName("pushButton");
        pushButton->setFlat(true);

        gridLayout->addWidget(pushButton, 1, 0, 1, 1);

        pushButton_2 = new QPushButton(groupBox_2);
        pushButton_2->setObjectName("pushButton_2");

        gridLayout->addWidget(pushButton_2, 1, 2, 1, 1);

        designated_point = new QPushButton(groupBox_2);
        designated_point->setObjectName("designated_point");
        designated_point->setFlat(true);

        gridLayout->addWidget(designated_point, 2, 0, 1, 1);

        pushButton_4 = new QPushButton(groupBox_2);
        pushButton_4->setObjectName("pushButton_4");

        gridLayout->addWidget(pushButton_4, 1, 3, 1, 1);


        gridLayout_2->addWidget(groupBox_2, 0, 0, 1, 1);

        groupBox_3 = new QGroupBox(CylinderDialog);
        groupBox_3->setObjectName("groupBox_3");
        groupBox_3->setFlat(true);
        gridLayout_3 = new QGridLayout(groupBox_3);
        gridLayout_3->setObjectName("gridLayout_3");
        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName("verticalLayout_2");
        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        label = new QLabel(groupBox_3);
        label->setObjectName("label");

        horizontalLayout_3->addWidget(label);

        comboBox_2 = new QComboBox(groupBox_3);
        comboBox_2->addItem(QString());
        comboBox_2->addItem(QString());
        comboBox_2->addItem(QString());
        comboBox_2->addItem(QString());
        comboBox_2->setObjectName("comboBox_2");

        horizontalLayout_3->addWidget(comboBox_2);


        verticalLayout_2->addLayout(horizontalLayout_3);

        pushButton_3 = new QPushButton(groupBox_3);
        pushButton_3->setObjectName("pushButton_3");
        pushButton_3->setFlat(true);

        verticalLayout_2->addWidget(pushButton_3);


        gridLayout_3->addLayout(verticalLayout_2, 1, 0, 1, 1);

        label_3 = new QLabel(groupBox_3);
        label_3->setObjectName("label_3");
        label_3->setFont(font);
        label_3->setStyleSheet(QString::fromUtf8("color: rgb(85, 160, 185);"));

        gridLayout_3->addWidget(label_3, 0, 0, 1, 1);


        gridLayout_2->addWidget(groupBox_3, 2, 0, 1, 1);

        groupBox = new QGroupBox(CylinderDialog);
        groupBox->setObjectName("groupBox");
        groupBox->setFlat(true);
        verticalLayout = new QVBoxLayout(groupBox);
        verticalLayout->setObjectName("verticalLayout");
        label_4 = new QLabel(groupBox);
        label_4->setObjectName("label_4");
        label_4->setFont(font);
        label_4->setStyleSheet(QString::fromUtf8("color: rgb(85, 160, 185);"));

        verticalLayout->addWidget(label_4);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        heigh = new QLabel(groupBox);
        heigh->setObjectName("heigh");

        horizontalLayout->addWidget(heigh);

        heightSpinBox = new QDoubleSpinBox(groupBox);
        heightSpinBox->setObjectName("heightSpinBox");
        heightSpinBox->setFrame(true);
        heightSpinBox->setMinimum(0.100000000000000);
        heightSpinBox->setSingleStep(0.100000000000000);
        heightSpinBox->setStepType(QAbstractSpinBox::StepType::DefaultStepType);
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


        gridLayout_2->addWidget(groupBox, 1, 0, 1, 1);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName("horizontalLayout_4");
        buttonBox = new QDialogButtonBox(CylinderDialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setStandardButtons(QDialogButtonBox::StandardButton::Cancel|QDialogButtonBox::StandardButton::Ok);

        horizontalLayout_4->addWidget(buttonBox);

        applyButton = new QPushButton(CylinderDialog);
        applyButton->setObjectName("applyButton");

        horizontalLayout_4->addWidget(applyButton);


        gridLayout_2->addLayout(horizontalLayout_4, 3, 0, 1, 1);


        retranslateUi(CylinderDialog);

        QMetaObject::connectSlotsByName(CylinderDialog);
    } // setupUi

    void retranslateUi(QDialog *CylinderDialog)
    {
        CylinderDialog->setWindowTitle(QCoreApplication::translate("CylinderDialog", "\345\234\206\346\237\261\344\275\223", nullptr));
        groupBox_2->setTitle(QString());
        label_2->setText(QCoreApplication::translate("CylinderDialog", "\350\275\264", nullptr));
        toolButton->setText(QCoreApplication::translate("CylinderDialog", "...", nullptr));
        originSnapToolButton->setText(QCoreApplication::translate("CylinderDialog", "...", nullptr));
        pushButton->setText(QCoreApplication::translate("CylinderDialog", "\346\214\207\345\256\232\347\237\242\351\207\217", nullptr));
        pushButton_2->setText(QCoreApplication::translate("CylinderDialog", "\345\217\215\345\220\221", nullptr));
        designated_point->setText(QCoreApplication::translate("CylinderDialog", "\346\214\207\345\256\232\347\202\271", nullptr));
        pushButton_4->setText(QCoreApplication::translate("CylinderDialog", "\347\237\242\351\207\217\345\257\271\350\257\235\346\241\206", nullptr));
        groupBox_3->setTitle(QString());
        label->setText(QCoreApplication::translate("CylinderDialog", "\345\270\203\345\260\224", nullptr));
        comboBox_2->setItemText(0, QCoreApplication::translate("CylinderDialog", "\346\227\240", nullptr));
        comboBox_2->setItemText(1, QCoreApplication::translate("CylinderDialog", "\345\207\217\345\216\273", nullptr));
        comboBox_2->setItemText(2, QCoreApplication::translate("CylinderDialog", "\345\220\210\345\271\266", nullptr));
        comboBox_2->setItemText(3, QCoreApplication::translate("CylinderDialog", "\347\233\270\344\272\244", nullptr));

        pushButton_3->setText(QCoreApplication::translate("CylinderDialog", "\351\200\211\346\213\251\344\275\223", nullptr));
        label_3->setText(QCoreApplication::translate("CylinderDialog", "\345\270\203\345\260\224", nullptr));
        groupBox->setTitle(QString());
        label_4->setText(QCoreApplication::translate("CylinderDialog", "\345\260\272\345\257\270", nullptr));
        heigh->setText(QCoreApplication::translate("CylinderDialog", "\351\253\230", nullptr));
        heightSpinBox->setSuffix(QCoreApplication::translate("CylinderDialog", "mm", nullptr));
        radius->setText(QCoreApplication::translate("CylinderDialog", "\345\215\212\345\276\204", nullptr));
        radiusSpinBox->setSuffix(QCoreApplication::translate("CylinderDialog", "mm", nullptr));
        applyButton->setText(QCoreApplication::translate("CylinderDialog", "\345\272\224\347\224\250", nullptr));
    } // retranslateUi

};

namespace Ui {
    class CylinderDialog: public Ui_CylinderDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CYLINDERDIALOG_H
