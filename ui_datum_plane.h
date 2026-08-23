/********************************************************************************
** Form generated from reading UI file 'datum_plane.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DATUM_PLANE_H
#define UI_DATUM_PLANE_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>

QT_BEGIN_NAMESPACE

class Ui_datum_plane
{
public:
    QGridLayout *gridLayout;
    QComboBox *comboBox;
    QLabel *Define_PlanerObject;
    QGridLayout *biasLayout;
    QLabel *Bias;
    QCheckBox *checkBox_Bias;
    QDoubleSpinBox *doubleSpinBox_BiasValue;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *datum_plane)
    {
        if (datum_plane->objectName().isEmpty())
            datum_plane->setObjectName("datum_plane");
        datum_plane->resize(400, 300);
        gridLayout = new QGridLayout(datum_plane);
        gridLayout->setObjectName("gridLayout");
        comboBox = new QComboBox(datum_plane);
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->setObjectName("comboBox");

        gridLayout->addWidget(comboBox, 0, 0, 1, 1);

        Define_PlanerObject = new QLabel(datum_plane);
        Define_PlanerObject->setObjectName("Define_PlanerObject");
        Define_PlanerObject->setWordWrap(true);

        gridLayout->addWidget(Define_PlanerObject, 1, 0, 1, 1);

        biasLayout = new QGridLayout();
        biasLayout->setObjectName("biasLayout");
        Bias = new QLabel(datum_plane);
        Bias->setObjectName("Bias");

        biasLayout->addWidget(Bias, 0, 0, 1, 1);

        checkBox_Bias = new QCheckBox(datum_plane);
        checkBox_Bias->setObjectName("checkBox_Bias");

        biasLayout->addWidget(checkBox_Bias, 1, 0, 1, 1);

        doubleSpinBox_BiasValue = new QDoubleSpinBox(datum_plane);
        doubleSpinBox_BiasValue->setObjectName("doubleSpinBox_BiasValue");
        doubleSpinBox_BiasValue->setDecimals(3);
        doubleSpinBox_BiasValue->setMinimum(-1000000.000000000000000);
        doubleSpinBox_BiasValue->setMaximum(1000000.000000000000000);
        doubleSpinBox_BiasValue->setValue(0.000000000000000);

        biasLayout->addWidget(doubleSpinBox_BiasValue, 1, 1, 1, 1);


        gridLayout->addLayout(biasLayout, 2, 0, 1, 1);

        buttonBox = new QDialogButtonBox(datum_plane);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setOrientation(Qt::Orientation::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::StandardButton::Cancel|QDialogButtonBox::StandardButton::Ok);

        gridLayout->addWidget(buttonBox, 3, 0, 1, 1);


        retranslateUi(datum_plane);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, datum_plane, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, datum_plane, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(datum_plane);
    } // setupUi

    void retranslateUi(QDialog *datum_plane)
    {
        datum_plane->setWindowTitle(QCoreApplication::translate("datum_plane", "\345\237\272\345\207\206\345\271\263\351\235\242", nullptr));
        comboBox->setItemText(0, QCoreApplication::translate("datum_plane", "\350\207\252\345\212\250\345\210\244\346\226\255", nullptr));
        comboBox->setItemText(1, QCoreApplication::translate("datum_plane", "\344\270\244\347\233\264\347\272\277", nullptr));
        comboBox->setItemText(2, QCoreApplication::translate("datum_plane", "\351\200\232\350\277\207\345\257\271\350\261\241", nullptr));
        comboBox->setItemText(3, QCoreApplication::translate("datum_plane", "\346\233\262\347\272\277\345\222\214\347\202\271", nullptr));
        comboBox->setItemText(4, QCoreApplication::translate("datum_plane", "YC-ZC\345\271\263\351\235\242", nullptr));
        comboBox->setItemText(5, QCoreApplication::translate("datum_plane", "XC-ZC\345\271\263\351\235\242", nullptr));
        comboBox->setItemText(6, QCoreApplication::translate("datum_plane", "XC-YC\345\271\263\351\235\242", nullptr));

        Define_PlanerObject->setText(QCoreApplication::translate("datum_plane", "\350\246\201\345\256\232\344\271\211\345\271\263\351\235\242\347\232\204\345\257\271\350\261\241", nullptr));
        Bias->setText(QCoreApplication::translate("datum_plane", "\345\201\217\347\275\256", nullptr));
        checkBox_Bias->setText(QCoreApplication::translate("datum_plane", "\345\201\217\347\275\256", nullptr));
    } // retranslateUi

};

namespace Ui {
    class datum_plane: public Ui_datum_plane {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DATUM_PLANE_H
