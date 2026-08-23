/********************************************************************************
** Form generated from reading UI file 'filletdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_FILLETDIALOG_H
#define UI_FILLETDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>

QT_BEGIN_NAMESPACE

class Ui_filletdialog
{
public:
    QGridLayout *gridLayout;
    QGroupBox *groupBox;
    QGridLayout *gridLayout_2;
    QLabel *continuity;
    QComboBox *comboBox;
    QPushButton *Selected_Edge;
    QLabel *shape;
    QComboBox *comboBox_2;
    QLabel *radius;
    QDoubleSpinBox *doubleSpinBox;
    QDialogButtonBox *buttonBox;
    QLabel *edge;
    QSpacerItem *verticalSpacer;

    void setupUi(QDialog *filletdialog)
    {
        if (filletdialog->objectName().isEmpty())
            filletdialog->setObjectName("filletdialog");
        filletdialog->resize(400, 300);
        gridLayout = new QGridLayout(filletdialog);
        gridLayout->setObjectName("gridLayout");
        groupBox = new QGroupBox(filletdialog);
        groupBox->setObjectName("groupBox");
        groupBox->setFlat(true);
        gridLayout_2 = new QGridLayout(groupBox);
        gridLayout_2->setObjectName("gridLayout_2");
        continuity = new QLabel(groupBox);
        continuity->setObjectName("continuity");

        gridLayout_2->addWidget(continuity, 0, 0, 1, 1);

        comboBox = new QComboBox(groupBox);
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->setObjectName("comboBox");

        gridLayout_2->addWidget(comboBox, 0, 1, 1, 1);

        Selected_Edge = new QPushButton(groupBox);
        Selected_Edge->setObjectName("Selected_Edge");
        Selected_Edge->setFlat(true);

        gridLayout_2->addWidget(Selected_Edge, 1, 0, 1, 2);

        shape = new QLabel(groupBox);
        shape->setObjectName("shape");

        gridLayout_2->addWidget(shape, 2, 0, 1, 1);

        comboBox_2 = new QComboBox(groupBox);
        comboBox_2->addItem(QString());
        comboBox_2->addItem(QString());
        comboBox_2->setObjectName("comboBox_2");

        gridLayout_2->addWidget(comboBox_2, 2, 1, 1, 1);

        radius = new QLabel(groupBox);
        radius->setObjectName("radius");

        gridLayout_2->addWidget(radius, 3, 0, 1, 1);

        doubleSpinBox = new QDoubleSpinBox(groupBox);
        doubleSpinBox->setObjectName("doubleSpinBox");
        doubleSpinBox->setDecimals(2);
        doubleSpinBox->setSingleStep(0.100000000000000);
        doubleSpinBox->setStepType(QAbstractSpinBox::StepType::DefaultStepType);
        doubleSpinBox->setValue(0.100000000000000);

        gridLayout_2->addWidget(doubleSpinBox, 3, 1, 1, 1);


        gridLayout->addWidget(groupBox, 1, 0, 1, 1);

        buttonBox = new QDialogButtonBox(filletdialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setOrientation(Qt::Orientation::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::StandardButton::Cancel|QDialogButtonBox::StandardButton::Ok);

        gridLayout->addWidget(buttonBox, 3, 0, 1, 1);

        edge = new QLabel(filletdialog);
        edge->setObjectName("edge");
        QFont font;
        font.setPointSize(13);
        edge->setFont(font);
        edge->setStyleSheet(QString::fromUtf8("color: rgb(85, 160, 185);"));

        gridLayout->addWidget(edge, 0, 0, 1, 1);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout->addItem(verticalSpacer, 2, 0, 1, 1);


        retranslateUi(filletdialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, filletdialog, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, filletdialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(filletdialog);
    } // setupUi

    void retranslateUi(QDialog *filletdialog)
    {
        filletdialog->setWindowTitle(QCoreApplication::translate("filletdialog", "\350\276\271\345\200\222\345\234\206", nullptr));
        groupBox->setTitle(QString());
        continuity->setText(QCoreApplication::translate("filletdialog", "\350\277\236\347\273\255\346\200\247", nullptr));
        comboBox->setItemText(0, QCoreApplication::translate("filletdialog", "G1\357\274\210\347\233\270\345\210\207\357\274\211", nullptr));
        comboBox->setItemText(1, QCoreApplication::translate("filletdialog", "G2\357\274\210\346\233\262\347\216\207\357\274\211", nullptr));

        Selected_Edge->setText(QCoreApplication::translate("filletdialog", "\345\267\262\351\200\211 0 \346\235\241", nullptr));
        shape->setText(QCoreApplication::translate("filletdialog", "\345\275\242\347\212\266", nullptr));
        comboBox_2->setItemText(0, QCoreApplication::translate("filletdialog", "\345\234\206", nullptr));
        comboBox_2->setItemText(1, QCoreApplication::translate("filletdialog", "\344\272\214\346\254\241\346\233\262\347\272\277", nullptr));

        radius->setText(QCoreApplication::translate("filletdialog", "\345\215\212\345\276\204", nullptr));
        edge->setText(QCoreApplication::translate("filletdialog", "\350\276\271", nullptr));
    } // retranslateUi

};

namespace Ui {
    class filletdialog: public Ui_filletdialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_FILLETDIALOG_H
