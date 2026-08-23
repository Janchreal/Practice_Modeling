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
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
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

class Ui_SphereParamsDialog
{
public:
    QGridLayout *gridLayout_5;
    QComboBox *comboBox;
    QGroupBox *groupBox_2;
    QGridLayout *gridLayout_2;
    QLabel *label;
    QToolButton *originSnapToolButton;
    QPushButton *designated_point;
    QGroupBox *groupBox;
    QGridLayout *gridLayout;
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
    QLabel *label_2;
    QGroupBox *groupBox_3;
    QGridLayout *gridLayout_4;
    QHBoxLayout *horizontalLayout_5;
    QLabel *label_3;
    QComboBox *comboBox_2;
    QPushButton *pushButton_2;
    QLabel *label_4;
    QGridLayout *gridLayout_3;
    QPushButton *cancleButton;
    QPushButton *okButton;
    QPushButton *applyButton;
    QSpacerItem *horizontalSpacer;

    void setupUi(QDialog *SphereParamsDialog)
    {
        if (SphereParamsDialog->objectName().isEmpty())
            SphereParamsDialog->setObjectName("SphereParamsDialog");
        SphereParamsDialog->resize(350, 400);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/icons/sphere_color.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        SphereParamsDialog->setWindowIcon(icon);
        gridLayout_5 = new QGridLayout(SphereParamsDialog);
        gridLayout_5->setObjectName("gridLayout_5");
        comboBox = new QComboBox(SphereParamsDialog);
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->setObjectName("comboBox");

        gridLayout_5->addWidget(comboBox, 0, 0, 1, 1);

        groupBox_2 = new QGroupBox(SphereParamsDialog);
        groupBox_2->setObjectName("groupBox_2");
        groupBox_2->setFlat(true);
        gridLayout_2 = new QGridLayout(groupBox_2);
        gridLayout_2->setObjectName("gridLayout_2");
        label = new QLabel(groupBox_2);
        label->setObjectName("label");
        QFont font;
        font.setPointSize(12);
        label->setFont(font);
        label->setStyleSheet(QString::fromUtf8("color: rgb(85, 160, 185);"));

        gridLayout_2->addWidget(label, 0, 0, 1, 1);

        originSnapToolButton = new QToolButton(groupBox_2);
        originSnapToolButton->setObjectName("originSnapToolButton");
        originSnapToolButton->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);

        gridLayout_2->addWidget(originSnapToolButton, 1, 1, 1, 1);

        designated_point = new QPushButton(groupBox_2);
        designated_point->setObjectName("designated_point");
        designated_point->setFlat(true);

        gridLayout_2->addWidget(designated_point, 1, 0, 1, 1);


        gridLayout_5->addWidget(groupBox_2, 1, 0, 1, 1);

        groupBox = new QGroupBox(SphereParamsDialog);
        groupBox->setObjectName("groupBox");
        groupBox->setFlat(true);
        gridLayout = new QGridLayout(groupBox);
        gridLayout->setObjectName("gridLayout");
        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName("verticalLayout");
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        Radius = new QLabel(groupBox);
        Radius->setObjectName("Radius");

        horizontalLayout->addWidget(Radius);

        radiusSpinBox = new QDoubleSpinBox(groupBox);
        radiusSpinBox->setObjectName("radiusSpinBox");
        radiusSpinBox->setMinimum(0.100000000000000);
        radiusSpinBox->setSingleStep(0.100000000000000);
        radiusSpinBox->setValue(2.000000000000000);

        horizontalLayout->addWidget(radiusSpinBox);


        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName("horizontalLayout_4");
        thetaResolution = new QLabel(groupBox);
        thetaResolution->setObjectName("thetaResolution");

        horizontalLayout_4->addWidget(thetaResolution);

        thetaResolutionSpinBox = new QDoubleSpinBox(groupBox);
        thetaResolutionSpinBox->setObjectName("thetaResolutionSpinBox");
        thetaResolutionSpinBox->setMinimum(0.100000000000000);
        thetaResolutionSpinBox->setSingleStep(0.100000000000000);
        thetaResolutionSpinBox->setValue(0.100000000000000);

        horizontalLayout_4->addWidget(thetaResolutionSpinBox);


        verticalLayout->addLayout(horizontalLayout_4);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        phiResolution = new QLabel(groupBox);
        phiResolution->setObjectName("phiResolution");

        horizontalLayout_2->addWidget(phiResolution);

        phiResolutionSpinBox = new QDoubleSpinBox(groupBox);
        phiResolutionSpinBox->setObjectName("phiResolutionSpinBox");
        phiResolutionSpinBox->setMinimum(0.100000000000000);
        phiResolutionSpinBox->setSingleStep(0.100000000000000);
        phiResolutionSpinBox->setValue(0.100000000000000);

        horizontalLayout_2->addWidget(phiResolutionSpinBox);


        verticalLayout->addLayout(horizontalLayout_2);


        gridLayout->addLayout(verticalLayout, 1, 0, 1, 1);

        label_2 = new QLabel(groupBox);
        label_2->setObjectName("label_2");
        label_2->setFont(font);
        label_2->setStyleSheet(QString::fromUtf8("color: rgb(85, 160, 185);"));

        gridLayout->addWidget(label_2, 0, 0, 1, 1);


        gridLayout_5->addWidget(groupBox, 2, 0, 1, 1);

        groupBox_3 = new QGroupBox(SphereParamsDialog);
        groupBox_3->setObjectName("groupBox_3");
        groupBox_3->setFlat(true);
        gridLayout_4 = new QGridLayout(groupBox_3);
        gridLayout_4->setObjectName("gridLayout_4");
        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setObjectName("horizontalLayout_5");
        label_3 = new QLabel(groupBox_3);
        label_3->setObjectName("label_3");

        horizontalLayout_5->addWidget(label_3);

        comboBox_2 = new QComboBox(groupBox_3);
        comboBox_2->addItem(QString());
        comboBox_2->addItem(QString());
        comboBox_2->addItem(QString());
        comboBox_2->addItem(QString());
        comboBox_2->setObjectName("comboBox_2");

        horizontalLayout_5->addWidget(comboBox_2);


        gridLayout_4->addLayout(horizontalLayout_5, 1, 0, 1, 1);

        pushButton_2 = new QPushButton(groupBox_3);
        pushButton_2->setObjectName("pushButton_2");
        pushButton_2->setFlat(true);

        gridLayout_4->addWidget(pushButton_2, 2, 0, 1, 1);

        label_4 = new QLabel(groupBox_3);
        label_4->setObjectName("label_4");
        label_4->setFont(font);
        label_4->setStyleSheet(QString::fromUtf8("color: rgb(85, 160, 185);"));

        gridLayout_4->addWidget(label_4, 0, 0, 1, 1);


        gridLayout_5->addWidget(groupBox_3, 3, 0, 1, 1);

        gridLayout_3 = new QGridLayout();
        gridLayout_3->setObjectName("gridLayout_3");
        cancleButton = new QPushButton(SphereParamsDialog);
        cancleButton->setObjectName("cancleButton");

        gridLayout_3->addWidget(cancleButton, 0, 3, 1, 1);

        okButton = new QPushButton(SphereParamsDialog);
        okButton->setObjectName("okButton");

        gridLayout_3->addWidget(okButton, 0, 1, 1, 1);

        applyButton = new QPushButton(SphereParamsDialog);
        applyButton->setObjectName("applyButton");

        gridLayout_3->addWidget(applyButton, 0, 2, 1, 1);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout_3->addItem(horizontalSpacer, 0, 0, 1, 1);


        gridLayout_5->addLayout(gridLayout_3, 4, 0, 1, 1);


        retranslateUi(SphereParamsDialog);

        QMetaObject::connectSlotsByName(SphereParamsDialog);
    } // setupUi

    void retranslateUi(QDialog *SphereParamsDialog)
    {
        SphereParamsDialog->setWindowTitle(QCoreApplication::translate("SphereParamsDialog", "\347\220\203\344\275\223", nullptr));
        comboBox->setItemText(0, QCoreApplication::translate("SphereParamsDialog", "\344\270\255\345\277\203\347\202\271\345\222\214\347\233\264\345\276\204", nullptr));
        comboBox->setItemText(1, QCoreApplication::translate("SphereParamsDialog", "\345\234\206\345\274\247", nullptr));

        groupBox_2->setTitle(QString());
        label->setText(QCoreApplication::translate("SphereParamsDialog", "\344\270\255\345\277\203\347\202\271", nullptr));
        originSnapToolButton->setText(QCoreApplication::translate("SphereParamsDialog", "...", nullptr));
        designated_point->setText(QCoreApplication::translate("SphereParamsDialog", "\346\214\207\345\256\232\347\202\271", nullptr));
        groupBox->setTitle(QString());
        Radius->setText(QCoreApplication::translate("SphereParamsDialog", "\345\215\212\345\276\204", nullptr));
        radiusSpinBox->setSuffix(QCoreApplication::translate("SphereParamsDialog", "mm", nullptr));
        thetaResolution->setText(QCoreApplication::translate("SphereParamsDialog", "\347\273\217\345\272\246\345\210\206\350\276\250\347\216\207", nullptr));
        phiResolution->setText(QCoreApplication::translate("SphereParamsDialog", "\347\272\254\345\272\246\345\210\206\350\276\250\347\216\207", nullptr));
        label_2->setText(QCoreApplication::translate("SphereParamsDialog", "\345\260\272\345\257\270", nullptr));
        groupBox_3->setTitle(QString());
        label_3->setText(QCoreApplication::translate("SphereParamsDialog", "\345\270\203\345\260\224", nullptr));
        comboBox_2->setItemText(0, QCoreApplication::translate("SphereParamsDialog", "\346\227\240", nullptr));
        comboBox_2->setItemText(1, QCoreApplication::translate("SphereParamsDialog", "\345\207\217\345\216\273", nullptr));
        comboBox_2->setItemText(2, QCoreApplication::translate("SphereParamsDialog", "\345\220\210\345\271\266", nullptr));
        comboBox_2->setItemText(3, QCoreApplication::translate("SphereParamsDialog", "\347\233\270\344\272\244", nullptr));

        pushButton_2->setText(QCoreApplication::translate("SphereParamsDialog", "\351\200\211\346\213\251\344\275\223", nullptr));
        label_4->setText(QCoreApplication::translate("SphereParamsDialog", "\345\270\203\345\260\224", nullptr));
        cancleButton->setText(QCoreApplication::translate("SphereParamsDialog", "\345\217\226\346\266\210", nullptr));
        okButton->setText(QCoreApplication::translate("SphereParamsDialog", "\347\241\256\350\256\244", nullptr));
        applyButton->setText(QCoreApplication::translate("SphereParamsDialog", "\345\272\224\347\224\250", nullptr));
    } // retranslateUi

};

namespace Ui {
    class SphereParamsDialog: public Ui_SphereParamsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SPHEREPARAMSDIALOG_H
