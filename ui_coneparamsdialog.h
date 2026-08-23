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

class Ui_ConeParamsDialog
{
public:
    QGridLayout *gridLayout_4;
    QGroupBox *groupBox_2;
    QGridLayout *gridLayout_2;
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
    QLabel *label_3;
    QHBoxLayout *horizontalLayout_5;
    QDialogButtonBox *okandcancel;
    QPushButton *applyButton;
    QGroupBox *groupBox_3;
    QGridLayout *gridLayout_3;
    QVBoxLayout *verticalLayout_2;
    QHBoxLayout *horizontalLayout_4;
    QLabel *label;
    QComboBox *comboBox;
    QPushButton *pushButton_3;
    QLabel *label_4;
    QGroupBox *groupBox;
    QGridLayout *gridLayout;
    QPushButton *designated_point;
    QPushButton *pushButton_2;
    QToolButton *toolButton;
    QToolButton *originSnapToolButton;
    QLabel *label_2;
    QPushButton *pushButton;
    QSpacerItem *horizontalSpacer;
    QPushButton *pushButton_4;

    void setupUi(QDialog *ConeParamsDialog)
    {
        if (ConeParamsDialog->objectName().isEmpty())
            ConeParamsDialog->setObjectName("ConeParamsDialog");
        ConeParamsDialog->resize(350, 400);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/icons/cone_color.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        ConeParamsDialog->setWindowIcon(icon);
        gridLayout_4 = new QGridLayout(ConeParamsDialog);
        gridLayout_4->setObjectName("gridLayout_4");
        groupBox_2 = new QGroupBox(ConeParamsDialog);
        groupBox_2->setObjectName("groupBox_2");
        groupBox_2->setFlat(true);
        gridLayout_2 = new QGridLayout(groupBox_2);
        gridLayout_2->setObjectName("gridLayout_2");
        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName("verticalLayout");
        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        radius1 = new QLabel(groupBox_2);
        radius1->setObjectName("radius1");

        horizontalLayout_3->addWidget(radius1);

        Radius1SpinBox = new QDoubleSpinBox(groupBox_2);
        Radius1SpinBox->setObjectName("Radius1SpinBox");
        Radius1SpinBox->setMinimum(0.100000000000000);
        Radius1SpinBox->setValue(2.000000000000000);

        horizontalLayout_3->addWidget(Radius1SpinBox);


        verticalLayout->addLayout(horizontalLayout_3);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        Radius2 = new QLabel(groupBox_2);
        Radius2->setObjectName("Radius2");

        horizontalLayout->addWidget(Radius2);

        Radius2SpinBox = new QDoubleSpinBox(groupBox_2);
        Radius2SpinBox->setObjectName("Radius2SpinBox");

        horizontalLayout->addWidget(Radius2SpinBox);


        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        Height = new QLabel(groupBox_2);
        Height->setObjectName("Height");

        horizontalLayout_2->addWidget(Height);

        HeightSpinBox = new QDoubleSpinBox(groupBox_2);
        HeightSpinBox->setObjectName("HeightSpinBox");
        HeightSpinBox->setMinimum(0.100000000000000);
        HeightSpinBox->setValue(4.000000000000000);

        horizontalLayout_2->addWidget(HeightSpinBox);


        verticalLayout->addLayout(horizontalLayout_2);


        gridLayout_2->addLayout(verticalLayout, 1, 0, 1, 1);

        label_3 = new QLabel(groupBox_2);
        label_3->setObjectName("label_3");
        QFont font;
        font.setPointSize(12);
        label_3->setFont(font);
        label_3->setStyleSheet(QString::fromUtf8("color: rgb(85, 160, 185);"));

        gridLayout_2->addWidget(label_3, 0, 0, 1, 1);


        gridLayout_4->addWidget(groupBox_2, 1, 0, 1, 1);

        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setObjectName("horizontalLayout_5");
        okandcancel = new QDialogButtonBox(ConeParamsDialog);
        okandcancel->setObjectName("okandcancel");
        okandcancel->setStandardButtons(QDialogButtonBox::StandardButton::Cancel|QDialogButtonBox::StandardButton::Ok);

        horizontalLayout_5->addWidget(okandcancel);

        applyButton = new QPushButton(ConeParamsDialog);
        applyButton->setObjectName("applyButton");

        horizontalLayout_5->addWidget(applyButton);


        gridLayout_4->addLayout(horizontalLayout_5, 3, 0, 1, 1);

        groupBox_3 = new QGroupBox(ConeParamsDialog);
        groupBox_3->setObjectName("groupBox_3");
        groupBox_3->setFlat(true);
        gridLayout_3 = new QGridLayout(groupBox_3);
        gridLayout_3->setObjectName("gridLayout_3");
        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName("verticalLayout_2");
        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName("horizontalLayout_4");
        label = new QLabel(groupBox_3);
        label->setObjectName("label");

        horizontalLayout_4->addWidget(label);

        comboBox = new QComboBox(groupBox_3);
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->setObjectName("comboBox");

        horizontalLayout_4->addWidget(comboBox);


        verticalLayout_2->addLayout(horizontalLayout_4);

        pushButton_3 = new QPushButton(groupBox_3);
        pushButton_3->setObjectName("pushButton_3");
        pushButton_3->setFlat(true);

        verticalLayout_2->addWidget(pushButton_3);


        gridLayout_3->addLayout(verticalLayout_2, 1, 0, 1, 1);

        label_4 = new QLabel(groupBox_3);
        label_4->setObjectName("label_4");
        label_4->setFont(font);
        label_4->setStyleSheet(QString::fromUtf8("color: rgb(85, 160, 185);"));

        gridLayout_3->addWidget(label_4, 0, 0, 1, 1);


        gridLayout_4->addWidget(groupBox_3, 2, 0, 1, 1);

        groupBox = new QGroupBox(ConeParamsDialog);
        groupBox->setObjectName("groupBox");
        groupBox->setFlat(true);
        gridLayout = new QGridLayout(groupBox);
        gridLayout->setObjectName("gridLayout");
        designated_point = new QPushButton(groupBox);
        designated_point->setObjectName("designated_point");
        designated_point->setFlat(true);

        gridLayout->addWidget(designated_point, 2, 0, 1, 1);

        pushButton_2 = new QPushButton(groupBox);
        pushButton_2->setObjectName("pushButton_2");

        gridLayout->addWidget(pushButton_2, 1, 2, 1, 1);

        toolButton = new QToolButton(groupBox);
        toolButton->setObjectName("toolButton");

        gridLayout->addWidget(toolButton, 1, 4, 1, 1);

        originSnapToolButton = new QToolButton(groupBox);
        originSnapToolButton->setObjectName("originSnapToolButton");
        originSnapToolButton->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);

        gridLayout->addWidget(originSnapToolButton, 2, 4, 1, 1);

        label_2 = new QLabel(groupBox);
        label_2->setObjectName("label_2");
        QFont font1;
        font1.setPointSize(12);
        font1.setBold(false);
        label_2->setFont(font1);
        label_2->setStyleSheet(QString::fromUtf8("color: rgb(85, 160, 185);"));

        gridLayout->addWidget(label_2, 0, 0, 1, 1);

        pushButton = new QPushButton(groupBox);
        pushButton->setObjectName("pushButton");
        pushButton->setFlat(true);

        gridLayout->addWidget(pushButton, 1, 0, 1, 1);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout->addItem(horizontalSpacer, 1, 1, 1, 1);

        pushButton_4 = new QPushButton(groupBox);
        pushButton_4->setObjectName("pushButton_4");

        gridLayout->addWidget(pushButton_4, 1, 3, 1, 1);


        gridLayout_4->addWidget(groupBox, 0, 0, 1, 1);


        retranslateUi(ConeParamsDialog);

        QMetaObject::connectSlotsByName(ConeParamsDialog);
    } // setupUi

    void retranslateUi(QDialog *ConeParamsDialog)
    {
        ConeParamsDialog->setWindowTitle(QCoreApplication::translate("ConeParamsDialog", "\345\234\206\351\224\245\344\275\223", nullptr));
        groupBox_2->setTitle(QString());
        radius1->setText(QCoreApplication::translate("ConeParamsDialog", "\345\272\225\351\203\250\345\215\212\345\276\204", nullptr));
        Radius1SpinBox->setSuffix(QCoreApplication::translate("ConeParamsDialog", "mm", nullptr));
        Radius2->setText(QCoreApplication::translate("ConeParamsDialog", "\351\241\266\351\203\250\345\215\212\345\276\204", nullptr));
        Radius2SpinBox->setSuffix(QCoreApplication::translate("ConeParamsDialog", "mm", nullptr));
        Height->setText(QCoreApplication::translate("ConeParamsDialog", "\351\253\230", nullptr));
        HeightSpinBox->setSuffix(QCoreApplication::translate("ConeParamsDialog", "mm", nullptr));
        label_3->setText(QCoreApplication::translate("ConeParamsDialog", "\345\260\272\345\257\270", nullptr));
        applyButton->setText(QCoreApplication::translate("ConeParamsDialog", "\345\272\224\347\224\250", nullptr));
        groupBox_3->setTitle(QString());
        label->setText(QCoreApplication::translate("ConeParamsDialog", "\345\270\203\345\260\224", nullptr));
        comboBox->setItemText(0, QCoreApplication::translate("ConeParamsDialog", "\346\227\240", nullptr));
        comboBox->setItemText(1, QCoreApplication::translate("ConeParamsDialog", "\345\220\210\345\271\266", nullptr));
        comboBox->setItemText(2, QCoreApplication::translate("ConeParamsDialog", "\345\207\217\345\216\273", nullptr));
        comboBox->setItemText(3, QCoreApplication::translate("ConeParamsDialog", "\347\233\270\344\272\244", nullptr));

        pushButton_3->setText(QCoreApplication::translate("ConeParamsDialog", "\351\200\211\346\213\251\344\275\223", nullptr));
        label_4->setText(QCoreApplication::translate("ConeParamsDialog", "\345\270\203\345\260\224", nullptr));
        groupBox->setTitle(QString());
        designated_point->setText(QCoreApplication::translate("ConeParamsDialog", "\346\214\207\345\256\232\347\202\271", nullptr));
        pushButton_2->setText(QCoreApplication::translate("ConeParamsDialog", "\345\217\215\345\220\221", nullptr));
        toolButton->setText(QCoreApplication::translate("ConeParamsDialog", "...", nullptr));
        originSnapToolButton->setText(QCoreApplication::translate("ConeParamsDialog", "...", nullptr));
        label_2->setText(QCoreApplication::translate("ConeParamsDialog", "\350\275\264", nullptr));
        pushButton->setText(QCoreApplication::translate("ConeParamsDialog", "\346\214\207\345\256\232\347\237\242\351\207\217", nullptr));
        pushButton_4->setText(QCoreApplication::translate("ConeParamsDialog", "\347\237\242\351\207\217\345\257\271\350\257\235\346\241\206", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ConeParamsDialog: public Ui_ConeParamsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CONEPARAMSDIALOG_H
