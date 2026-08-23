/********************************************************************************
** Form generated from reading UI file 'expressiondialog.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_EXPRESSIONDIALOG_H
#define UI_EXPRESSIONDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ExpressionDialog
{
public:
    QGroupBox *controlGroup;
    QTableWidget *controlGroupTable;
    QGroupBox *dimensionGroup;
    QGridLayout *gridLayout;
    QLabel *label;
    QLineEdit *expressionNameEdit;
    QLabel *label_2;
    QLineEdit *expressionValueEdit;
    QLabel *label_3;
    QPushButton *addExpressionBtn;
    QLabel *label_4;
    QPushButton *modifyExpressionBtn;
    QWidget *widget;
    QHBoxLayout *horizontalLayout;
    QPushButton *applyBtn;
    QPushButton *okBtn;
    QPushButton *cancelBtn;

    void setupUi(QDialog *ExpressionDialog)
    {
        if (ExpressionDialog->objectName().isEmpty())
            ExpressionDialog->setObjectName("ExpressionDialog");
        ExpressionDialog->resize(554, 521);
        controlGroup = new QGroupBox(ExpressionDialog);
        controlGroup->setObjectName("controlGroup");
        controlGroup->setGeometry(QRect(10, 20, 531, 491));
        controlGroupTable = new QTableWidget(controlGroup);
        if (controlGroupTable->columnCount() < 4)
            controlGroupTable->setColumnCount(4);
        QTableWidgetItem *__qtablewidgetitem = new QTableWidgetItem();
        controlGroupTable->setHorizontalHeaderItem(0, __qtablewidgetitem);
        QTableWidgetItem *__qtablewidgetitem1 = new QTableWidgetItem();
        controlGroupTable->setHorizontalHeaderItem(1, __qtablewidgetitem1);
        QTableWidgetItem *__qtablewidgetitem2 = new QTableWidgetItem();
        controlGroupTable->setHorizontalHeaderItem(2, __qtablewidgetitem2);
        QTableWidgetItem *__qtablewidgetitem3 = new QTableWidgetItem();
        controlGroupTable->setHorizontalHeaderItem(3, __qtablewidgetitem3);
        controlGroupTable->setObjectName("controlGroupTable");
        controlGroupTable->setGeometry(QRect(7, 18, 511, 221));
        dimensionGroup = new QGroupBox(controlGroup);
        dimensionGroup->setObjectName("dimensionGroup");
        dimensionGroup->setGeometry(QRect(7, 254, 511, 191));
        gridLayout = new QGridLayout(dimensionGroup);
        gridLayout->setObjectName("gridLayout");
        label = new QLabel(dimensionGroup);
        label->setObjectName("label");

        gridLayout->addWidget(label, 0, 0, 1, 1);

        expressionNameEdit = new QLineEdit(dimensionGroup);
        expressionNameEdit->setObjectName("expressionNameEdit");

        gridLayout->addWidget(expressionNameEdit, 0, 1, 1, 1);

        label_2 = new QLabel(dimensionGroup);
        label_2->setObjectName("label_2");

        gridLayout->addWidget(label_2, 1, 0, 1, 1);

        expressionValueEdit = new QLineEdit(dimensionGroup);
        expressionValueEdit->setObjectName("expressionValueEdit");

        gridLayout->addWidget(expressionValueEdit, 1, 1, 1, 1);

        label_3 = new QLabel(dimensionGroup);
        label_3->setObjectName("label_3");

        gridLayout->addWidget(label_3, 2, 0, 1, 1);

        addExpressionBtn = new QPushButton(dimensionGroup);
        addExpressionBtn->setObjectName("addExpressionBtn");

        gridLayout->addWidget(addExpressionBtn, 2, 1, 1, 1);

        label_4 = new QLabel(dimensionGroup);
        label_4->setObjectName("label_4");

        gridLayout->addWidget(label_4, 3, 0, 1, 1);

        modifyExpressionBtn = new QPushButton(dimensionGroup);
        modifyExpressionBtn->setObjectName("modifyExpressionBtn");

        gridLayout->addWidget(modifyExpressionBtn, 3, 1, 1, 1);

        widget = new QWidget(controlGroup);
        widget->setObjectName("widget");
        widget->setGeometry(QRect(290, 460, 235, 26));
        horizontalLayout = new QHBoxLayout(widget);
        horizontalLayout->setObjectName("horizontalLayout");
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        applyBtn = new QPushButton(widget);
        applyBtn->setObjectName("applyBtn");

        horizontalLayout->addWidget(applyBtn);

        okBtn = new QPushButton(widget);
        okBtn->setObjectName("okBtn");

        horizontalLayout->addWidget(okBtn);

        cancelBtn = new QPushButton(widget);
        cancelBtn->setObjectName("cancelBtn");

        horizontalLayout->addWidget(cancelBtn);


        retranslateUi(ExpressionDialog);

        QMetaObject::connectSlotsByName(ExpressionDialog);
    } // setupUi

    void retranslateUi(QDialog *ExpressionDialog)
    {
        ExpressionDialog->setWindowTitle(QCoreApplication::translate("ExpressionDialog", "\350\241\250\350\276\276\345\274\217", nullptr));
        controlGroup->setTitle(QCoreApplication::translate("ExpressionDialog", "\346\216\247\344\273\266\347\273\204", nullptr));
        QTableWidgetItem *___qtablewidgetitem = controlGroupTable->horizontalHeaderItem(0);
        ___qtablewidgetitem->setText(QCoreApplication::translate("ExpressionDialog", "\345\220\215\347\247\260", nullptr));
        QTableWidgetItem *___qtablewidgetitem1 = controlGroupTable->horizontalHeaderItem(1);
        ___qtablewidgetitem1->setText(QCoreApplication::translate("ExpressionDialog", "\350\241\250\350\276\276\345\274\217", nullptr));
        QTableWidgetItem *___qtablewidgetitem2 = controlGroupTable->horizontalHeaderItem(2);
        ___qtablewidgetitem2->setText(QCoreApplication::translate("ExpressionDialog", "\350\241\250\350\276\276\345\274\217\345\200\274", nullptr));
        QTableWidgetItem *___qtablewidgetitem3 = controlGroupTable->horizontalHeaderItem(3);
        ___qtablewidgetitem3->setText(QCoreApplication::translate("ExpressionDialog", "\345\205\267\344\275\223\345\206\205\345\256\271", nullptr));
        dimensionGroup->setTitle(QCoreApplication::translate("ExpressionDialog", "\345\260\272\345\257\270\345\261\236\346\200\247", nullptr));
        label->setText(QCoreApplication::translate("ExpressionDialog", "\350\241\250\350\276\276\345\274\217\345\220\215\347\247\260", nullptr));
        label_2->setText(QCoreApplication::translate("ExpressionDialog", "\350\241\250\350\276\276\345\274\217\345\200\274", nullptr));
        label_3->setText(QCoreApplication::translate("ExpressionDialog", "\346\267\273\345\212\240\350\241\250\350\276\276\345\274\217", nullptr));
        addExpressionBtn->setText(QCoreApplication::translate("ExpressionDialog", "\346\267\273\345\212\240\350\241\250\350\276\276\345\274\217", nullptr));
        label_4->setText(QCoreApplication::translate("ExpressionDialog", "\344\277\256\346\224\271\350\241\250\350\276\276\345\274\217", nullptr));
        modifyExpressionBtn->setText(QCoreApplication::translate("ExpressionDialog", "\344\277\256\346\224\271\350\241\250\350\276\276\345\274\217", nullptr));
        applyBtn->setText(QCoreApplication::translate("ExpressionDialog", "\345\272\224\347\224\250", nullptr));
        okBtn->setText(QCoreApplication::translate("ExpressionDialog", "\347\241\256\345\256\232", nullptr));
        cancelBtn->setText(QCoreApplication::translate("ExpressionDialog", "\345\217\226\346\266\210", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ExpressionDialog: public Ui_ExpressionDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_EXPRESSIONDIALOG_H
