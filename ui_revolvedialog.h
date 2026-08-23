/********************************************************************************
** Form generated from reading UI file 'revolvedialog.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_REVOLVEDIALOG_H
#define UI_REVOLVEDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_revolvedialog
{
public:
    QGridLayout *gridLayout_5;
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
    QPushButton *selectButton;
    QToolButton *toolButton;
    QPushButton *pushButton_2;
    QLabel *label_2;
    QToolButton *originSnapToolButton;
    QLabel *label_5;
    QPushButton *pushButton;
    QPushButton *pushButton_3;
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
    QLabel *label_7;
    QPushButton *previewButton;
    QCheckBox *checkBox;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *revolvedialog)
    {
        if (revolvedialog->objectName().isEmpty())
            revolvedialog->setObjectName("revolvedialog");
        revolvedialog->resize(352, 413);
        gridLayout_5 = new QGridLayout(revolvedialog);
        gridLayout_5->setObjectName("gridLayout_5");
        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName("verticalLayout");
        groupBox = new QGroupBox(revolvedialog);
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

        groupBox_2 = new QGroupBox(revolvedialog);
        groupBox_2->setObjectName("groupBox_2");
        gridLayout_3 = new QGridLayout(groupBox_2);
        gridLayout_3->setObjectName("gridLayout_3");
        selectButton = new QPushButton(groupBox_2);
        selectButton->setObjectName("selectButton");

        gridLayout_3->addWidget(selectButton, 0, 1, 1, 1);

        toolButton = new QToolButton(groupBox_2);
        toolButton->setObjectName("toolButton");

        gridLayout_3->addWidget(toolButton, 0, 4, 1, 1);

        pushButton_2 = new QPushButton(groupBox_2);
        pushButton_2->setObjectName("pushButton_2");

        gridLayout_3->addWidget(pushButton_2, 0, 2, 1, 1);

        label_2 = new QLabel(groupBox_2);
        label_2->setObjectName("label_2");

        gridLayout_3->addWidget(label_2, 0, 0, 1, 1);

        originSnapToolButton = new QToolButton(groupBox_2);
        originSnapToolButton->setObjectName("originSnapToolButton");

        gridLayout_3->addWidget(originSnapToolButton, 1, 4, 1, 1);

        label_5 = new QLabel(groupBox_2);
        label_5->setObjectName("label_5");

        gridLayout_3->addWidget(label_5, 1, 0, 1, 1);

        pushButton = new QPushButton(groupBox_2);
        pushButton->setObjectName("pushButton");

        gridLayout_3->addWidget(pushButton, 1, 1, 1, 1);

        pushButton_3 = new QPushButton(groupBox_2);
        pushButton_3->setObjectName("pushButton_3");

        gridLayout_3->addWidget(pushButton_3, 0, 3, 1, 1);


        verticalLayout->addWidget(groupBox_2);

        groupBox_3 = new QGroupBox(revolvedialog);
        groupBox_3->setObjectName("groupBox_3");
        gridLayout_2 = new QGridLayout(groupBox_3);
        gridLayout_2->setObjectName("gridLayout_2");
        label_3 = new QLabel(groupBox_3);
        label_3->setObjectName("label_3");

        gridLayout_2->addWidget(label_3, 0, 0, 1, 1);

        startCombo = new QComboBox(groupBox_3);
        startCombo->addItem(QString());
        startCombo->setObjectName("startCombo");
        QFont font;
        font.setKerning(true);
        startCombo->setFont(font);
        startCombo->setIconSize(QSize(20, 20));

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
        endCombo->setIconSize(QSize(20, 20));

        gridLayout_2->addWidget(endCombo, 2, 1, 1, 1);

        endDistanceLabel = new QLabel(groupBox_3);
        endDistanceLabel->setObjectName("endDistanceLabel");

        gridLayout_2->addWidget(endDistanceLabel, 3, 0, 1, 1);

        endDistanceLineEdit = new QLineEdit(groupBox_3);
        endDistanceLineEdit->setObjectName("endDistanceLineEdit");

        gridLayout_2->addWidget(endDistanceLineEdit, 3, 1, 1, 1);


        verticalLayout->addWidget(groupBox_3);

        groupBox_4 = new QGroupBox(revolvedialog);
        groupBox_4->setObjectName("groupBox_4");
        gridLayout_4 = new QGridLayout(groupBox_4);
        gridLayout_4->setObjectName("gridLayout_4");
        label_7 = new QLabel(groupBox_4);
        label_7->setObjectName("label_7");

        gridLayout_4->addWidget(label_7, 0, 0, 1, 1);

        previewButton = new QPushButton(groupBox_4);
        previewButton->setObjectName("previewButton");

        gridLayout_4->addWidget(previewButton, 0, 1, 1, 1);

        checkBox = new QCheckBox(groupBox_4);
        checkBox->setObjectName("checkBox");

        gridLayout_4->addWidget(checkBox, 1, 0, 1, 1);

        buttonBox = new QDialogButtonBox(groupBox_4);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setStandardButtons(QDialogButtonBox::StandardButton::Cancel|QDialogButtonBox::StandardButton::Ok);

        gridLayout_4->addWidget(buttonBox, 2, 0, 1, 1);


        verticalLayout->addWidget(groupBox_4);


        gridLayout_5->addLayout(verticalLayout, 0, 0, 1, 1);


        retranslateUi(revolvedialog);

        QMetaObject::connectSlotsByName(revolvedialog);
    } // setupUi

    void retranslateUi(QDialog *revolvedialog)
    {
        revolvedialog->setWindowTitle(QCoreApplication::translate("revolvedialog", "\346\227\213\350\275\254", nullptr));
        groupBox->setTitle(QCoreApplication::translate("revolvedialog", "\350\241\250\345\214\272\345\237\237\351\251\261\345\212\250", nullptr));
        sectionTypeCombo->setItemText(0, QCoreApplication::translate("revolvedialog", "\346\233\262\347\272\277", nullptr));
        sectionTypeCombo->setItemText(1, QCoreApplication::translate("revolvedialog", "\351\235\242\347\211\207", nullptr));

        selectedCountLabel->setText(QCoreApplication::translate("revolvedialog", "\346\230\276\347\244\272\345\267\262\351\200\211\346\213\251\346\225\260\351\207\217", nullptr));
        clearSelectionButton->setText(QCoreApplication::translate("revolvedialog", "\345\217\226\346\266\210\351\200\211\344\270\255", nullptr));
        label->setText(QCoreApplication::translate("revolvedialog", "\350\241\250\345\214\272\345\237\237\347\261\273\345\236\213", nullptr));
        geometrySelectorButton->setText(QCoreApplication::translate("revolvedialog", "\345\207\240\344\275\225\351\200\211\346\213\251\345\231\250", nullptr));
        groupBox_2->setTitle(QCoreApplication::translate("revolvedialog", "\346\226\271\345\220\221", nullptr));
        selectButton->setText(QCoreApplication::translate("revolvedialog", "\351\200\211\346\213\251\347\237\242\351\207\217", nullptr));
        toolButton->setText(QCoreApplication::translate("revolvedialog", "...", nullptr));
        pushButton_2->setText(QCoreApplication::translate("revolvedialog", "\345\217\215\345\220\221", nullptr));
        label_2->setText(QCoreApplication::translate("revolvedialog", "\346\214\207\345\256\232\347\237\242\351\207\217", nullptr));
        originSnapToolButton->setText(QCoreApplication::translate("revolvedialog", "...", nullptr));
        label_5->setText(QCoreApplication::translate("revolvedialog", "\346\214\207\345\256\232\347\202\271", nullptr));
        pushButton->setText(QCoreApplication::translate("revolvedialog", "\346\214\207\345\256\232\347\202\271", nullptr));
        pushButton_3->setText(QCoreApplication::translate("revolvedialog", "\347\237\242\351\207\217\345\257\271\350\257\235\346\241\206", nullptr));
        groupBox_3->setTitle(QCoreApplication::translate("revolvedialog", "\351\231\220\345\210\266", nullptr));
        label_3->setText(QCoreApplication::translate("revolvedialog", "\345\274\200\345\247\213", nullptr));
        startCombo->setItemText(0, QCoreApplication::translate("revolvedialog", "\345\200\274", nullptr));

        startDistanceLabel->setText(QCoreApplication::translate("revolvedialog", "\350\247\222\345\272\246", nullptr));
        label_4->setText(QCoreApplication::translate("revolvedialog", "\347\273\223\346\235\237", nullptr));
        endCombo->setItemText(0, QCoreApplication::translate("revolvedialog", "\345\200\274", nullptr));

        endDistanceLabel->setText(QCoreApplication::translate("revolvedialog", "\350\247\222\345\272\246", nullptr));
        groupBox_4->setTitle(QCoreApplication::translate("revolvedialog", "\347\273\223\346\236\234", nullptr));
        label_7->setText(QCoreApplication::translate("revolvedialog", "\346\233\264\346\226\260\347\273\230\345\210\266\347\273\223\346\236\234", nullptr));
        previewButton->setText(QCoreApplication::translate("revolvedialog", "\351\242\204\350\247\210", nullptr));
        checkBox->setText(QCoreApplication::translate("revolvedialog", "\350\207\252\345\212\250\345\260\201\345\217\243", nullptr));
    } // retranslateUi

};

namespace Ui {
    class revolvedialog: public Ui_revolvedialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_REVOLVEDIALOG_H
