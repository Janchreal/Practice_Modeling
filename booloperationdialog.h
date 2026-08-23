#ifndef BOOLOPERATIONDIALOG_H
#define BOOLOPERATIONDIALOG_H

#include <QDialog>
#include <QStringList>

namespace Ui {
class booloperationdialog;
}

class BoolOperationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BoolOperationDialog(QWidget *parent = nullptr);
    ~BoolOperationDialog();

    int getOperationType() const;
    bool getKeepTarget() const;
    bool getKeepTool() const;

    void setTargetBodyName(const QString& name);
    void setToolBodyNames(const QStringList& names);
    void setToolSelectionPrompt();

    void enableConfirmButton(bool enabled);

signals:
    void startSelectionTarget();
    void startSelectionTool();
    void selectionConfirmed();

private slots:
    void on_okButton_clicked();
    void on_cancelButton_clicked();
    void onOperationChanged(int index);

    void on_targetSelectButton_clicked();
    void on_toolSelectButton_clicked();

private:
    Ui::booloperationdialog *ui;

    void checkSelectionComplete();
    bool isPlaceholderText(const QString& text) const;
};

#endif // BOOLOPERATIONDIALOG_H
