#include "expression_dialog.h"
#include "ui_expression_dialog.h"
#include "main_window.h"  // 包含主窗口头文件以访问模型数据
#include <QTableWidgetItem>

ExpressionDialog::ExpressionDialog(Widget *parent) :
    QDialog(parent),
    ui(new Ui::ExpressionDialog),
    parentWidget(parent)
{
    ui->setupUi(this);

    // 设置窗口属性
    setWindowTitle("表达式");

    // 初始化表格列宽
    ui->controlGroupTable->setColumnWidth(0, 80);   // 名称列
    ui->controlGroupTable->setColumnWidth(1, 150);  // 表达式列
    ui->controlGroupTable->setColumnWidth(2, 100);  // 表达式值列
    // 具体内容列自动拉伸

    // 初始禁用修改按钮
    ui->modifyExpressionBtn->setEnabled(false);

    // 更新模型参数显示
    updateModelParameters();
}

ExpressionDialog::~ExpressionDialog()
{
    delete ui;
}

void ExpressionDialog::updateModelParameters()
{
    updateControlGroupTable();
}

void ExpressionDialog::updateControlGroupTable()
{
    if (!parentWidget) return;

    // 获取主窗口的历史记录
    const QList<ModelingHistory>& historyList = parentWidget->getHistoryList();

    // 计算总参数数量
    int totalParams = 0;
    for (const ModelingHistory& history : historyList) {
        if (history.type == CUBOID) {
            totalParams += 3; // 长方体有长宽高3个参数
        } else if (history.type == CYLINDER) {
            totalParams += 2; // 圆柱体有半径和高度2个参数
        } else if (history.type == CONE) {
            totalParams += 2; // 圆锥体有半径和高度2个参数
        } else if (history.type == SPHERE) {
            totalParams += 1; // 球体有半径1个参数
        }
        // 对于其他类型（布尔运算、拉伸等），暂时不显示参数
    }

    ui->controlGroupTable->setRowCount(totalParams);

    int row = 0;
    for (int i = 0; i < historyList.size(); ++i) {
        const ModelingHistory& history = historyList[i];
        QString modelName = history.name;

        if (history.type == CUBOID) {
            // 长方体参数
            ui->controlGroupTable->setItem(row, 0, new QTableWidgetItem(QString("%1_x").arg(modelName)));
            ui->controlGroupTable->setItem(row, 1, new QTableWidgetItem(QString("%1_length = %2").arg(modelName).arg(history.param1)));
            ui->controlGroupTable->setItem(row, 2, new QTableWidgetItem(QString::number(history.param1)));
            ui->controlGroupTable->setItem(row, 3, new QTableWidgetItem("长方体长度"));
            row++;

            ui->controlGroupTable->setItem(row, 0, new QTableWidgetItem(QString("%1_y").arg(modelName)));
            ui->controlGroupTable->setItem(row, 1, new QTableWidgetItem(QString("%1_width = %2").arg(modelName).arg(history.param2)));
            ui->controlGroupTable->setItem(row, 2, new QTableWidgetItem(QString::number(history.param2)));
            ui->controlGroupTable->setItem(row, 3, new QTableWidgetItem("长方体宽度"));
            row++;

            ui->controlGroupTable->setItem(row, 0, new QTableWidgetItem(QString("%1_z").arg(modelName)));
            ui->controlGroupTable->setItem(row, 1, new QTableWidgetItem(QString("%1_height = %2").arg(modelName).arg(history.param3)));
            ui->controlGroupTable->setItem(row, 2, new QTableWidgetItem(QString::number(history.param3)));
            ui->controlGroupTable->setItem(row, 3, new QTableWidgetItem("长方体高度"));
            row++;

        } else if (history.type == CYLINDER) {
            // 圆柱体参数
            ui->controlGroupTable->setItem(row, 0, new QTableWidgetItem(QString("%1_r").arg(modelName)));
            ui->controlGroupTable->setItem(row, 1, new QTableWidgetItem(QString("%1_radius = %2").arg(modelName).arg(history.param1)));
            ui->controlGroupTable->setItem(row, 2, new QTableWidgetItem(QString::number(history.param1)));
            ui->controlGroupTable->setItem(row, 3, new QTableWidgetItem("圆柱体半径"));
            row++;

            ui->controlGroupTable->setItem(row, 0, new QTableWidgetItem(QString("%1_h").arg(modelName)));
            ui->controlGroupTable->setItem(row, 1, new QTableWidgetItem(QString("%1_height = %2").arg(modelName).arg(history.param2)));
            ui->controlGroupTable->setItem(row, 2, new QTableWidgetItem(QString::number(history.param2)));
            ui->controlGroupTable->setItem(row, 3, new QTableWidgetItem("圆柱体高度"));
            row++;

        } else if (history.type == CONE) {
            // 圆锥体参数
            ui->controlGroupTable->setItem(row, 0, new QTableWidgetItem(QString("%1_r").arg(modelName)));
            ui->controlGroupTable->setItem(row, 1, new QTableWidgetItem(QString("%1_radius = %2").arg(modelName).arg(history.param1)));
            ui->controlGroupTable->setItem(row, 2, new QTableWidgetItem(QString::number(history.param1)));
            ui->controlGroupTable->setItem(row, 3, new QTableWidgetItem("圆锥体半径"));
            row++;

            ui->controlGroupTable->setItem(row, 0, new QTableWidgetItem(QString("%1_h").arg(modelName)));
            ui->controlGroupTable->setItem(row, 1, new QTableWidgetItem(QString("%1_height = %2").arg(modelName).arg(history.param2)));
            ui->controlGroupTable->setItem(row, 2, new QTableWidgetItem(QString::number(history.param2)));
            ui->controlGroupTable->setItem(row, 3, new QTableWidgetItem("圆锥体高度"));
            row++;

        } else if (history.type == SPHERE) {
            // 球体参数
            ui->controlGroupTable->setItem(row, 0, new QTableWidgetItem(QString("%1_r").arg(modelName)));
            ui->controlGroupTable->setItem(row, 1, new QTableWidgetItem(QString("%1_radius = %2").arg(modelName).arg(history.param1)));
            ui->controlGroupTable->setItem(row, 2, new QTableWidgetItem(QString::number(history.param1)));
            ui->controlGroupTable->setItem(row, 3, new QTableWidgetItem("球体半径"));
            row++;
        }
    }
}

void ExpressionDialog::on_addExpressionBtn_clicked()
{
    // 添加自定义表达式功能（可选）
    QString name = ui->expressionNameEdit->text().trimmed();
    QString value = ui->expressionValueEdit->text().trimmed();

    if (name.isEmpty() || value.isEmpty()) {
        QMessageBox::warning(this, "警告", "请输入表达式名称和值！");
        return;
    }

    // 这里可以添加自定义表达式的逻辑
    // 暂时简单处理
    Q_UNUSED(name);
    Q_UNUSED(value);

    // 清空输入框
    ui->expressionNameEdit->clear();
    ui->expressionValueEdit->clear();
}

void ExpressionDialog::on_applyBtn_clicked()
{
    // 检查是否有选中的参数需要应用
    int currentRow = ui->controlGroupTable->currentRow();
    if (currentRow >= 0) {
        // 如果有选中的参数，应用当前输入框中的值
        on_modifyExpressionBtn_clicked();
    } else {
        // 如果没有选中的参数，只是简单确认
        QMessageBox::information(this, "应用", "参数已应用！");
    }
}

void ExpressionDialog::on_okBtn_clicked()
{
    // 先应用当前修改（如果有的话）
    on_applyBtn_clicked();
    // 然后关闭对话框
    accept();
}

void ExpressionDialog::on_modifyExpressionBtn_clicked()
{
    int currentRow = ui->controlGroupTable->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "警告", "请先选择要修改的参数！");
        return;
    }

    QString paramName = ui->expressionNameEdit->text().trimmed();
    QString newValueStr = ui->expressionValueEdit->text().trimmed();

    if (paramName.isEmpty() || newValueStr.isEmpty()) {
        QMessageBox::warning(this, "警告", "参数名称和值不能为空！");
        return;
    }

    // 转换数值
    bool ok;
    double newValue = newValueStr.toDouble(&ok);
    if (!ok) {
        QMessageBox::warning(this, "错误", "请输入有效的数值！");
        return;
    }

    if (newValue <= 0) {
        QMessageBox::warning(this, "错误", "参数值必须大于0！");
        return;
    }

    // 通知主窗口更新模型参数
    if (parentWidget) {
        parentWidget->updateModelParameter(paramName, newValue);
    } else {
        QMessageBox::warning(this, "错误", "无法访问主窗口！");
        return;
    }

    // 关键：更新表格显示，确保显示新的参数值
    updateControlGroupTable();

    // 清空输入框并禁用修改按钮
    ui->expressionNameEdit->clear();
    ui->expressionValueEdit->clear();
    ui->modifyExpressionBtn->setEnabled(false);

    QMessageBox::information(this, "成功", "参数修改成功！");
}

void ExpressionDialog::on_cancelBtn_clicked()
{
    reject();
}

void ExpressionDialog::on_controlGroupTable_itemSelectionChanged()
{
    int currentRow = ui->controlGroupTable->currentRow();
    if (currentRow >= 0) {
        // 当选择表格行时，将数据显示在输入框中并启用修改按钮
        QString name = ui->controlGroupTable->item(currentRow, 0)->text();
        QString value = ui->controlGroupTable->item(currentRow, 2)->text();

        ui->expressionNameEdit->setText(name);
        ui->expressionValueEdit->setText(value);
        ui->modifyExpressionBtn->setEnabled(true);
    } else {
        // 没有选择时禁用修改按钮
        ui->modifyExpressionBtn->setEnabled(false);
    }
}
