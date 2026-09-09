#ifndef CHAMFERDIALOG_H
#define CHAMFERDIALOG_H

#include <QDialog>

namespace Ui {
class chamferdialog;
}

class chamferdialog : public QDialog
{
    Q_OBJECT

public:
    explicit chamferdialog(QWidget *parent = nullptr);
    ~chamferdialog();

    QString sectionText() const; // 横截面：对称/非对称/偏置和角度
    // 对称倒角：distanceValue() 返回 distance1
    double distanceValue() const;
    // 非对称倒角：distance1 / distance2 分别控制两侧距离（对应 OCCT Dis1 / Dis2）
    double distance1Value() const;
    double distance2Value() const;
    void setDistance1Value(double v);
    void setDistance2Value(double v);
    void setSelectedEdgeCount(int count);

signals:
    void edgeModeHintRequested();
    void parametersChanged();

private:
    Ui::chamferdialog *ui;
};

#endif // CHAMFERDIALOG_H
