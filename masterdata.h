#ifndef DATA_H
#define DATA_H

#include <QDialog>
#include <QAbstractButton>

namespace Ui {
    class masterdata;
}

class masterdata : public QDialog
{
    Q_OBJECT

public:
    explicit masterdata(QWidget *parent = nullptr);
    ~masterdata();

private slots:
    void on_buttonBox_clicked(QAbstractButton *button);

private:
    Ui::masterdata *ui;
};

#endif // DATA_H
