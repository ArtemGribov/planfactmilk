#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDate>
#include <QLabel>
#include <QDir>
#include <masterdata.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QString version;
    void startUnpack(const QString &pathprogram,
                    const QString &pathdirplan,
                    const QString &pathdestiny);
    void deletegarbage();
    bool calculation();
    QString findSourceFile(const QString &sourceserver);
    bool findFileFact();
    bool loadFileFact(const QString &pathfile, const QString &pathfileprevious);
    virtual void timerEvent(QTimerEvent*);
    virtual void resizeEvent(QResizeEvent*);
    QString findDirFact(const QDir &dir);
    bool loadCompopentsData();
    bool loadCriteria();
    bool isComponent(const QString *Category,
                     const QString *Type,
                     const QString *Material,
                     const QString *Plant);
    void addSubTotalRow();
    void addTotalRow();
    void deltaTotalStock(QString type);
    void addTotalReq();
    void slicetime();
    bool prepeadTables();

private slots:
    void on_dateEdit_userDateChanged(const QDate &date);
    bool on_pushButtonStart_clicked();
    bool on_action_FindDirectoryFact_triggered();
    void on_action_About_triggered();
//    void on_action_MasterData_triggered();

    void on_action_Exit_triggered();

private:
    Ui::MainWindow *ui;
    QDate selectDate;
    QString selectPlant;
    float MilkCriteria;
    float SkMilkCriteria;
    float CreamCriteria;
    QString pathprogram;
    QString pathdirplan;
    QString pathdestin;
    QString pathfileplan;
    QString destinfileplan;
    QLabel* labelstatus;
    const QStringList lstDisk = { "C", "D", "E" };
    QDir dirFact;
    QString pathFact;
    QString fileFact;
    QString fileFactPrevious;
    QString pathFactFile;
    QString pathFactFilePrevious;
    int timerId;
    QString namefileplanmask;
    QString internalpathdirfact;
    QStringList admin;
    QStringList user;

    QStringList ColumnHeader;

    QStringList listMilkPlan;
    QStringList listSkMilkPlan;
    QStringList listCreamPlan;
    QStringList listMilkFact;
    QStringList listSkMilkFact;
    QStringList listCreamFact;

public:
    QList<QStringList> rowListMain;
    QList<QStringList> rowListProduct;
    QList<QStringList> rowListStep;
    QList<QStringList> rowListRun;
    QList<QStringList> rowListProductFlow;
    QList<QStringList> rowListStock;
    QList<QStringList> rowListFact;
    QList<QStringList> rowListFactPrevous;
};
#endif // MAINWINDOW_H
