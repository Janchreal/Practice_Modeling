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
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>

QT_BEGIN_NAMESPACE

class Ui_booloperationdialog
{
public:
    QGridLayout *gridLayout;
    QGroupBox *groupBox;
    QGridLayout *gridLayout_2;
    QLabel *operationLabel;
    QPushButton *targetSelectButton;
    QPushButton *toolSelectButton;
    QLabel *toolNameLabel;
    QLabel *targetNameLabel;
    QPushButton *okButton;
    QPushButton *cancelButton;
    QComboBox *operationComboBox;

    void setupUi(QDialog *booloperationdialog)
    {
        if (booloperationdialog->objectName().isEmpty())
            booloperationdialog->setObjectName("booloperationdialog");
        booloperationdialog->resize(400, 300);
        gridLayout = new QGridLayout(booloperationdialog);
        gridLayout->setObjectName("gridLayout");
        groupBox = new QGroupBox(booloperationdialog);
        groupBox->setObjectName("groupBox");
        gridLayout_2 = new QGridLayout(groupBox);
        gridLayout_2->setObjectName("gridLayout_2");
        operationLabel = new QLabel(groupBox);
        operationLabel->setObjectName("operationLabel");

        gridLayout_2->addWidget(operationLabel, 2, 0, 1, 1);

        targetSelectButton = new QPushButton(groupBox);
        targetSelectButton->setObjectName("targetSelectButton");

        gridLayout_2->addWidget(targetSelectButton, 0, 1, 1, 1);

        toolSelectButton = new QPushButton(groupBox);
        toolSelectButton->setObjectName("toolSelectButton");

        gridLayout_2->addWidget(toolSelectButton, 1, 1, 1, 1);

        toolNameLabel = new QLabel(groupBox);
        toolNameLabel->setObjectName("toolNameLabel");

        gridLayout_2->addWidget(toolNameLabel, 1, 0, 1, 1);

        targetNameLabel = new QLabel(groupBox);
        targetNameLabel->setObjectName("targetNameLabel");

        gridLayout_2->addWidget(targetNameLabel, 0, 0, 1, 1);

        okButton = new QPushButton(groupBox);
        okButton->setObjectName("okButton");

        gridLayout_2->addWidget(okButton, 3, 0, 1, 1);

        cancelButton = new QPushButton(groupBox);
        cancelButton->setObjectName("cancelButton");

        gridLayout_2->addWidget(cancelButton, 3, 1, 1, 1);

        operationComboBox = new QComboBox(groupBox);
        operationComboBox->setObjectName("operationComboBox");

        gridLayout_2->addWidget(operationComboBox, 2, 1, 1, 1);


        gridLayout->addWidget(groupBox, 0, 0, 1, 1);


        retranslateUi(booloperationdialog);

        QMetaObject::connectSlotsByName(booloperationdialog);
    } // setupUi

    void retranslateUi(QDialog *booloperationdialog)
    {
        booloperationdialog->setWindowTitle(QCoreApplication::translate("booloperationdialog", "\345\270\203\345\260\224\350\277\220\347\256\227", nullptr));
        groupBox->setTitle(QCoreApplication::translate("booloperationdialog", "\351\200\211\346\213\251\346\223\215\344\275\234\345\257\271\350\261\241", nullptr));
        operationLabel->setText(QCoreApplication::translate("booloperationdialog", "\350\277\220\347\256\227\347\261\273\345\236\213", nullptr));
        targetSelectButton->setText(QCoreApplication::translate("booloperationdialog", "\351\200\211\346\213\251", nullptr));
        toolSelectButton->setText(QCoreApplication::translate("booloperationdialog", "\351\200\211\346\213\251", nullptr));
        toolNameLabel->setText(QCoreApplication::translate("booloperationdialog", "\345\267\245\345\205\267\344\275\223", nullptr));
        targetNameLabel->setText(QCoreApplication::translate("booloperationdialog", "\347\233\256\346\240\207\344\275\223", nullptr));
        okButton->setText(QCoreApplication::translate("booloperationdialog", "\347\241\256\350\256\244", nullptr));
        cancelButton->setText(QCoreApplication::translate("booloperationdialog", "\345\217\226\346\266\210", nullptr));
    } // retranslateUi

};

namespace Ui {
    class booloperationdialog: public Ui_booloperationdialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_BOOLOPERATIONDIALOG_H
