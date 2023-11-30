#include "masterdata.h"
#include "ui_masterdata.h"
#include <QDebug>

masterdata::masterdata(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::masterdata)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::Window);
}

masterdata::~masterdata()
{
    delete ui;
}

void masterdata::on_buttonBox_clicked(QAbstractButton *button)
{
    if(button->text() == "OK") {
        QString MilkPlan = ui->lineEdit_MilkPlan->text();
        QString MilkFact = ui->lineEdit_MilkFact->text();

        QString SkMilkPlan = ui->lineEdit_SkMilkPlan->text();
        QString SkMilkFact = ui->lineEdit_SkMilkFact->text();

        QString CreamPlan = ui->lineEdit_CreamPlan->text();
        QString CreamFact = ui->lineEdit_CreamFact->text();
    }
    else if (button->text() == "Cancel") {
        //Действие при нажатии кнопки
    }

    qDebug() << "Нажатие кнопки: " << button->text();
}
