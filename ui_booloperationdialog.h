/********************************************************************************
** Form generated from reading UI file 'booloperationdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_BOOLOPERATIONDIALOG_H
#define UI_BOOLOPERATIONDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_booloperationdialog
{
public:
    QVBoxLayout *verticalLayout;
    QGroupBox *groupBox;
    QGridLayout *gridLayout_2;
    QHBoxLayout *horizontalLayout;
    QLabel *targetLabel;
    QLabel *targetNameLabel;
    QPushButton *targetSelectButton;
    QHBoxLayout *horizontalLayout_2;
    QLabel *toolLabel;
    QLabel *toolNameLabel;
    QPushButton *toolSelectButton;
    QHBoxLayout *horizontalLayout_3;
    QLabel *operationLabel;
    QComboBox *operationComboBox;
    QGroupBox *settingsGroupBox;
    QVBoxLayout *settingsLayout;
    QCheckBox *keepTargetCheckBox;
    QCheckBox *keepToolCheckBox;
    QHBoxLayout *horizontalLayout_4;
    QSpacerItem *horizontalSpacer;
    QPushButton *okButton;
    QPushButton *cancelButton;

    void setupUi(QDialog *booloperationdialog)
    {
        if (booloperationdialog->objectName().isEmpty())
            booloperationdialog->setObjectName("booloperationdialog");
        booloperationdialog->resize(450, 380);
        verticalLayout = new QVBoxLayout(booloperationdialog);
        verticalLayout->setObjectName("verticalLayout");
        groupBox = new QGroupBox(booloperationdialog);
        groupBox->setObjectName("groupBox");
        gridLayout_2 = new QGridLayout(groupBox);
        gridLayout_2->setObjectName("gridLayout_2");
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        targetLabel = new QLabel(groupBox);
        targetLabel->setObjectName("targetLabel");

        horizontalLayout->addWidget(targetLabel);

        targetNameLabel = new QLabel(groupBox);
        targetNameLabel->setObjectName("targetNameLabel");

        horizontalLayout->addWidget(targetNameLabel);

        targetSelectButton = new QPushButton(groupBox);
        targetSelectButton->setObjectName("targetSelectButton");

        horizontalLayout->addWidget(targetSelectButton);


        gridLayout_2->addLayout(horizontalLayout, 0, 0, 1, 1);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        toolLabel = new QLabel(groupBox);
        toolLabel->setObjectName("toolLabel");

        horizontalLayout_2->addWidget(toolLabel);

        toolNameLabel = new QLabel(groupBox);
        toolNameLabel->setObjectName("toolNameLabel");

        horizontalLayout_2->addWidget(toolNameLabel);

        toolSelectButton = new QPushButton(groupBox);
        toolSelectButton->setObjectName("toolSelectButton");

        horizontalLayout_2->addWidget(toolSelectButton);


        gridLayout_2->addLayout(horizontalLayout_2, 1, 0, 1, 1);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        operationLabel = new QLabel(groupBox);
        operationLabel->setObjectName("operationLabel");

        horizontalLayout_3->addWidget(operationLabel);

        operationComboBox = new QComboBox(groupBox);
        operationComboBox->setObjectName("operationComboBox");

        horizontalLayout_3->addWidget(operationComboBox);


        gridLayout_2->addLayout(horizontalLayout_3, 2, 0, 1, 1);


        verticalLayout->addWidget(groupBox);

        settingsGroupBox = new QGroupBox(booloperationdialog);
        settingsGroupBox->setObjectName("settingsGroupBox");
        settingsLayout = new QVBoxLayout(settingsGroupBox);
        settingsLayout->setObjectName("settingsLayout");
        keepTargetCheckBox = new QCheckBox(settingsGroupBox);
        keepTargetCheckBox->setObjectName("keepTargetCheckBox");
        keepTargetCheckBox->setChecked(false);

        settingsLayout->addWidget(keepTargetCheckBox);

        keepToolCheckBox = new QCheckBox(settingsGroupBox);
        keepToolCheckBox->setObjectName("keepToolCheckBox");
        keepToolCheckBox->setChecked(false);

        settingsLayout->addWidget(keepToolCheckBox);


        verticalLayout->addWidget(settingsGroupBox);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName("horizontalLayout_4");
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer);

        okButton = new QPushButton(booloperationdialog);
        okButton->setObjectName("okButton");

        horizontalLayout_4->addWidget(okButton);

        cancelButton = new QPushButton(booloperationdialog);
        cancelButton->setObjectName("cancelButton");

        horizontalLayout_4->addWidget(cancelButton);


        verticalLayout->addLayout(horizontalLayout_4);


        retranslateUi(booloperationdialog);

        QMetaObject::connectSlotsByName(booloperationdialog);
    } // setupUi

    void retranslateUi(QDialog *booloperationdialog)
    {
        booloperationdialog->setWindowTitle(QCoreApplication::translate("booloperationdialog", "\345\270\203\345\260\224\350\277\220\347\256\227", nullptr));
        groupBox->setTitle(QCoreApplication::translate("booloperationdialog", "\351\200\211\346\213\251\346\223\215\344\275\234\345\257\271\350\261\241", nullptr));
        targetLabel->setText(QCoreApplication::translate("booloperationdialog", "\347\233\256\346\240\207", nullptr));
        targetNameLabel->setText(QCoreApplication::translate("booloperationdialog", "\346\234\252\351\200\211\346\213\251", nullptr));
        targetSelectButton->setText(QCoreApplication::translate("booloperationdialog", "\351\200\211\346\213\251\344\275\223", nullptr));
        toolLabel->setText(QCoreApplication::translate("booloperationdialog", "\345\267\245\345\205\267", nullptr));
        toolNameLabel->setText(QCoreApplication::translate("booloperationdialog", "\346\234\252\351\200\211\346\213\251", nullptr));
        toolSelectButton->setText(QCoreApplication::translate("booloperationdialog", "\351\200\211\346\213\251\344\275\223", nullptr));
        operationLabel->setText(QCoreApplication::translate("booloperationdialog", "\350\277\220\347\256\227\347\261\273\345\236\213", nullptr));
        settingsGroupBox->setTitle(QCoreApplication::translate("booloperationdialog", "\350\256\276\347\275\256", nullptr));
        keepTargetCheckBox->setText(QCoreApplication::translate("booloperationdialog", "\344\277\235\345\255\230\347\233\256\346\240\207", nullptr));
        keepToolCheckBox->setText(QCoreApplication::translate("booloperationdialog", "\344\277\235\345\255\230\345\267\245\345\205\267", nullptr));
        okButton->setText(QCoreApplication::translate("booloperationdialog", "\347\241\256\345\256\232", nullptr));
        cancelButton->setText(QCoreApplication::translate("booloperationdialog", "\345\217\226\346\266\210", nullptr));
    } // retranslateUi

};

namespace Ui {
    class booloperationdialog: public Ui_booloperationdialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_BOOLOPERATIONDIALOG_H
