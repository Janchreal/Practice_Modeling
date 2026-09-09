#ifndef HISTORYLISTITEM_H
#define HISTORYLISTITEM_H

#include <QWidget>

class QLabel;
class QPushButton;

class HistoryListItem : public QWidget
{
    Q_OBJECT

public:
    explicit HistoryListItem(QWidget *parent = nullptr);

    // 设置数据
    void setData(const QString& name, const QString& type,
                 const QString& time, const QString& params,
                 const QColor& bgColor);

    // 设置可见性状态
    void setVisibility(bool visible);

    // 设置各个部分
    void setName(const QString& name);
    void setType(const QString& type);
    void setTime(const QString& time);
    void setParams(const QString& params);
    void setBackgroundColor(const QColor& color);

signals:
    void deleteRequested();

private slots:
    void onDeleteClicked();

private:
    void setupUI();
    void setupConnections();

private:
    QLabel *nameLabel;
    QLabel *typeLabel;
    QLabel *timeLabel;
    QLabel *paramsLabel;
    QPushButton *deleteButton;

    // 保存原始背景颜色，以便在可见性改变时恢复
    QColor originalBackgroundColor;
    QString originalNameStyle;
    QString originalParamsStyle;
};

#endif // HISTORYLISTITEM_H
