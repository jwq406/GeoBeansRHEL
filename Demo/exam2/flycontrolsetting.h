#ifndef FLYCONTROLSETTING_H
#define FLYCONTROLSETTING_H

#include <QDialog>

namespace Ui {
class FlyControlSetting;
}

class FlyControlSetting : public QDialog
{
    Q_OBJECT

public:
    explicit FlyControlSetting(QWidget *parent = 0);
    ~FlyControlSetting();

private slots:
    void on_pushButton_clicked();

private:
    Ui::FlyControlSetting *ui;
};

#endif // FLYCONTROLSETTING_H
