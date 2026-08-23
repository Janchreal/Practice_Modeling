#include "historylistitem.h"
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>

HistoryListItem::HistoryListItem(QWidget *parent)
    : QWidget(parent)
    , nameLabel(new QLabel(this))
    , typeLabel(new QLabel(this))
    , timeLabel(new QLabel(this))
    , paramsLabel(new QLabel(this))
    , deleteButton(new QPushButton("删除", this))
    , originalBackgroundColor(Qt::white)
{
    setupUI();
    setupConnections();
}

void HistoryListItem::setupUI()
{
    // 保存原始样式
    originalNameStyle = "font-weight: bold; font-size: 14px; color: black;";
    originalParamsStyle = "font-size: 12px; color: black;";

    // 设置名称和参数的样式
    nameLabel->setStyleSheet(originalNameStyle);
    paramsLabel->setStyleSheet(originalParamsStyle);

    // 隐藏类型和时间标签
    typeLabel->setVisible(false);
    timeLabel->setVisible(false);

    // 设置标签属性
    nameLabel->setWordWrap(true);
    nameLabel->setMinimumHeight(20);
    paramsLabel->setWordWrap(true);
    paramsLabel->setMinimumHeight(18);

    // 允许标签扩展
    nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    paramsLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    // 设置删除按钮样式
    deleteButton->setFixedSize(60, 25);
    deleteButton->setStyleSheet(
        "QPushButton { "
        "background-color: #ff6b6b; "
        "color: white; "
        "border: none; "
        "border-radius: 4px; "
        "font-size: 12px; "
        "}"
        "QPushButton:hover { background-color: #ff5252; }"
        "QPushButton:pressed { background-color: #e53935; }"
        );

    // 创建布局
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    QVBoxLayout *textLayout = new QVBoxLayout();

    // 只添加名称和参数标签
    textLayout->addWidget(nameLabel);
    textLayout->addWidget(paramsLabel);
    textLayout->setSpacing(2);

    mainLayout->addLayout(textLayout, 1);
    mainLayout->addWidget(deleteButton);
    mainLayout->setContentsMargins(12, 8, 12, 8);
    mainLayout->setSpacing(10);

    // 设置widget的最小尺寸
    this->setMinimumHeight(60);
}

void HistoryListItem::setupConnections()
{
    connect(deleteButton, &QPushButton::clicked, this, &HistoryListItem::onDeleteClicked);
}

void HistoryListItem::setData(const QString& name, const QString& type,
                              const QString& time, const QString& params,
                              const QColor& bgColor)
{
    setName(name);
    setParams(params);
    setBackgroundColor(bgColor);
    // 保存原始背景颜色
    originalBackgroundColor = bgColor;
}

void HistoryListItem::setName(const QString& name)
{
    nameLabel->setText(name);
    nameLabel->setToolTip(name);
}

void HistoryListItem::setType(const QString& type)
{
    // 不再使用，但保留接口
}

void HistoryListItem::setTime(const QString& time)
{
    // 不再使用，但保留接口
}

void HistoryListItem::setParams(const QString& params)
{
    paramsLabel->setText(params);
    paramsLabel->setToolTip(params);
}

void HistoryListItem::setBackgroundColor(const QColor& color)
{
    QString style = QString("HistoryListItem { "
                            "background-color: %1; "
                            "border-radius: 6px; "
                            "border: 1px solid %2; "
                            "}")
                        .arg(color.name())
                        .arg(color.darker(110).name());
    this->setStyleSheet(style);
}

void HistoryListItem::onDeleteClicked()
{
    emit deleteRequested();
}

void HistoryListItem::setVisibility(bool visible)
{
    // 根据可见性设置不同的样式
    if (visible) {
        // 可见状态 - 正常显示
        nameLabel->setStyleSheet(originalNameStyle);
        paramsLabel->setStyleSheet(originalParamsStyle);

        // 恢复原始背景颜色
        setBackgroundColor(originalBackgroundColor);
    } else {
        // 隐藏状态 - 灰色显示，带删除线
        nameLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: lightgray; text-decoration: line-through;");
        paramsLabel->setStyleSheet("font-size: 12px; color: lightgray; text-decoration: line-through;");

        // 使用更浅的背景颜色表示隐藏状态
        QColor hiddenColor = originalBackgroundColor.lighter(150); // 变亮50%
        setBackgroundColor(hiddenColor);
    }
}
