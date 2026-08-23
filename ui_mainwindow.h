/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QWidget>
#include "qvtkopenglnativewidget.h"

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QTabWidget *tabWidget;
    QWidget *tab;
    QWidget *tab_2;
    QWidget *tab_3;
    QGroupBox *groupBox;
    QGridLayout *gridLayout;
    QGroupBox *groupBox_10;
    QHBoxLayout *horizontalLayout;
    QPushButton *redoButton;
    QPushButton *undoButton;
    QPushButton *pushButton_31;
    QGroupBox *groupBox_7;
    QGridLayout *gridLayout_6;
    QPushButton *pushButton_15;
    QPushButton *pushButton_16;
    QGroupBox *groupBox_9;
    QGridLayout *gridLayout_8;
    QPushButton *pushButton_18;
    QPushButton *pushButton_20;
    QPushButton *pushButton_22;
    QPushButton *pushButton_19;
    QPushButton *pushButton_21;
    QPushButton *pushButton_23;
    QPushButton *pushButton_24;
    QPushButton *pushButton_25;
    QPushButton *pushButton_26;
    QGroupBox *groupBox_11;
    QGridLayout *gridLayout_10;
    QPushButton *expressionBtn;
    QPushButton *pushButton_30;
    QGroupBox *groupBox_4;
    QGridLayout *gridLayout_3;
    QPushButton *pushButton_4;
    QPushButton *pushButton_5;
    QPushButton *pushButton_6;
    QGroupBox *groupBox_8;
    QGridLayout *gridLayout_7;
    QPushButton *boolOperationButton;
    QGroupBox *groupBox_6;
    QGridLayout *gridLayout_5;
    QPushButton *pushButton_9;
    QPushButton *pushButton_14;
    QPushButton *extrude;
    QPushButton *fillet;
    QPushButton *revolve;
    QPushButton *hollow;
    QGroupBox *groupBox_5;
    QGridLayout *gridLayout_4;
    QPushButton *pushButton_7;
    QPushButton *pushButton_8;
    QPushButton *pushButton_10;
    QPushButton *pushButton_12;
    QPushButton *pushButton_11;
    QPushButton *pushButton_13;
    QGroupBox *groupBox_3;
    QGridLayout *gridLayout_2;
    QPushButton *cuboid;
    QPushButton *sphere;
    QPushButton *cylinder;
    QPushButton *cone;
    QPushButton *pushButton_32;
    QWidget *tab_4;
    QVTKOpenGLNativeWidget *widget_vtkContainer;
    QGroupBox *groupBox_2;
    QListWidget *historyList;
    QPushButton *clear_history;
    QPushButton *showAllButton;
    QTreeWidget *treeWidget;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(800, 600);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        tabWidget = new QTabWidget(centralwidget);
        tabWidget->setObjectName("tabWidget");
        tabWidget->setGeometry(QRect(-10, 0, 1641, 191));
        tab = new QWidget();
        tab->setObjectName("tab");
        tabWidget->addTab(tab, QString());
        tab_2 = new QWidget();
        tab_2->setObjectName("tab_2");
        tabWidget->addTab(tab_2, QString());
        tab_3 = new QWidget();
        tab_3->setObjectName("tab_3");
        groupBox = new QGroupBox(tab_3);
        groupBox->setObjectName("groupBox");
        groupBox->setGeometry(QRect(10, 0, 1611, 151));
        groupBox->setFlat(true);
        gridLayout = new QGridLayout(groupBox);
        gridLayout->setObjectName("gridLayout");
        groupBox_10 = new QGroupBox(groupBox);
        groupBox_10->setObjectName("groupBox_10");
        horizontalLayout = new QHBoxLayout(groupBox_10);
        horizontalLayout->setObjectName("horizontalLayout");
        redoButton = new QPushButton(groupBox_10);
        redoButton->setObjectName("redoButton");

        horizontalLayout->addWidget(redoButton);

        undoButton = new QPushButton(groupBox_10);
        undoButton->setObjectName("undoButton");

        horizontalLayout->addWidget(undoButton);


        gridLayout->addWidget(groupBox_10, 0, 7, 1, 1);

        pushButton_31 = new QPushButton(groupBox);
        pushButton_31->setObjectName("pushButton_31");

        gridLayout->addWidget(pushButton_31, 0, 10, 1, 1);

        groupBox_7 = new QGroupBox(groupBox);
        groupBox_7->setObjectName("groupBox_7");
        gridLayout_6 = new QGridLayout(groupBox_7);
        gridLayout_6->setObjectName("gridLayout_6");
        pushButton_15 = new QPushButton(groupBox_7);
        pushButton_15->setObjectName("pushButton_15");

        gridLayout_6->addWidget(pushButton_15, 0, 0, 1, 1);

        pushButton_16 = new QPushButton(groupBox_7);
        pushButton_16->setObjectName("pushButton_16");

        gridLayout_6->addWidget(pushButton_16, 1, 0, 1, 1);


        gridLayout->addWidget(groupBox_7, 0, 4, 1, 1);

        groupBox_9 = new QGroupBox(groupBox);
        groupBox_9->setObjectName("groupBox_9");
        gridLayout_8 = new QGridLayout(groupBox_9);
        gridLayout_8->setObjectName("gridLayout_8");
        pushButton_18 = new QPushButton(groupBox_9);
        pushButton_18->setObjectName("pushButton_18");

        gridLayout_8->addWidget(pushButton_18, 0, 0, 1, 1);

        pushButton_20 = new QPushButton(groupBox_9);
        pushButton_20->setObjectName("pushButton_20");

        gridLayout_8->addWidget(pushButton_20, 0, 1, 1, 1);

        pushButton_22 = new QPushButton(groupBox_9);
        pushButton_22->setObjectName("pushButton_22");

        gridLayout_8->addWidget(pushButton_22, 0, 2, 1, 1);

        pushButton_19 = new QPushButton(groupBox_9);
        pushButton_19->setObjectName("pushButton_19");

        gridLayout_8->addWidget(pushButton_19, 1, 0, 1, 1);

        pushButton_21 = new QPushButton(groupBox_9);
        pushButton_21->setObjectName("pushButton_21");

        gridLayout_8->addWidget(pushButton_21, 1, 1, 1, 1);

        pushButton_23 = new QPushButton(groupBox_9);
        pushButton_23->setObjectName("pushButton_23");

        gridLayout_8->addWidget(pushButton_23, 1, 2, 1, 1);

        pushButton_24 = new QPushButton(groupBox_9);
        pushButton_24->setObjectName("pushButton_24");

        gridLayout_8->addWidget(pushButton_24, 2, 0, 1, 1);

        pushButton_25 = new QPushButton(groupBox_9);
        pushButton_25->setObjectName("pushButton_25");

        gridLayout_8->addWidget(pushButton_25, 2, 1, 1, 1);

        pushButton_26 = new QPushButton(groupBox_9);
        pushButton_26->setObjectName("pushButton_26");

        gridLayout_8->addWidget(pushButton_26, 2, 2, 1, 1);


        gridLayout->addWidget(groupBox_9, 0, 6, 1, 1);

        groupBox_11 = new QGroupBox(groupBox);
        groupBox_11->setObjectName("groupBox_11");
        gridLayout_10 = new QGridLayout(groupBox_11);
        gridLayout_10->setObjectName("gridLayout_10");
        expressionBtn = new QPushButton(groupBox_11);
        expressionBtn->setObjectName("expressionBtn");

        gridLayout_10->addWidget(expressionBtn, 0, 0, 1, 1);

        pushButton_30 = new QPushButton(groupBox_11);
        pushButton_30->setObjectName("pushButton_30");

        gridLayout_10->addWidget(pushButton_30, 1, 0, 1, 1);


        gridLayout->addWidget(groupBox_11, 0, 8, 1, 1);

        groupBox_4 = new QGroupBox(groupBox);
        groupBox_4->setObjectName("groupBox_4");
        gridLayout_3 = new QGridLayout(groupBox_4);
        gridLayout_3->setObjectName("gridLayout_3");
        pushButton_4 = new QPushButton(groupBox_4);
        pushButton_4->setObjectName("pushButton_4");

        gridLayout_3->addWidget(pushButton_4, 0, 0, 1, 1);

        pushButton_5 = new QPushButton(groupBox_4);
        pushButton_5->setObjectName("pushButton_5");

        gridLayout_3->addWidget(pushButton_5, 1, 0, 1, 1);

        pushButton_6 = new QPushButton(groupBox_4);
        pushButton_6->setObjectName("pushButton_6");

        gridLayout_3->addWidget(pushButton_6, 2, 0, 1, 1);


        gridLayout->addWidget(groupBox_4, 0, 1, 1, 1);

        groupBox_8 = new QGroupBox(groupBox);
        groupBox_8->setObjectName("groupBox_8");
        gridLayout_7 = new QGridLayout(groupBox_8);
        gridLayout_7->setObjectName("gridLayout_7");
        boolOperationButton = new QPushButton(groupBox_8);
        boolOperationButton->setObjectName("boolOperationButton");

        gridLayout_7->addWidget(boolOperationButton, 0, 0, 1, 1);


        gridLayout->addWidget(groupBox_8, 0, 5, 1, 1);

        groupBox_6 = new QGroupBox(groupBox);
        groupBox_6->setObjectName("groupBox_6");
        gridLayout_5 = new QGridLayout(groupBox_6);
        gridLayout_5->setObjectName("gridLayout_5");
        pushButton_9 = new QPushButton(groupBox_6);
        pushButton_9->setObjectName("pushButton_9");

        gridLayout_5->addWidget(pushButton_9, 0, 0, 1, 1);

        pushButton_14 = new QPushButton(groupBox_6);
        pushButton_14->setObjectName("pushButton_14");

        gridLayout_5->addWidget(pushButton_14, 0, 1, 1, 1);

        extrude = new QPushButton(groupBox_6);
        extrude->setObjectName("extrude");

        gridLayout_5->addWidget(extrude, 1, 0, 1, 1);

        fillet = new QPushButton(groupBox_6);
        fillet->setObjectName("fillet");

        gridLayout_5->addWidget(fillet, 1, 1, 1, 1);

        revolve = new QPushButton(groupBox_6);
        revolve->setObjectName("revolve");

        gridLayout_5->addWidget(revolve, 2, 0, 1, 1);

        hollow = new QPushButton(groupBox_6);
        hollow->setObjectName("hollow");

        gridLayout_5->addWidget(hollow, 2, 1, 1, 1);


        gridLayout->addWidget(groupBox_6, 0, 3, 1, 1);

        groupBox_5 = new QGroupBox(groupBox);
        groupBox_5->setObjectName("groupBox_5");
        gridLayout_4 = new QGridLayout(groupBox_5);
        gridLayout_4->setObjectName("gridLayout_4");
        pushButton_7 = new QPushButton(groupBox_5);
        pushButton_7->setObjectName("pushButton_7");

        gridLayout_4->addWidget(pushButton_7, 0, 0, 1, 1);

        pushButton_8 = new QPushButton(groupBox_5);
        pushButton_8->setObjectName("pushButton_8");

        gridLayout_4->addWidget(pushButton_8, 0, 1, 1, 1);

        pushButton_10 = new QPushButton(groupBox_5);
        pushButton_10->setObjectName("pushButton_10");

        gridLayout_4->addWidget(pushButton_10, 1, 0, 1, 1);

        pushButton_12 = new QPushButton(groupBox_5);
        pushButton_12->setObjectName("pushButton_12");

        gridLayout_4->addWidget(pushButton_12, 1, 1, 1, 1);

        pushButton_11 = new QPushButton(groupBox_5);
        pushButton_11->setObjectName("pushButton_11");

        gridLayout_4->addWidget(pushButton_11, 2, 0, 1, 1);

        pushButton_13 = new QPushButton(groupBox_5);
        pushButton_13->setObjectName("pushButton_13");

        gridLayout_4->addWidget(pushButton_13, 2, 1, 1, 1);


        gridLayout->addWidget(groupBox_5, 0, 2, 1, 1);

        groupBox_3 = new QGroupBox(groupBox);
        groupBox_3->setObjectName("groupBox_3");
        groupBox_3->setAutoFillBackground(false);
        groupBox_3->setStyleSheet(QString::fromUtf8("background-color: rgb(243, 243, 243);"));
        gridLayout_2 = new QGridLayout(groupBox_3);
        gridLayout_2->setObjectName("gridLayout_2");
        cuboid = new QPushButton(groupBox_3);
        cuboid->setObjectName("cuboid");
        cuboid->setAutoFillBackground(false);
        cuboid->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);"));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/icons/cuboid_color.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        cuboid->setIcon(icon);
        cuboid->setFlat(true);

        gridLayout_2->addWidget(cuboid, 0, 0, 1, 1);

        sphere = new QPushButton(groupBox_3);
        sphere->setObjectName("sphere");
        sphere->setAutoFillBackground(false);
        sphere->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);"));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/icons/sphere_color.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        sphere->setIcon(icon1);
        sphere->setFlat(true);

        gridLayout_2->addWidget(sphere, 0, 1, 1, 1);

        cylinder = new QPushButton(groupBox_3);
        cylinder->setObjectName("cylinder");
        cylinder->setAutoFillBackground(false);
        cylinder->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);"));
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/icons/cylinder_color.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        cylinder->setIcon(icon2);
        cylinder->setFlat(true);

        gridLayout_2->addWidget(cylinder, 1, 0, 1, 1);

        cone = new QPushButton(groupBox_3);
        cone->setObjectName("cone");
        cone->setAutoFillBackground(false);
        cone->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);"));
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/icons/cone_color.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        cone->setIcon(icon3);
        cone->setFlat(true);

        gridLayout_2->addWidget(cone, 2, 0, 1, 1);


        gridLayout->addWidget(groupBox_3, 0, 0, 1, 1);

        pushButton_32 = new QPushButton(groupBox);
        pushButton_32->setObjectName("pushButton_32");

        gridLayout->addWidget(pushButton_32, 0, 9, 1, 1);

        tabWidget->addTab(tab_3, QString());
        tab_4 = new QWidget();
        tab_4->setObjectName("tab_4");
        tabWidget->addTab(tab_4, QString());
        widget_vtkContainer = new QVTKOpenGLNativeWidget(centralwidget);
        widget_vtkContainer->setObjectName("widget_vtkContainer");
        widget_vtkContainer->setGeometry(QRect(200, 220, 1421, 751));
        groupBox_2 = new QGroupBox(centralwidget);
        groupBox_2->setObjectName("groupBox_2");
        groupBox_2->setGeometry(QRect(-20, 210, 211, 761));
        historyList = new QListWidget(groupBox_2);
        historyList->setObjectName("historyList");
        historyList->setGeometry(QRect(0, 270, 211, 481));
        clear_history = new QPushButton(groupBox_2);
        clear_history->setObjectName("clear_history");
        clear_history->setGeometry(QRect(10, 700, 81, 31));
        showAllButton = new QPushButton(groupBox_2);
        showAllButton->setObjectName("showAllButton");
        showAllButton->setGeometry(QRect(110, 700, 81, 31));
        treeWidget = new QTreeWidget(groupBox_2);
        QTreeWidgetItem *__qtreewidgetitem = new QTreeWidgetItem(treeWidget);
        __qtreewidgetitem->setTextAlignment(0, Qt::AlignLeading|Qt::AlignVCenter);
        QTreeWidgetItem *__qtreewidgetitem1 = new QTreeWidgetItem(treeWidget);
        new QTreeWidgetItem(__qtreewidgetitem1);
        new QTreeWidgetItem(__qtreewidgetitem1);
        new QTreeWidgetItem(__qtreewidgetitem1);
        new QTreeWidgetItem(__qtreewidgetitem1);
        new QTreeWidgetItem(__qtreewidgetitem1);
        new QTreeWidgetItem(__qtreewidgetitem1);
        new QTreeWidgetItem(__qtreewidgetitem1);
        new QTreeWidgetItem(__qtreewidgetitem1);
        QTreeWidgetItem *__qtreewidgetitem2 = new QTreeWidgetItem(treeWidget);
        new QTreeWidgetItem(__qtreewidgetitem2);
        new QTreeWidgetItem(__qtreewidgetitem2);
        new QTreeWidgetItem(__qtreewidgetitem2);
        new QTreeWidgetItem(__qtreewidgetitem2);
        new QTreeWidgetItem(__qtreewidgetitem2);
        new QTreeWidgetItem(__qtreewidgetitem2);
        new QTreeWidgetItem(__qtreewidgetitem2);
        new QTreeWidgetItem(__qtreewidgetitem2);
        new QTreeWidgetItem(treeWidget);
        treeWidget->setObjectName("treeWidget");
        treeWidget->setGeometry(QRect(0, 20, 211, 241));
        treeWidget->header()->setDefaultSectionSize(105);
        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 800, 18));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tab), QCoreApplication::translate("MainWindow", "\346\226\207\344\273\266", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tab_2), QCoreApplication::translate("MainWindow", "\350\247\206\345\233\276", nullptr));
        groupBox->setTitle(QString());
        groupBox_10->setTitle(QCoreApplication::translate("MainWindow", "\345\216\206\345\217\262\346\265\201", nullptr));
        redoButton->setText(QCoreApplication::translate("MainWindow", "\351\207\215\345\201\232", nullptr));
        undoButton->setText(QCoreApplication::translate("MainWindow", "\346\222\244\351\224\200", nullptr));
        pushButton_31->setText(QCoreApplication::translate("MainWindow", "\344\277\235\345\255\230\350\257\273\345\217\226", nullptr));
        groupBox_7->setTitle(QCoreApplication::translate("MainWindow", "\350\256\276\350\256\241\347\211\271\345\276\201", nullptr));
        pushButton_15->setText(QCoreApplication::translate("MainWindow", "\347\274\235\345\220\210", nullptr));
        pushButton_16->setText(QCoreApplication::translate("MainWindow", "\345\210\207\345\210\206\344\275\223", nullptr));
        groupBox_9->setTitle(QCoreApplication::translate("MainWindow", "\346\233\262\351\235\242\347\211\271\345\276\201", nullptr));
        pushButton_18->setText(QCoreApplication::translate("MainWindow", "4\347\202\271\346\233\262\351\235\242", nullptr));
        pushButton_20->setText(QCoreApplication::translate("MainWindow", "\345\273\266\347\224\263\346\233\262\351\235\242", nullptr));
        pushButton_22->setText(QCoreApplication::translate("MainWindow", "\350\243\201\345\210\207\346\233\262\351\235\242", nullptr));
        pushButton_19->setText(QCoreApplication::translate("MainWindow", "\345\201\217\347\275\256\346\233\262\351\235\242", nullptr));
        pushButton_21->setText(QCoreApplication::translate("MainWindow", "\345\273\266\347\224\263\346\233\262\351\235\2422", nullptr));
        pushButton_23->setText(QCoreApplication::translate("MainWindow", "\346\213\206\345\210\206\346\233\262\351\235\242", nullptr));
        pushButton_24->setText(QCoreApplication::translate("MainWindow", "\351\235\242\345\200\222\345\234\206", nullptr));
        pushButton_25->setText(QCoreApplication::translate("MainWindow", "\351\235\242\346\233\277\346\215\242", nullptr));
        pushButton_26->setText(QCoreApplication::translate("MainWindow", "\346\217\220\345\217\226\344\270\255\351\235\242", nullptr));
        groupBox_11->setTitle(QCoreApplication::translate("MainWindow", "\345\267\245\345\205\267", nullptr));
        expressionBtn->setText(QCoreApplication::translate("MainWindow", "\350\241\250\350\276\276\345\274\217", nullptr));
        pushButton_30->setText(QCoreApplication::translate("MainWindow", "\347\247\273\345\212\250\345\257\271\350\261\241", nullptr));
        groupBox_4->setTitle(QCoreApplication::translate("MainWindow", "\345\237\272\345\207\206\347\211\271\345\276\201", nullptr));
        pushButton_4->setText(QCoreApplication::translate("MainWindow", "\345\237\272\345\207\206\350\275\264", nullptr));
        pushButton_5->setText(QCoreApplication::translate("MainWindow", "\345\237\272\345\207\206\345\235\220\346\240\207\347\263\273", nullptr));
        pushButton_6->setText(QCoreApplication::translate("MainWindow", "\345\237\272\345\207\206\345\271\263\351\235\242", nullptr));
        groupBox_8->setTitle(QCoreApplication::translate("MainWindow", "\345\207\240\344\275\225\350\277\220\347\256\227", nullptr));
        boolOperationButton->setText(QCoreApplication::translate("MainWindow", "\345\270\203\345\260\224\350\277\220\347\256\227", nullptr));
        groupBox_6->setTitle(QCoreApplication::translate("MainWindow", "\347\273\206\350\212\202\347\211\271\345\276\201", nullptr));
        pushButton_9->setText(QCoreApplication::translate("MainWindow", "\350\243\201\345\210\207\346\233\262\347\272\277", nullptr));
        pushButton_14->setText(QCoreApplication::translate("MainWindow", "\345\200\222\345\234\206\350\247\222", nullptr));
        extrude->setText(QCoreApplication::translate("MainWindow", "\346\213\211\344\274\270", nullptr));
        fillet->setText(QCoreApplication::translate("MainWindow", "\345\200\222\350\247\222", nullptr));
        revolve->setText(QCoreApplication::translate("MainWindow", "\346\227\213\350\275\254", nullptr));
        hollow->setText(QCoreApplication::translate("MainWindow", "\346\214\226\347\251\272", nullptr));
        groupBox_5->setTitle(QCoreApplication::translate("MainWindow", "\346\233\262\347\272\277\347\211\271\345\276\201", nullptr));
        pushButton_7->setText(QCoreApplication::translate("MainWindow", "\347\233\264\347\272\277", nullptr));
        pushButton_8->setText(QCoreApplication::translate("MainWindow", "\346\212\233\347\211\251\347\272\277", nullptr));
        pushButton_10->setText(QCoreApplication::translate("MainWindow", "\347\233\264\347\272\2772", nullptr));
        pushButton_12->setText(QCoreApplication::translate("MainWindow", "\346\212\225\345\275\261\346\233\262\347\272\277", nullptr));
        pushButton_11->setText(QCoreApplication::translate("MainWindow", "\345\234\206\345\274\247", nullptr));
        pushButton_13->setText(QCoreApplication::translate("MainWindow", "\345\201\217\347\275\256\346\233\262\347\272\277", nullptr));
        groupBox_3->setTitle(QCoreApplication::translate("MainWindow", "\345\237\272\346\234\254\345\273\272\346\250\241\347\211\271\345\276\201", nullptr));
        cuboid->setText(QCoreApplication::translate("MainWindow", "\351\225\277\346\226\271\344\275\223", nullptr));
        sphere->setText(QCoreApplication::translate("MainWindow", "\347\220\203\344\275\223", nullptr));
        cylinder->setText(QCoreApplication::translate("MainWindow", "\345\234\206\346\237\261\344\275\223", nullptr));
        cone->setText(QCoreApplication::translate("MainWindow", "\345\234\206\351\224\245", nullptr));
        pushButton_32->setText(QCoreApplication::translate("MainWindow", "\345\210\206\346\236\220", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tab_3), QCoreApplication::translate("MainWindow", "\345\273\272\346\250\241", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tab_4), QCoreApplication::translate("MainWindow", "\350\215\211\345\233\276", nullptr));
        groupBox_2->setTitle(QCoreApplication::translate("MainWindow", "\345\273\272\346\250\241\345\216\206\345\217\262", nullptr));
        clear_history->setText(QCoreApplication::translate("MainWindow", "\346\270\205\347\251\272", nullptr));
        showAllButton->setText(QCoreApplication::translate("MainWindow", "\346\230\276\347\244\272\346\211\200\346\234\211", nullptr));
        QTreeWidgetItem *___qtreewidgetitem = treeWidget->headerItem();
        ___qtreewidgetitem->setText(1, QCoreApplication::translate("MainWindow", "\345\217\257\350\247\201\346\200\247", nullptr));
        ___qtreewidgetitem->setText(0, QCoreApplication::translate("MainWindow", "\345\220\215\347\247\260", nullptr));

        const bool __sortingEnabled = treeWidget->isSortingEnabled();
        treeWidget->setSortingEnabled(false);
        QTreeWidgetItem *___qtreewidgetitem1 = treeWidget->topLevelItem(0);
        ___qtreewidgetitem1->setText(0, QCoreApplication::translate("MainWindow", "\345\216\206\345\217\262\350\256\260\345\275\225\345\273\272\346\250\241\346\250\241\345\274\217", nullptr));
        QTreeWidgetItem *___qtreewidgetitem2 = treeWidget->topLevelItem(1);
        ___qtreewidgetitem2->setText(0, QCoreApplication::translate("MainWindow", "\346\250\241\345\236\213\350\247\206\345\233\276", nullptr));
        QTreeWidgetItem *___qtreewidgetitem3 = ___qtreewidgetitem2->child(0);
        ___qtreewidgetitem3->setText(0, QCoreApplication::translate("MainWindow", "\344\273\260\350\247\206\345\233\276", nullptr));
        QTreeWidgetItem *___qtreewidgetitem4 = ___qtreewidgetitem2->child(1);
        ___qtreewidgetitem4->setText(0, QCoreApplication::translate("MainWindow", "\344\277\257\350\247\206\345\233\276", nullptr));
        QTreeWidgetItem *___qtreewidgetitem5 = ___qtreewidgetitem2->child(2);
        ___qtreewidgetitem5->setText(0, QCoreApplication::translate("MainWindow", "\345\211\215\350\247\206\345\233\276", nullptr));
        QTreeWidgetItem *___qtreewidgetitem6 = ___qtreewidgetitem2->child(3);
        ___qtreewidgetitem6->setText(0, QCoreApplication::translate("MainWindow", "\345\217\263\350\247\206\345\233\276", nullptr));
        QTreeWidgetItem *___qtreewidgetitem7 = ___qtreewidgetitem2->child(4);
        ___qtreewidgetitem7->setText(0, QCoreApplication::translate("MainWindow", "\345\220\216\350\247\206\345\233\276", nullptr));
        QTreeWidgetItem *___qtreewidgetitem8 = ___qtreewidgetitem2->child(5);
        ___qtreewidgetitem8->setText(0, QCoreApplication::translate("MainWindow", "\345\267\246\350\247\206\345\233\276", nullptr));
        QTreeWidgetItem *___qtreewidgetitem9 = ___qtreewidgetitem2->child(6);
        ___qtreewidgetitem9->setText(0, QCoreApplication::translate("MainWindow", "\346\255\243\344\270\211\350\275\264\346\265\213\350\247\206\345\233\276", nullptr));
        QTreeWidgetItem *___qtreewidgetitem10 = ___qtreewidgetitem2->child(7);
        ___qtreewidgetitem10->setText(0, QCoreApplication::translate("MainWindow", "\346\255\243\347\255\211\346\265\213\345\233\276", nullptr));
        QTreeWidgetItem *___qtreewidgetitem11 = treeWidget->topLevelItem(2);
        ___qtreewidgetitem11->setText(0, QCoreApplication::translate("MainWindow", "\346\221\204\345\203\217\346\234\272", nullptr));
        QTreeWidgetItem *___qtreewidgetitem12 = ___qtreewidgetitem11->child(0);
        ___qtreewidgetitem12->setText(0, QCoreApplication::translate("MainWindow", "\344\273\260\350\247\206\345\233\276", nullptr));
        QTreeWidgetItem *___qtreewidgetitem13 = ___qtreewidgetitem11->child(1);
        ___qtreewidgetitem13->setText(0, QCoreApplication::translate("MainWindow", "\344\277\257\350\247\206\345\233\276", nullptr));
        QTreeWidgetItem *___qtreewidgetitem14 = ___qtreewidgetitem11->child(2);
        ___qtreewidgetitem14->setText(0, QCoreApplication::translate("MainWindow", "\345\211\215\350\247\206\345\233\276", nullptr));
        QTreeWidgetItem *___qtreewidgetitem15 = ___qtreewidgetitem11->child(3);
        ___qtreewidgetitem15->setText(0, QCoreApplication::translate("MainWindow", "\345\217\263\350\247\206\345\233\276", nullptr));
        QTreeWidgetItem *___qtreewidgetitem16 = ___qtreewidgetitem11->child(4);
        ___qtreewidgetitem16->setText(0, QCoreApplication::translate("MainWindow", "\345\220\216\350\247\206\345\233\276", nullptr));
        QTreeWidgetItem *___qtreewidgetitem17 = ___qtreewidgetitem11->child(5);
        ___qtreewidgetitem17->setText(0, QCoreApplication::translate("MainWindow", "\345\267\246\350\247\206\345\233\276", nullptr));
        QTreeWidgetItem *___qtreewidgetitem18 = ___qtreewidgetitem11->child(6);
        ___qtreewidgetitem18->setText(0, QCoreApplication::translate("MainWindow", "\346\255\243\344\270\211\350\275\264\344\276\247\350\247\206\345\233\276", nullptr));
        QTreeWidgetItem *___qtreewidgetitem19 = ___qtreewidgetitem11->child(7);
        ___qtreewidgetitem19->setText(0, QCoreApplication::translate("MainWindow", "\346\255\243\347\255\211\346\265\213\345\233\276", nullptr));
        QTreeWidgetItem *___qtreewidgetitem20 = treeWidget->topLevelItem(3);
        ___qtreewidgetitem20->setText(0, QCoreApplication::translate("MainWindow", "\346\250\241\345\236\213\345\216\206\345\217\262", nullptr));
        treeWidget->setSortingEnabled(__sortingEnabled);

    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
