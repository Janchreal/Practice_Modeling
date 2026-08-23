/********************************************************************************
** Form generated from reading UI file 'sketchcreatedialog.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SKETCHCREATEDIALOG_H
#define UI_SKETCHCREATEDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QToolButton>

QT_BEGIN_NAMESPACE

class Ui_SketchCreateDialog
{
public:
    QGridLayout *gridLayout;
    QComboBox *comboBox_method;
    QGroupBox *groupBox_csys;
    QGridLayout *gridLayout_csys;
    QLabel *label_planeMethod;
    QComboBox *comboBox_planeMethod;
    QLabel *label_ref;
    QComboBox *comboBox_reference;
    QLabel *label_originMethod;
    QComboBox *comboBox_originMethod;
    QHBoxLayout *horizontalLayout_planePick;
    QCheckBox *checkBox_pickEnabled;
    QSpacerItem *horizontalSpacer;
    QToolButton *toolButton_pickPlane;
    QLabel *label_planeInfo;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *SketchCreateDialog)
    {
        if (SketchCreateDialog->objectName().isEmpty())
            SketchCreateDialog->setObjectName("SketchCreateDialog");
        SketchCreateDialog->resize(430, 230);
        gridLayout = new QGridLayout(SketchCreateDialog);
        gridLayout->setObjectName("gridLayout");
        comboBox_method = new QComboBox(SketchCreateDialog);
        comboBox_method->addItem(QString());
        comboBox_method->addItem(QString());
        comboBox_method->setObjectName("comboBox_method");

        gridLayout->addWidget(comboBox_method, 0, 0, 1, 1);

        groupBox_csys = new QGroupBox(SketchCreateDialog);
        groupBox_csys->setObjectName("groupBox_csys");
        gridLayout_csys = new QGridLayout(groupBox_csys);
        gridLayout_csys->setObjectName("gridLayout_csys");
        label_planeMethod = new QLabel(groupBox_csys);
        label_planeMethod->setObjectName("label_planeMethod");

        gridLayout_csys->addWidget(label_planeMethod, 0, 0, 1, 1);

        comboBox_planeMethod = new QComboBox(groupBox_csys);
        comboBox_planeMethod->addItem(QString());
        comboBox_planeMethod->addItem(QString());
        comboBox_planeMethod->setObjectName("comboBox_planeMethod");

        gridLayout_csys->addWidget(comboBox_planeMethod, 0, 1, 1, 1);

        label_ref = new QLabel(groupBox_csys);
        label_ref->setObjectName("label_ref");

        gridLayout_csys->addWidget(label_ref, 1, 0, 1, 1);

        comboBox_reference = new QComboBox(groupBox_csys);
        comboBox_reference->addItem(QString());
        comboBox_reference->addItem(QString());
        comboBox_reference->setObjectName("comboBox_reference");

        gridLayout_csys->addWidget(comboBox_reference, 1, 1, 1, 1);

        label_originMethod = new QLabel(groupBox_csys);
        label_originMethod->setObjectName("label_originMethod");

        gridLayout_csys->addWidget(label_originMethod, 2, 0, 1, 1);

        comboBox_originMethod = new QComboBox(groupBox_csys);
        comboBox_originMethod->addItem(QString());
        comboBox_originMethod->addItem(QString());
        comboBox_originMethod->setObjectName("comboBox_originMethod");

        gridLayout_csys->addWidget(comboBox_originMethod, 2, 1, 1, 1);

        horizontalLayout_planePick = new QHBoxLayout();
        horizontalLayout_planePick->setObjectName("horizontalLayout_planePick");
        checkBox_pickEnabled = new QCheckBox(groupBox_csys);
        checkBox_pickEnabled->setObjectName("checkBox_pickEnabled");
        checkBox_pickEnabled->setChecked(true);

        horizontalLayout_planePick->addWidget(checkBox_pickEnabled);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_planePick->addItem(horizontalSpacer);

        toolButton_pickPlane = new QToolButton(groupBox_csys);
        toolButton_pickPlane->setObjectName("toolButton_pickPlane");

        horizontalLayout_planePick->addWidget(toolButton_pickPlane);


        gridLayout_csys->addLayout(horizontalLayout_planePick, 3, 0, 1, 2);

        label_planeInfo = new QLabel(groupBox_csys);
        label_planeInfo->setObjectName("label_planeInfo");
        label_planeInfo->setWordWrap(true);

        gridLayout_csys->addWidget(label_planeInfo, 4, 0, 1, 2);


        gridLayout->addWidget(groupBox_csys, 1, 0, 1, 1);

        buttonBox = new QDialogButtonBox(SketchCreateDialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setOrientation(Qt::Orientation::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        gridLayout->addWidget(buttonBox, 2, 0, 1, 1);


        retranslateUi(SketchCreateDialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, SketchCreateDialog, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, SketchCreateDialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(SketchCreateDialog);
    } // setupUi

    void retranslateUi(QDialog *SketchCreateDialog)
    {
        SketchCreateDialog->setWindowTitle(QCoreApplication::translate("SketchCreateDialog", "\345\210\233\345\273\272\350\215\211\345\233\276", nullptr));
        comboBox_method->setItemText(0, QCoreApplication::translate("SketchCreateDialog", "\345\234\250\345\271\263\351\235\242\344\270\212", nullptr));
        comboBox_method->setItemText(1, QCoreApplication::translate("SketchCreateDialog", "\345\237\272\344\272\216\350\267\257\345\276\204", nullptr));

        groupBox_csys->setTitle(QCoreApplication::translate("SketchCreateDialog", "\350\215\211\345\233\276\345\235\220\346\240\207\347\263\273", nullptr));
        label_planeMethod->setText(QCoreApplication::translate("SketchCreateDialog", "\345\271\263\351\235\242\346\226\271\346\263\225", nullptr));
        comboBox_planeMethod->setItemText(0, QCoreApplication::translate("SketchCreateDialog", "\350\207\252\345\212\250\345\210\244\346\226\255", nullptr));
        comboBox_planeMethod->setItemText(1, QCoreApplication::translate("SketchCreateDialog", "\346\226\260\345\271\263\351\235\242", nullptr));

        label_ref->setText(QCoreApplication::translate("SketchCreateDialog", "\345\217\202\350\200\203", nullptr));
        comboBox_reference->setItemText(0, QCoreApplication::translate("SketchCreateDialog", "\346\260\264\345\271\263", nullptr));
        comboBox_reference->setItemText(1, QCoreApplication::translate("SketchCreateDialog", "\347\253\226\347\233\264", nullptr));

        label_originMethod->setText(QCoreApplication::translate("SketchCreateDialog", "\345\216\237\347\202\271\346\226\271\346\263\225", nullptr));
        comboBox_originMethod->setItemText(0, QCoreApplication::translate("SketchCreateDialog", "\346\214\207\345\256\232\347\202\271", nullptr));
        comboBox_originMethod->setItemText(1, QCoreApplication::translate("SketchCreateDialog", "\344\275\277\347\224\250\345\267\245\344\275\234\351\203\250\344\273\266\345\216\237\347\202\271", nullptr));

        checkBox_pickEnabled->setText(QCoreApplication::translate("SketchCreateDialog", "\346\214\207\345\256\232\345\235\220\346\240\207\347\263\273", nullptr));
        toolButton_pickPlane->setText(QCoreApplication::translate("SketchCreateDialog", "\346\213\276\345\217\226", nullptr));
        label_planeInfo->setText(QCoreApplication::translate("SketchCreateDialog", "\345\217\202\350\200\203\345\271\263\351\235\242\357\274\232\346\234\252\346\214\207\345\256\232\357\274\210\350\257\267\347\202\271\345\207\273\345\217\263\344\276\247\346\213\276\345\217\226\357\274\211", nullptr));
    } // retranslateUi

};

namespace Ui {
    class SketchCreateDialog: public Ui_SketchCreateDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SKETCHCREATEDIALOG_H
