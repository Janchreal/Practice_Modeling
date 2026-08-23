#include "booloperationdialog.h"
#include "ui_booloperationdialog.h"

#include <QComboBox>
#include <QLabel>
#include <QPushButton>

BoolOperationDialog::BoolOperationDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::booloperationdialog)
{
    ui->setupUi(this);

    setWindowTitle(QStringLiteral("布尔运算"));

    ui->operationComboBox->clear();
    ui->operationComboBox->addItem(QStringLiteral("并集"));
    ui->operationComboBox->addItem(QStringLiteral("交集"));
    ui->operationComboBox->addItem(QStringLiteral("差集"));
    ui->operationComboBox->setCurrentIndex(0);

    ui->targetNameLabel->setText(QStringLiteral("未选择"));
    ui->toolNameLabel->setText(QStringLiteral("未选择"));

    ui->keepTargetCheckBox->setChecked(false);
    ui->keepToolCheckBox->setChecked(false);

    ui->targetSelectButton->setStyleSheet(
        QStringLiteral("QPushButton { background-color: #90EE90; font-weight: bold; }"));
    ui->toolSelectButton->setStyleSheet(
        QStringLiteral("QPushButton { background-color: #FFB6C1; font-weight: bold; }"));

    connect(ui->okButton, &QPushButton::clicked, this, &BoolOperationDialog::on_okButton_clicked);
    connect(ui->cancelButton, &QPushButton::clicked, this, &BoolOperationDialog::on_cancelButton_clicked);
    connect(ui->targetSelectButton, &QPushButton::clicked,
            this, &BoolOperationDialog::on_targetSelectButton_clicked);
    connect(ui->toolSelectButton, &QPushButton::clicked,
            this, &BoolOperationDialog::on_toolSelectButton_clicked);
    connect(ui->operationComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BoolOperationDialog::onOperationChanged);

    ui->okButton->setEnabled(false);
    resize(450, 380);
}

BoolOperationDialog::~BoolOperationDialog()
{
    delete ui;
}

int BoolOperationDialog::getOperationType() const
{
    return ui->operationComboBox->currentIndex();
}

bool BoolOperationDialog::getKeepTarget() const
{
    return ui->keepTargetCheckBox->isChecked();
}

bool BoolOperationDialog::getKeepTool() const
{
    return ui->keepToolCheckBox->isChecked();
}

void BoolOperationDialog::setToolSelectionPrompt()
{
    ui->toolNameLabel->setText(QStringLiteral("请点击选择工具体（可多选）..."));
    ui->okButton->setEnabled(false);
}

void BoolOperationDialog::setTargetBodyName(const QString& name)
{
    ui->targetNameLabel->setText(name);
    checkSelectionComplete();
}

void BoolOperationDialog::setToolBodyNames(const QStringList& names)
{
    if (names.isEmpty()) {
        ui->toolNameLabel->setText(QStringLiteral("未选择"));
    } else {
        ui->toolNameLabel->setText(
            QStringLiteral("%1 (%2)").arg(names.join(QStringLiteral(", "))).arg(names.size()));
    }
    checkSelectionComplete();
}

void BoolOperationDialog::enableConfirmButton(bool enabled)
{
    ui->okButton->setEnabled(enabled);
}

bool BoolOperationDialog::isPlaceholderText(const QString& text) const
{
    return text == QStringLiteral("未选择")
        || text.startsWith(QStringLiteral("请点击选择"));
}

void BoolOperationDialog::checkSelectionComplete()
{
    const bool targetSelected = !isPlaceholderText(ui->targetNameLabel->text());
    const bool toolSelected = !isPlaceholderText(ui->toolNameLabel->text());
    ui->okButton->setEnabled(targetSelected && toolSelected);
}

void BoolOperationDialog::onOperationChanged(int index)
{
    Q_UNUSED(index);
}

void BoolOperationDialog::on_okButton_clicked()
{
    if (isPlaceholderText(ui->targetNameLabel->text())
        || isPlaceholderText(ui->toolNameLabel->text())) {
        return;
    }

    if (!signalsBlocked()) {
        accept();
    }
}

void BoolOperationDialog::on_cancelButton_clicked()
{
    if (!signalsBlocked()) {
        reject();
    }
}

void BoolOperationDialog::on_targetSelectButton_clicked()
{
    ui->targetNameLabel->setText(QStringLiteral("请点击选择目标体..."));
    ui->okButton->setEnabled(false);
    emit startSelectionTarget();
}

void BoolOperationDialog::on_toolSelectButton_clicked()
{
    setToolSelectionPrompt();
    emit startSelectionTool();
}
