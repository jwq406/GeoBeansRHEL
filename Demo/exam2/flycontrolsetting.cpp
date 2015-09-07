#include "flycontrolsetting.h"
#include "ui_flycontrolsetting.h"
#include <QProcess>

FlyControlSetting::FlyControlSetting(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FlyControlSetting)
{
    ui->setupUi(this);
}

FlyControlSetting::~FlyControlSetting()
{
    delete ui;
}

void FlyControlSetting::on_pushButton_clicked()
{
    QString args = ui->lineEdit->text();
  //  this->addLayer(args);
    QProcess *process= new QProcess();
    process->start("/opt/mygis/startFly.sh", QStringList() << args);


    ui->pushButton->setText(QString("started"));
}
