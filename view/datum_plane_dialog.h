#ifndef DATUM_PLANE_H
#define DATUM_PLANE_H

#include <QDialog>
#include <QList>
#include <gp_Pnt.hxx>

namespace Ui {
class datum_plane;
}

class datum_plane : public QDialog
{
    Q_OBJECT

public:
    explicit datum_plane(QWidget *parent = nullptr);
    ~datum_plane();

    void setCapturedPoints(const QList<gp_Pnt>& points);
    QString modeText() const;
    bool isOffsetEnabled() const;
    double offsetValue() const;

signals:
    /** 创建方式 / 偏置等参数变化时发出，用于实时预览 */
    void parametersChanged();

private:
    Ui::datum_plane *ui;
    QList<gp_Pnt> capturedPoints_;

    void refreshDefineObjectText();
    void emitParametersChanged();
};

#endif // DATUM_PLANE_H
