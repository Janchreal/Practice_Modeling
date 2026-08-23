/********************************************************************************
** Form generated from reading UI file 'chamferdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CHAMFERDIALOG_H
#define UI_CHAMFERDIALOG_H

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
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_chamferdialog
{
public:
    QVBoxLayout *verticalLayout;
    QLabel *edge;
    QPushButton *selectededge;
    QGroupBox *groupBox_2;
    QGridLayout *gridLayout;
    QLabel *label_3;
    QComboBox *comboBox;
    QLabel *label_4;
    QLabel *label_2;
    QSpacerItem *verticalSpacer;
    QDoubleSpinBox *doubleSpinBox;
    QDoubleSpinBox *distance2SpinBox;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *chamferdialog)
    {
        if (chamferdialog->objectName().isEmpty())
            chamferdialog->setObjectName("chamferdialog");
        chamferdialog->resize(400, 300);
        verticalLayout = new QVBoxLayout(chamferdialog);
        verticalLayout->setObjectName("verticalLayout");
        edge = new QLabel(chamferdialog);
        edge->setObjectName("edge");
        QFont font;
        font.setPointSize(13);
        edge->setFont(font);
        edge->setStyleSheet(QString::fromUtf8("color: rgb(85, 160, 185);"));

        verticalLayout->addWidget(edge);

        selectededge = new QPushButton(chamferdialog);
        selectededge->setObjectName("selectededge");
        selectededge->setFlat(true);

        verticalLayout->addWidget(selectededge);

        groupBox_2 = new QGroupBox(chamferdialog);
        groupBox_2->setObjectName("groupBox_2");
        groupBox_2->setFlat(true);
        gridLayout = new QGridLayout(groupBox_2);
        gridLayout->setObjectName("gridLayout");
        label_3 = new QLabel(groupBox_2);
        label_3->setObjectName("label_3");

        gridLayout->addWidget(label_3, 1, 0, 1, 1);

        comboBox = new QComboBox(groupBox_2);
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->setObjectName("comboBox");

        gridLayout->addWidget(comboBox, 1, 1, 2, 1);

        label_4 = new QLabel(groupBox_2);
        label_4->setObjectName("label_4");

        gridLayout->addWidget(label_4, 2, 0, 3, 1);

        label_2 = new QLabel(groupBox_2);
        label_2->setObjectName("label_2");

        gridLayout->addWidget(label_2, 0, 0, 1, 1);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout->addItem(verticalSpacer, 5, 0, 1, 1);

        doubleSpinBox = new QDoubleSpinBox(groupBox_2);
        doubleSpinBox->setObjectName("doubleSpinBox");

        gridLayout->addWidget(doubleSpinBox, 3, 1, 1, 1);

        distance2SpinBox = new QDoubleSpinBox(groupBox_2);
        distance2SpinBox->setObjectName("distance2SpinBox");

        gridLayout->addWidget(distance2SpinBox, 4, 1, 1, 1);


        verticalLayout->addWidget(groupBox_2);

        buttonBox = new QDialogButtonBox(chamferdialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setOrientation(Qt::Orientation::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::StandardButton::Cancel|QDialogButtonBox::StandardButton::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(chamferdialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, chamferdialog, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, chamferdialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(chamferdialog);
    } // setupUi

    void retranslateUi(QDialog *chamferdialog)
    {
        chamferdialog->setWindowTitle(QCoreApplication::translate("chamferdialog", "Dialog", nullptr));
        edge->setText(QCoreApplication::translate("chamferdialog", "\350\276\271", nullptr));
        selectededge->setText(QCoreApplication::translate("chamferdialog", "\345\267\262\351\200\211 0 \346\235\241", nullptr));
        groupBox_2->setTitle(QString());
        label_3->setText(QCoreApplication::translate("chamferdialog", "\346\250\252\346\210\252\351\235\242", nullptr));
        comboBox->setItemText(0, QCoreApplication::translate("chamferdialog", "\345\257\271\347\247\260", nullptr));
        comboBox->setItemText(1, QCoreApplication::translate("chamferdialog", "\351\235\236\345\257\271\347\247\260", nullptr));
        comboBox->setItemText(2, QCoreApplication::translate("chamferdialog", "\345\201\217\347\275\256\345\222\214\350\247\222\345\272\246", nullptr));

        label_4->setText(QCoreApplication::translate("chamferdialog", "\350\267\235\347\246\273", nullptr));
        label_2->setText(QCoreApplication::translate("chamferdialog", "\345\201\217\347\275\256", nullptr));
    } // retranslateUi

};

namespace Ui {
    class chamferdialog: public Ui_chamferdialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CHAMFERDIALOG_H
