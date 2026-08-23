/********************************************************************************
** Form generated from reading UI file 'extrusiondialog.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_EXTRUSIONDIALOG_H
#define UI_EXTRUSIONDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ExtrusionDialog
{
public:
    QWidget *widget;
    QVBoxLayout *verticalLayout;
    QGroupBox *groupBox;
    QGridLayout *gridLayout;
    QComboBox *sectionTypeCombo;
    QLabel *selectedCountLabel;
    QPushButton *clearSelectionButton;
    QLabel *label;
    QPushButton *geometrySelectorButton;
    QGroupBox *groupBox_2;
    QGridLayout *gridLayout_3;
    QLabel *label_2;
    QComboBox *comboBox;
    QGroupBox *groupBox_3;
    QGridLayout *gridLayout_2;
    QLabel *label_3;
    QComboBox *startCombo;
    QLabel *startDistanceLabel;
    QLineEdit *startDistanceLineEdit;
    QLabel *label_4;
    QComboBox *endCombo;
    QLabel *endDistanceLabel;
    QLineEdit *endDistanceLineEdit;
    QGroupBox *groupBox_4;
    QGridLayout *gridLayout_4;
    QCheckBox *checkBox;
    QPushButton *previewButton;
    QLabel *label_7;
    QWidget *widget1;
    QHBoxLayout *horizontalLayout;
    QPushButton *okButton;
    QPushButton *cancelButton;

    void setupUi(QDialog *ExtrusionDialog)
    {
        if (ExtrusionDialog->objectName().isEmpty())
            ExtrusionDialog->setObjectName("ExtrusionDialog");
        ExtrusionDialog->resize(429, 405);
        widget = new QWidget(ExtrusionDialog);
        widget->setObjectName("widget");
        widget->setGeometry(QRect(10, 0, 411, 361));
        verticalLayout = new QVBoxLayout(widget);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        groupBox = new QGroupBox(widget);
        groupBox->setObjectName("groupBox");
        gridLayout = new QGridLayout(groupBox);
        gridLayout->setObjectName("gridLayout");
        sectionTypeCombo = new QComboBox(groupBox);
        sectionTypeCombo->addItem(QString());
        sectionTypeCombo->addItem(QString());
        sectionTypeCombo->setObjectName("sectionTypeCombo");

        gridLayout->addWidget(sectionTypeCombo, 0, 2, 1, 2);

        selectedCountLabel = new QLabel(groupBox);
        selectedCountLabel->setObjectName("selectedCountLabel");

        gridLayout->addWidget(selectedCountLabel, 1, 1, 1, 2);

        clearSelectionButton = new QPushButton(groupBox);
        clearSelectionButton->setObjectName("clearSelectionButton");

        gridLayout->addWidget(clearSelectionButton, 1, 3, 1, 1);

        label = new QLabel(groupBox);
        label->setObjectName("label");

        gridLayout->addWidget(label, 0, 0, 1, 2);

        geometrySelectorButton = new QPushButton(groupBox);
        geometrySelectorButton->setObjectName("geometrySelectorButton");

        gridLayout->addWidget(geometrySelectorButton, 1, 0, 1, 1);


        verticalLayout->addWidget(groupBox);

        groupBox_2 = new QGroupBox(widget);
        groupBox_2->setObjectName("groupBox_2");
        gridLayout_3 = new QGridLayout(groupBox_2);
        gridLayout_3->setObjectName("gridLayout_3");
        label_2 = new QLabel(groupBox_2);
        label_2->setObjectName("label_2");

        gridLayout_3->addWidget(label_2, 0, 0, 1, 1);

        comboBox = new QComboBox(groupBox_2);
        comboBox->setObjectName("comboBox");

        gridLayout_3->addWidget(comboBox, 0, 1, 1, 1);


        verticalLayout->addWidget(groupBox_2);

        groupBox_3 = new QGroupBox(widget);
        groupBox_3->setObjectName("groupBox_3");
        gridLayout_2 = new QGridLayout(groupBox_3);
        gridLayout_2->setObjectName("gridLayout_2");
        label_3 = new QLabel(groupBox_3);
        label_3->setObjectName("label_3");

        gridLayout_2->addWidget(label_3, 0, 0, 1, 1);

        startCombo = new QComboBox(groupBox_3);
        startCombo->addItem(QString());
        startCombo->setObjectName("startCombo");

        gridLayout_2->addWidget(startCombo, 0, 1, 1, 1);

        startDistanceLabel = new QLabel(groupBox_3);
        startDistanceLabel->setObjectName("startDistanceLabel");

        gridLayout_2->addWidget(startDistanceLabel, 1, 0, 1, 1);

        startDistanceLineEdit = new QLineEdit(groupBox_3);
        startDistanceLineEdit->setObjectName("startDistanceLineEdit");

        gridLayout_2->addWidget(startDistanceLineEdit, 1, 1, 1, 1);

        label_4 = new QLabel(groupBox_3);
        label_4->setObjectName("label_4");

        gridLayout_2->addWidget(label_4, 2, 0, 1, 1);

        endCombo = new QComboBox(groupBox_3);
        endCombo->addItem(QString());
        endCombo->setObjectName("endCombo");

        gridLayout_2->addWidget(endCombo, 2, 1, 1, 1);

        endDistanceLabel = new QLabel(groupBox_3);
        endDistanceLabel->setObjectName("endDistanceLabel");

        gridLayout_2->addWidget(endDistanceLabel, 3, 0, 1, 1);

        endDistanceLineEdit = new QLineEdit(groupBox_3);
        endDistanceLineEdit->setObjectName("endDistanceLineEdit");

        gridLayout_2->addWidget(endDistanceLineEdit, 3, 1, 1, 1);


        verticalLayout->addWidget(groupBox_3);

        groupBox_4 = new QGroupBox(widget);
        groupBox_4->setObjectName("groupBox_4");
        gridLayout_4 = new QGridLayout(groupBox_4);
        gridLayout_4->setObjectName("gridLayout_4");
        checkBox = new QCheckBox(groupBox_4);
        checkBox->setObjectName("checkBox");

        gridLayout_4->addWidget(checkBox, 1, 0, 1, 1);

        previewButton = new QPushButton(groupBox_4);
        previewButton->setObjectName("previewButton");

        gridLayout_4->addWidget(previewButton, 0, 1, 1, 1);

        label_7 = new QLabel(groupBox_4);
        label_7->setObjectName("label_7");

        gridLayout_4->addWidget(label_7, 0, 0, 1, 1);


        verticalLayout->addWidget(groupBox_4);

        widget1 = new QWidget(ExtrusionDialog);
        widget1->setObjectName("widget1");
        widget1->setGeometry(QRect(170, 380, 251, 21));
        horizontalLayout = new QHBoxLayout(widget1);
        horizontalLayout->setObjectName("horizontalLayout");
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        okButton = new QPushButton(widget1);
        okButton->setObjectName("okButton");

        horizontalLayout->addWidget(okButton);

        cancelButton = new QPushButton(widget1);
        cancelButton->setObjectName("cancelButton");

        horizontalLayout->addWidget(cancelButton);


        retranslateUi(ExtrusionDialog);

        QMetaObject::connectSlotsByName(ExtrusionDialog);
    } // setupUi

    void retranslateUi(QDialog *ExtrusionDialog)
    {
        ExtrusionDialog->setWindowTitle(QCoreApplication::translate("ExtrusionDialog", "Dialog", nullptr));
        groupBox->setTitle(QCoreApplication::translate("ExtrusionDialog", "\350\241\250\345\214\272\345\237\237\351\251\261\345\212\250", nullptr));
        sectionTypeCombo->setItemText(0, QCoreApplication::translate("ExtrusionDialog", "\346\233\262\347\272\277", nullptr));
        sectionTypeCombo->setItemText(1, QCoreApplication::translate("ExtrusionDialog", "\351\235\242\347\211\207", nullptr));

        selectedCountLabel->setText(QCoreApplication::translate("ExtrusionDialog", "\346\230\276\347\244\272\345\267\262\351\200\211\346\213\251\346\225\260\351\207\217", nullptr));
        clearSelectionButton->setText(QCoreApplication::translate("ExtrusionDialog", "\345\217\226\346\266\210\351\200\211\344\270\255", nullptr));
        label->setText(QCoreApplication::translate("ExtrusionDialog", "\350\241\250\345\214\272\345\237\237\347\261\273\345\236\213", nullptr));
        geometrySelectorButton->setText(QCoreApplication::translate("ExtrusionDialog", "\345\207\240\344\275\225\351\200\211\346\213\251\345\231\250", nullptr));
        groupBox_2->setTitle(QCoreApplication::translate("ExtrusionDialog", "\346\226\271\345\220\221", nullptr));
        label_2->setText(QCoreApplication::translate("ExtrusionDialog", "\346\214\207\345\256\232\347\237\242\351\207\217", nullptr));
        groupBox_3->setTitle(QCoreApplication::translate("ExtrusionDialog", "\351\231\220\345\210\266", nullptr));
        label_3->setText(QCoreApplication::translate("ExtrusionDialog", "\345\274\200\345\247\213", nullptr));
        startCombo->setItemText(0, QCoreApplication::translate("ExtrusionDialog", "\345\200\274", nullptr));

        startDistanceLabel->setText(QCoreApplication::translate("ExtrusionDialog", "\350\267\235\347\246\273", nullptr));
        label_4->setText(QCoreApplication::translate("ExtrusionDialog", "\347\273\223\346\235\237", nullptr));
        endCombo->setItemText(0, QCoreApplication::translate("ExtrusionDialog", "\345\200\274", nullptr));

        endDistanceLabel->setText(QCoreApplication::translate("ExtrusionDialog", "\350\267\235\347\246\273", nullptr));
        groupBox_4->setTitle(QCoreApplication::translate("ExtrusionDialog", "\347\273\223\346\236\234", nullptr));
        checkBox->setText(QCoreApplication::translate("ExtrusionDialog", "\350\207\252\345\212\250\345\260\201\345\217\243", nullptr));
        previewButton->setText(QCoreApplication::translate("ExtrusionDialog", "\351\242\204\350\247\210", nullptr));
        label_7->setText(QCoreApplication::translate("ExtrusionDialog", "\346\233\264\346\226\260\347\273\230\345\210\266\347\273\223\346\236\234", nullptr));
        okButton->setText(QCoreApplication::translate("ExtrusionDialog", "\347\241\256\345\256\232", nullptr));
        cancelButton->setText(QCoreApplication::translate("ExtrusionDialog", "\345\217\226\346\266\210", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ExtrusionDialog: public Ui_ExtrusionDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_EXTRUSIONDIALOG_H
