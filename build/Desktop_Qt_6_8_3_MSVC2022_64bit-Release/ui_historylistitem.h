/********************************************************************************
** Form generated from reading UI file 'historylistitem.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_HISTORYLISTITEM_H
#define UI_HISTORYLISTITEM_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>

QT_BEGIN_NAMESPACE

class Ui_HistoryListItem
{
public:

    void setupUi(QDialog *HistoryListItem)
    {
        if (HistoryListItem->objectName().isEmpty())
            HistoryListItem->setObjectName("HistoryListItem");
        HistoryListItem->resize(400, 300);

        retranslateUi(HistoryListItem);

        QMetaObject::connectSlotsByName(HistoryListItem);
    } // setupUi

    void retranslateUi(QDialog *HistoryListItem)
    {
        HistoryListItem->setWindowTitle(QCoreApplication::translate("HistoryListItem", "Dialog", nullptr));
    } // retranslateUi

};

namespace Ui {
    class HistoryListItem: public Ui_HistoryListItem {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_HISTORYLISTITEM_H
