#include "widget.h"
#include "handle_spec.h"

#include <QApplication>
#include <QPalette>
#include <QColor>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    // 使用 Fusion 保证不同控件外观一致（后续再叠加 QSS 细节）
    a.setStyle(QStyleFactory::create("Fusion"));

    // CAD 浅色主题：石板灰底 + 青绿强调色（与特征对话框标题色系一致）
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(241, 244, 247));
    palette.setColor(QPalette::WindowText, QColor(36, 42, 48));
    palette.setColor(QPalette::Base, QColor(255, 255, 255));
    palette.setColor(QPalette::AlternateBase, QColor(246, 248, 250));
    palette.setColor(QPalette::Text, QColor(36, 42, 48));
    palette.setColor(QPalette::Button, QColor(241, 244, 247));
    palette.setColor(QPalette::ButtonText, QColor(36, 42, 48));
    palette.setColor(QPalette::Highlight, QColor(42, 168, 154));
    palette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
    palette.setColor(QPalette::ToolTipBase, QColor(255, 255, 255));
    palette.setColor(QPalette::ToolTipText, QColor(36, 42, 48));
    palette.setColor(QPalette::Link, QColor(44, 95, 143));
    a.setPalette(palette);

    // 全局 QSS：覆盖主窗口、所有对话框、消息框、输入框等控件样式
    a.setStyleSheet(R"(
        /* Base */
        QWidget {
            font-family: "Segoe UI", "Microsoft YaHei UI", "Microsoft YaHei", "PingFang SC", sans-serif;
            font-size: 12px;
            color: #242a30;
        }

        QToolTip {
            background: #ffffff;
            color: #242a30;
            border: 1px solid #cfd8e3;
            border-radius: 6px;
            padding: 6px 8px;
        }

        /* Main window / docks */
        QMainWindow {
            background: #f1f4f7;
        }
        QMenuBar {
            background: #e8eef3;
            border-bottom: 1px solid #d5dee8;
            padding: 2px 4px;
            spacing: 2px;
        }
        QMenuBar::item {
            background: transparent;
            padding: 6px 10px;
            border-radius: 6px;
        }
        QMenuBar::item:selected {
            background: rgba(42, 168, 154, 0.16);
        }
        QMenu {
            background: #ffffff;
            border: 1px solid #d5dee8;
            border-radius: 8px;
            padding: 6px;
        }
        QMenu::item {
            padding: 7px 28px 7px 12px;
            border-radius: 6px;
        }
        QMenu::item:selected {
            background: rgba(42, 168, 154, 0.16);
        }
        QMenu::separator {
            height: 1px;
            background: #e4ebf2;
            margin: 4px 8px;
        }

        QToolBar {
            background: #eef3f7;
            border: none;
            border-bottom: 1px solid #d5dee8;
            spacing: 4px;
            padding: 4px 6px;
        }
        /* 勿写全局 QToolButton/QTabBar：会打到 SARibbon（布局错乱、文字重叠） */
        QDialog QToolButton, QDockWidget QToolButton, QWidget#titleBar QToolButton {
            background: transparent;
            border: 1px solid transparent;
            border-radius: 8px;
            padding: 5px;
            min-width: 28px;
            min-height: 28px;
        }
        QDialog QToolButton:hover, QDockWidget QToolButton:hover {
            background: rgba(42, 168, 154, 0.12);
            border-color: rgba(42, 168, 154, 0.28);
        }
        QDialog QToolButton:pressed, QDialog QToolButton:checked,
        QDockWidget QToolButton:pressed, QDockWidget QToolButton:checked {
            background: rgba(44, 95, 143, 0.18);
            border-color: rgba(44, 95, 143, 0.35);
        }

        QDockWidget {
            titlebar-close-icon: none;
        }
        QDockWidget::title {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                        stop:0 #eef3f7, stop:1 #e4ebf2);
            border: 1px solid #d5dee8;
            padding: 7px 10px;
            border-radius: 8px;
            font-weight: 600;
            color: #2c5f8f;
        }

        /* GroupBox */
        QGroupBox {
            border: 1px solid #d5dee8;
            border-radius: 10px;
            margin-top: 10px;
            padding: 10px 8px 8px 8px;
            background: #ffffff;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 12px;
            padding: 0 8px;
            color: #2aa89a;
            font-weight: 700;
            font-size: 13px;
        }

        /* Buttons */
        QPushButton {
            background: #f4f7fa;
            border: 1px solid #cfd8e3;
            border-radius: 8px;
            padding: 6px 14px;
            min-height: 22px;
        }
        QPushButton:hover {
            background: #e8f7f4;
            border-color: #7bcfc4;
        }
        QPushButton:pressed {
            background: #d5efe9;
            border-color: #2aa89a;
        }
        QPushButton:disabled {
            color: #9aa6b2;
            background: #f0f3f6;
            border-color: #e0e6ec;
        }
        QPushButton[flat="true"] {
            background: transparent;
            border: none;
            padding: 2px 4px;
        }
        QPushButton[flat="true"]:hover {
            background: rgba(42, 168, 154, 0.10);
            border-radius: 6px;
        }

        /* 自定义标题栏按钮（SketchToolInputDialog） */
        QWidget#titleBar QToolButton {
            color: #ffffff;
            font-size: 16px;
            border: none;
            min-width: 24px;
            min-height: 24px;
            background: transparent;
        }

        /* Inputs */
        QLineEdit, QTextEdit, QPlainTextEdit, QComboBox, QSpinBox, QDoubleSpinBox {
            background: #ffffff;
            border: 1px solid #cfd8e3;
            border-radius: 8px;
            padding: 5px 8px;
            selection-background-color: #2aa89a;
            selection-color: #ffffff;
        }
        QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus,
        QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus {
            border-color: #2aa89a;
        }
        QComboBox::drop-down {
            border: none;
            width: 22px;
        }
        QComboBox QAbstractItemView {
            background: #ffffff;
            border: 1px solid #d5dee8;
            selection-background-color: rgba(42, 168, 154, 0.22);
            selection-color: #242a30;
            outline: none;
        }

        /* Tabs（仅普通 QTabWidget；排除 SARibbonTabBar） */
        QTabWidget::pane {
            border: 1px solid #d5dee8;
            border-radius: 10px;
            top: -1px;
            background: #ffffff;
        }
        QTabWidget > QTabBar::tab {
            background: #e8eef3;
            border: 1px solid #d5dee8;
            border-bottom: none;
            padding: 8px 14px;
            border-top-left-radius: 10px;
            border-top-right-radius: 10px;
            margin-right: 3px;
            color: #5a6775;
        }
        QTabWidget > QTabBar::tab:selected {
            background: #ffffff;
            color: #2c5f8f;
            font-weight: 700;
        }
        QTabWidget > QTabBar::tab:hover:!selected {
            background: #f2f6f9;
            color: #2aa89a;
        }

        /* Tree / lists */
        QTreeWidget, QListWidget, QTableWidget {
            background: #ffffff;
            border: 1px solid #d5dee8;
            border-radius: 10px;
            outline: none;
            alternate-background-color: #f7fafc;
        }
        QTreeWidget::item, QListWidget::item {
            padding: 5px 8px;
            border-radius: 4px;
            min-height: 22px;
        }
        QTreeWidget::item:hover, QListWidget::item:hover {
            background: rgba(42, 168, 154, 0.10);
        }
        QTreeWidget::item:selected, QListWidget::item:selected {
            background: #2aa89a;
            color: #ffffff;
        }
        QHeaderView::section {
            background: #eef3f7;
            border: 1px solid #d5dee8;
            padding: 6px 8px;
            font-weight: 600;
            color: #2c5f8f;
        }

        /* Scrollbars */
        QScrollBar:vertical {
            background: transparent;
            width: 10px;
            margin: 2px;
        }
        QScrollBar::handle:vertical {
            background: #c5d0db;
            border-radius: 5px;
            min-height: 28px;
        }
        QScrollBar::handle:vertical:hover {
            background: #2aa89a;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0;
        }
        QScrollBar:horizontal {
            background: transparent;
            height: 10px;
            margin: 2px;
        }
        QScrollBar::handle:horizontal {
            background: #c5d0db;
            border-radius: 5px;
            min-width: 28px;
        }
        QScrollBar::handle:horizontal:hover {
            background: #2aa89a;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0;
        }

        /* Dialogs / message boxes */
        QDialog {
            background: #f7f9fb;
        }
        QMessageBox {
            background: #ffffff;
        }

        /* Status bar */
        QStatusBar {
            background: #e8eef3;
            border-top: 1px solid #d5dee8;
            color: #5a6775;
        }
        QStatusBar::item {
            border: none;
        }

        /* Check / radio */
        QCheckBox, QRadioButton {
            spacing: 8px;
        }
        QCheckBox::indicator, QRadioButton::indicator {
            width: 15px;
            height: 15px;
        }
    )");

    // 注册所有操作柄规格（COND/CR/GR 字段，在 Widget 创建前完成）
    registerAllHandleSpecs();

    Widget w;
    w.show();
    return a.exec();
}
