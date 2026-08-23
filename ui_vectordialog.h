/********************************************************************************
** Form generated from reading UI file 'vectordialog.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_VECTORDIALOG_H
#define UI_VECTORDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_vectordialog
{
public:
    QGridLayout *gridLayout_2;
    QComboBox *comboBox;
    QGroupBox *groupBox;
    QGridLayout *gridLayout;
    QSpacerItem *verticalSpacer;
    QSpacerItem *horizontalSpacer;
    QPushButton *pushButton;
    QDialogButtonBox *buttonBox;
    QVBoxLayout *vectorDefineLayout;

    void setupUi(QDialog *vectordialog)
    {
        if (vectordialog->objectName().isEmpty())
            vectordialog->setObjectName("vectordialog");
        vectordialog->resize(195, 200);
        gridLayout_2 = new QGridLayout(vectordialog);
        gridLayout_2->setObjectName("gridLayout_2");
        comboBox = new QComboBox(vectordialog);
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->setObjectName("comboBox");

        gridLayout_2->addWidget(comboBox, 0, 0, 1, 1);

        groupBox = new QGroupBox(vectordialog);
        groupBox->setObjectName("groupBox");
        groupBox->setFlat(true);
        gridLayout = new QGridLayout(groupBox);
        gridLayout->setObjectName("gridLayout");
        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout->addItem(verticalSpacer, 1, 0, 1, 1);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout->addItem(horizontalSpacer, 0, 1, 1, 1);

        pushButton = new QPushButton(groupBox);
        pushButton->setObjectName("pushButton");

        gridLayout->addWidget(pushButton, 0, 0, 1, 1);


        gridLayout_2->addWidget(groupBox, 1, 0, 1, 1);

        buttonBox = new QDialogButtonBox(vectordialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setOrientation(Qt::Orientation::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::StandardButton::Cancel|QDialogButtonBox::StandardButton::Ok);

        gridLayout_2->addWidget(buttonBox, 3, 0, 1, 1);

        vectorDefineLayout = new QVBoxLayout();
        vectorDefineLayout->setObjectName("vectorDefineLayout");

        gridLayout_2->addLayout(vectorDefineLayout, 2, 0, 1, 1);


        retranslateUi(vectordialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, vectordialog, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, vectordialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(vectordialog);
    } // setupUi

    void retranslateUi(QDialog *vectordialog)
    {
        vectordialog->setWindowTitle(QCoreApplication::translate("vectordialog", "Dialog", nullptr));
        comboBox->setItemText(0, QCoreApplication::translate("vectordialog", "\350\207\252\345\212\250\345\210\244\346\226\255\347\232\204\347\237\242\351\207\217", nullptr));
        comboBox->setItemText(1, QCoreApplication::translate("vectordialog", "\344\270\244\347\202\271", nullptr));
        comboBox->setItemText(2, QCoreApplication::translate("vectordialog", "\344\270\216XC\346\210\220\344\270\200\345\256\232\350\247\222\345\272\246", nullptr));
        comboBox->setItemText(3, QCoreApplication::translate("vectordialog", "\346\233\262\347\272\277/\350\275\264\347\237\242\351\207\217", nullptr));
        comboBox->setItemText(4, QCoreApplication::translate("vectordialog", "\346\233\262\347\272\277\344\270\212\347\237\242\351\207\217", nullptr));
        comboBox->setItemText(5, QCoreApplication::translate("vectordialog", "\351\235\242/\345\271\263\351\235\242\346\263\225\345\220\221\351\207\217", nullptr));
        comboBox->setItemText(6, QCoreApplication::translate("vectordialog", "\351\235\242\344\270\212\347\202\271\347\232\204\347\237\242\351\207\217", nullptr));
        comboBox->setItemText(7, QCoreApplication::translate("vectordialog", "XC\350\275\264", nullptr));
        comboBox->setItemText(8, QCoreApplication::translate("vectordialog", "YC\350\275\264", nullptr));
        comboBox->setItemText(9, QCoreApplication::translate("vectordialog", "ZC\350\275\264", nullptr));
        comboBox->setItemText(10, QCoreApplication::translate("vectordialog", "-XC\350\275\264", nullptr));
        comboBox->setItemText(11, QCoreApplication::translate("vectordialog", "-YC\350\275\264", nullptr));
        comboBox->setItemText(12, QCoreApplication::translate("vectordialog", "-ZC\350\275\264", nullptr));
        comboBox->setItemText(13, QCoreApplication::translate("vectordialog", "\350\247\206\345\233\276\346\226\271\345\220\221", nullptr));

        groupBox->setTitle(QCoreApplication::translate("vectordialog", "\347\237\242\351\207\217\346\226\271\344\275\215", nullptr));
        pushButton->setText(QCoreApplication::translate("vectordialog", "\345\217\215\350\275\254", nullptr));
    } // retranslateUi

};

namespace Ui {
    class vectordialog: public Ui_vectordialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_VECTORDIALOG_H
