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
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
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

class Ui_CuboidParamsDialog
{
public:
    QGridLayout *gridLayout_5;
    QGroupBox *groupBox_11;
    QGridLayout *gridLayout_4;
    QVBoxLayout *verticalLayout_2;
    QHBoxLayout *horizontalLayout_6;
    QLabel *label_11;
    QComboBox *comboBox_2;
    QPushButton *pushButton_2;
    QLabel *label_5;
    QGroupBox *groupBox_8;
    QGridLayout *gridLayout;
    QCheckBox *checkBox_2;
    QLabel *label_4;
    QGridLayout *gridLayout_6;
    QPushButton *okButton;
    QPushButton *cancelButton;
    QPushButton *applyButton;
    QGroupBox *groupBox_2;
    QGridLayout *gridLayout_2;
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
    QLabel *label_6;
    QGroupBox *groupBox_3;
    QGridLayout *gridLayout_3;
    QSpacerItem *horizontalSpacer;
    QPushButton *designated_point;
    QLabel *label_7;
    QPushButton *pushButton_3;
    QPushButton *pushButton_4;
    QPushButton *pushButton;
    QToolButton *originSnapToolButton;
    QToolButton *toolButton;
    QComboBox *comboBox_3;

    void setupUi(QDialog *CuboidParamsDialog)
    {
        if (CuboidParamsDialog->objectName().isEmpty())
            CuboidParamsDialog->setObjectName("CuboidParamsDialog");
        CuboidParamsDialog->resize(350, 400);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/icons/cuboid_color.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        CuboidParamsDialog->setWindowIcon(icon);
        gridLayout_5 = new QGridLayout(CuboidParamsDialog);
        gridLayout_5->setObjectName("gridLayout_5");
        groupBox_11 = new QGroupBox(CuboidParamsDialog);
        groupBox_11->setObjectName("groupBox_11");
        groupBox_11->setFlat(true);
        gridLayout_4 = new QGridLayout(groupBox_11);
        gridLayout_4->setObjectName("gridLayout_4");
        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName("verticalLayout_2");
        horizontalLayout_6 = new QHBoxLayout();
        horizontalLayout_6->setObjectName("horizontalLayout_6");
        label_11 = new QLabel(groupBox_11);
        label_11->setObjectName("label_11");

        horizontalLayout_6->addWidget(label_11);

        comboBox_2 = new QComboBox(groupBox_11);
        comboBox_2->addItem(QString());
        comboBox_2->addItem(QString());
        comboBox_2->addItem(QString());
        comboBox_2->addItem(QString());
        comboBox_2->setObjectName("comboBox_2");

        horizontalLayout_6->addWidget(comboBox_2);


        verticalLayout_2->addLayout(horizontalLayout_6);

        pushButton_2 = new QPushButton(groupBox_11);
        pushButton_2->setObjectName("pushButton_2");
        pushButton_2->setFlat(true);

        verticalLayout_2->addWidget(pushButton_2);


        gridLayout_4->addLayout(verticalLayout_2, 1, 0, 1, 1);

        label_5 = new QLabel(groupBox_11);
        label_5->setObjectName("label_5");
        QFont font;
        font.setPointSize(12);
        label_5->setFont(font);
        label_5->setStyleSheet(QString::fromUtf8("color: rgb(85, 160, 185);"));

        gridLayout_4->addWidget(label_5, 0, 0, 1, 1);


        gridLayout_5->addWidget(groupBox_11, 3, 0, 1, 2);

        groupBox_8 = new QGroupBox(CuboidParamsDialog);
        groupBox_8->setObjectName("groupBox_8");
        groupBox_8->setFlat(true);
        gridLayout = new QGridLayout(groupBox_8);
        gridLayout->setObjectName("gridLayout");
        checkBox_2 = new QCheckBox(groupBox_8);
        checkBox_2->setObjectName("checkBox_2");

        gridLayout->addWidget(checkBox_2, 1, 0, 1, 1);

        label_4 = new QLabel(groupBox_8);
        label_4->setObjectName("label_4");
        label_4->setFont(font);
        label_4->setStyleSheet(QString::fromUtf8("color: rgb(85, 160, 185);"));

        gridLayout->addWidget(label_4, 0, 0, 1, 1);


        gridLayout_5->addWidget(groupBox_8, 4, 0, 1, 2);

        gridLayout_6 = new QGridLayout();
        gridLayout_6->setObjectName("gridLayout_6");
        okButton = new QPushButton(CuboidParamsDialog);
        okButton->setObjectName("okButton");

        gridLayout_6->addWidget(okButton, 0, 0, 1, 1);

        cancelButton = new QPushButton(CuboidParamsDialog);
        cancelButton->setObjectName("cancelButton");

        gridLayout_6->addWidget(cancelButton, 0, 2, 1, 1);

        applyButton = new QPushButton(CuboidParamsDialog);
        applyButton->setObjectName("applyButton");

        gridLayout_6->addWidget(applyButton, 0, 1, 1, 1);


        gridLayout_5->addLayout(gridLayout_6, 5, 1, 1, 1);

        groupBox_2 = new QGroupBox(CuboidParamsDialog);
        groupBox_2->setObjectName("groupBox_2");
        QFont font1;
        font1.setPointSize(9);
        groupBox_2->setFont(font1);
        groupBox_2->setFlat(true);
        gridLayout_2 = new QGridLayout(groupBox_2);
        gridLayout_2->setObjectName("gridLayout_2");
        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName("verticalLayout");
        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName("horizontalLayout_4");
        label = new QLabel(groupBox_2);
        label->setObjectName("label");

        horizontalLayout_4->addWidget(label);

        lengthSpinBox = new QDoubleSpinBox(groupBox_2);
        lengthSpinBox->setObjectName("lengthSpinBox");
        lengthSpinBox->setMinimum(0.100000000000000);
        lengthSpinBox->setSingleStep(0.100000000000000);
        lengthSpinBox->setValue(2.000000000000000);

        horizontalLayout_4->addWidget(lengthSpinBox);


        verticalLayout->addLayout(horizontalLayout_4);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        label_2 = new QLabel(groupBox_2);
        label_2->setObjectName("label_2");

        horizontalLayout_3->addWidget(label_2);

        widthSpinBox = new QDoubleSpinBox(groupBox_2);
        widthSpinBox->setObjectName("widthSpinBox");
        widthSpinBox->setMinimum(0.100000000000000);
        widthSpinBox->setSingleStep(0.100000000000000);
        widthSpinBox->setValue(2.000000000000000);

        horizontalLayout_3->addWidget(widthSpinBox);


        verticalLayout->addLayout(horizontalLayout_3);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        label_3 = new QLabel(groupBox_2);
        label_3->setObjectName("label_3");

        horizontalLayout_2->addWidget(label_3);

        heightSpinBox = new QDoubleSpinBox(groupBox_2);
        heightSpinBox->setObjectName("heightSpinBox");
        heightSpinBox->setMinimum(0.100000000000000);
        heightSpinBox->setSingleStep(0.100000000000000);
        heightSpinBox->setValue(2.000000000000000);

        horizontalLayout_2->addWidget(heightSpinBox);


        verticalLayout->addLayout(horizontalLayout_2);


        gridLayout_2->addLayout(verticalLayout, 1, 0, 1, 1);

        label_6 = new QLabel(groupBox_2);
        label_6->setObjectName("label_6");
        label_6->setFont(font);
        label_6->setStyleSheet(QString::fromUtf8("color: rgb(85, 160, 185);"));

        gridLayout_2->addWidget(label_6, 0, 0, 1, 1);


        gridLayout_5->addWidget(groupBox_2, 2, 0, 1, 2);

        groupBox_3 = new QGroupBox(CuboidParamsDialog);
        groupBox_3->setObjectName("groupBox_3");
        groupBox_3->setFont(font);
        groupBox_3->setFlat(true);
        groupBox_3->setCheckable(false);
        gridLayout_3 = new QGridLayout(groupBox_3);
        gridLayout_3->setObjectName("gridLayout_3");
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout_3->addItem(horizontalSpacer, 2, 1, 1, 1);

        designated_point = new QPushButton(groupBox_3);
        designated_point->setObjectName("designated_point");
        designated_point->setFont(font1);
        designated_point->setFlat(true);

        gridLayout_3->addWidget(designated_point, 3, 0, 1, 1);

        label_7 = new QLabel(groupBox_3);
        label_7->setObjectName("label_7");
        label_7->setStyleSheet(QString::fromUtf8("color: rgb(85, 160, 185);"));

        gridLayout_3->addWidget(label_7, 0, 0, 1, 1);

        pushButton_3 = new QPushButton(groupBox_3);
        pushButton_3->setObjectName("pushButton_3");
        pushButton_3->setFont(font1);

        gridLayout_3->addWidget(pushButton_3, 2, 3, 1, 1);

        pushButton_4 = new QPushButton(groupBox_3);
        pushButton_4->setObjectName("pushButton_4");
        pushButton_4->setFont(font1);

        gridLayout_3->addWidget(pushButton_4, 2, 0, 1, 1);

        pushButton = new QPushButton(groupBox_3);
        pushButton->setObjectName("pushButton");
        pushButton->setFont(font1);

        gridLayout_3->addWidget(pushButton, 2, 2, 1, 1);

        originSnapToolButton = new QToolButton(groupBox_3);
        originSnapToolButton->setObjectName("originSnapToolButton");
        originSnapToolButton->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);

        gridLayout_3->addWidget(originSnapToolButton, 3, 4, 1, 1);

        toolButton = new QToolButton(groupBox_3);
        toolButton->setObjectName("toolButton");

        gridLayout_3->addWidget(toolButton, 2, 4, 1, 1);


        gridLayout_5->addWidget(groupBox_3, 1, 0, 1, 2);

        comboBox_3 = new QComboBox(CuboidParamsDialog);
        comboBox_3->addItem(QString());
        comboBox_3->addItem(QString());
        comboBox_3->addItem(QString());
        comboBox_3->setObjectName("comboBox_3");

        gridLayout_5->addWidget(comboBox_3, 0, 0, 1, 2);


        retranslateUi(CuboidParamsDialog);

        QMetaObject::connectSlotsByName(CuboidParamsDialog);
    } // setupUi

    void retranslateUi(QDialog *CuboidParamsDialog)
    {
        CuboidParamsDialog->setWindowTitle(QCoreApplication::translate("CuboidParamsDialog", "\345\235\227", nullptr));
        groupBox_11->setTitle(QString());
        label_11->setText(QCoreApplication::translate("CuboidParamsDialog", "\345\270\203\345\260\224", nullptr));
        comboBox_2->setItemText(0, QCoreApplication::translate("CuboidParamsDialog", "\346\227\240", nullptr));
        comboBox_2->setItemText(1, QCoreApplication::translate("CuboidParamsDialog", "\345\220\210\345\271\266", nullptr));
        comboBox_2->setItemText(2, QCoreApplication::translate("CuboidParamsDialog", "\345\207\217\345\216\273", nullptr));
        comboBox_2->setItemText(3, QCoreApplication::translate("CuboidParamsDialog", "\347\233\270\344\272\244", nullptr));

        pushButton_2->setText(QCoreApplication::translate("CuboidParamsDialog", "\351\200\211\346\213\251\344\275\223", nullptr));
        label_5->setText(QCoreApplication::translate("CuboidParamsDialog", "\345\270\203\345\260\224", nullptr));
        groupBox_8->setTitle(QString());
        checkBox_2->setText(QCoreApplication::translate("CuboidParamsDialog", "\345\205\263\350\201\224\345\216\237\347\202\271", nullptr));
        label_4->setText(QCoreApplication::translate("CuboidParamsDialog", "\350\256\276\347\275\256", nullptr));
        okButton->setText(QCoreApplication::translate("CuboidParamsDialog", "\347\241\256\350\256\244", nullptr));
        cancelButton->setText(QCoreApplication::translate("CuboidParamsDialog", "\345\217\226\346\266\210", nullptr));
        applyButton->setText(QCoreApplication::translate("CuboidParamsDialog", "\345\272\224\347\224\250", nullptr));
        groupBox_2->setTitle(QString());
        label->setText(QCoreApplication::translate("CuboidParamsDialog", "\351\225\277\345\272\246\357\274\210XC\357\274\211", nullptr));
        lengthSpinBox->setSuffix(QCoreApplication::translate("CuboidParamsDialog", "mm", nullptr));
        label_2->setText(QCoreApplication::translate("CuboidParamsDialog", "\345\256\275\345\272\246\357\274\210YC\357\274\211", nullptr));
        widthSpinBox->setSuffix(QCoreApplication::translate("CuboidParamsDialog", "mm", nullptr));
        label_3->setText(QCoreApplication::translate("CuboidParamsDialog", "\351\253\230\345\272\246\357\274\210ZC\357\274\211", nullptr));
        heightSpinBox->setSuffix(QCoreApplication::translate("CuboidParamsDialog", "mm", nullptr));
        label_6->setText(QCoreApplication::translate("CuboidParamsDialog", "\345\260\272\345\257\270", nullptr));
        groupBox_3->setTitle(QString());
        designated_point->setText(QCoreApplication::translate("CuboidParamsDialog", "\346\214\207\345\256\232\347\202\271", nullptr));
        label_7->setText(QCoreApplication::translate("CuboidParamsDialog", "\345\216\237\347\202\271", nullptr));
        pushButton_3->setText(QCoreApplication::translate("CuboidParamsDialog", "\347\237\242\351\207\217\345\257\271\350\257\235\346\241\206", nullptr));
        pushButton_4->setText(QCoreApplication::translate("CuboidParamsDialog", "\346\214\207\345\256\232\347\237\242\351\207\217", nullptr));
        pushButton->setText(QCoreApplication::translate("CuboidParamsDialog", "\345\217\215\345\220\221", nullptr));
        originSnapToolButton->setText(QCoreApplication::translate("CuboidParamsDialog", "...", nullptr));
        toolButton->setText(QCoreApplication::translate("CuboidParamsDialog", "...", nullptr));
        comboBox_3->setItemText(0, QCoreApplication::translate("CuboidParamsDialog", "\345\216\237\347\202\271\345\222\214\350\276\271\351\225\277", nullptr));
        comboBox_3->setItemText(1, QCoreApplication::translate("CuboidParamsDialog", "\344\270\244\347\202\271\345\222\214\351\253\230\345\272\246", nullptr));
        comboBox_3->setItemText(2, QCoreApplication::translate("CuboidParamsDialog", "\344\270\244\344\270\252\345\257\271\350\247\222\347\202\271", nullptr));

    } // retranslateUi

};

namespace Ui {
    class CuboidParamsDialog: public Ui_CuboidParamsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CUBOIDPARAMSDIALOG_H
