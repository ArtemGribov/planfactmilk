#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtDebug>
#include <QProcess>
#include <QFile>
#include "Windows.h"
#include "tablemodelmain.h"
#include <QMessageBox>
#include "masterdata.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //Версия
    version = "2.3.0";

    //Дата
    ui->dateEdit->setDate(QDateTime::currentDateTime().date());
    //ui->dateEdit->setDate(QDate(2023, 12, 7));

    //Инициализация дисков для поиска маршрута
    const QStringList lstDisk = { "D", "C", "E" };

    //Инициализация строки состояния
    labelstatus = new QLabel(this);
    statusBar()->addWidget(labelstatus);

    ColumnHeader << "StepId"
                 << "RunId"
                 << "MachineId"
                 << "ProductLocationId"
                 << "Description"
                 << "Marker"
                 << "Version"
                 << "OperationId"
                 << "OrderPlan"
                 << "StartPlan"
                 << "EndPlan"
                 << "QuantityPlan"
                 << "OrderFact"
                 << "Status"
                 << "StartFact"
                 << "EndFact"
                 << "QuantityFact"
                 << "DeliveryFact"
                 << "MilkReqPlan"
                 << "SkMilkReqPlan"
                 << "CreamReqPlan"
                 << "MilkReqFact"
                 << "SkMilkReqFact"
                 << "CreamReqFact"
                 << "MilkDelta"
                 << "SkMilkDelta"
                 << "CreamDelta";
}

MainWindow::~MainWindow()
{
    deletegarbage();
    delete ui;
}

void MainWindow::on_dateEdit_userDateChanged(const QDate &date)
{
    ui->dateEdit->setDate(date);
}

//Конфигуратор настоек пользователя
bool MainWindow::on_action_FindDirectoryFact_triggered()
{
    QApplication::processEvents();

    pathFact = "";
    //Проверка на дисках
    foreach(QString disk, lstDisk) {
        dirFact.setPath(disk + ":/");
        //Поиск директории
        labelstatus->setText("Поиск директории c фактическими данными. Пожалуйста, подождите...");
        findDirFact(dirFact);
        if(!pathFact.isEmpty()) break;
    }
    labelstatus->setText("Директория с фактическими данными найдена: " + pathFact);
    //Сохранение настроек в файл
    QFile file(QDir::tempPath() + "/" + "settings.cfg");
    if(QFile::exists(QDir::tempPath() + "/" + "/settings.cfg")) {
        QFile::remove(QDir::tempPath() + "/" + "/settings.cfg");
    }
    file.open(QIODevice::WriteOnly);
    QTextStream out(&file);
    out << pathFact;
    file.close();

    return true;
}

//Поиск директории с фактом
QString MainWindow::findDirFact(const QDir &dirFact) {
    QApplication::processEvents();
    QStringList listDir = dirFact.entryList(QDir::NoDot | QDir::NoDotDot | QDir::Dirs);
    foreach(QString subdir, listDir) {
        //Рекурсивный поиск
        if(subdir == "Snapshots from SAP") {
            pathFact = dirFact.absoluteFilePath(subdir);
            break;
        }
        else if(pathFact.isEmpty()) {
            findDirFact(QDir(dirFact.absoluteFilePath(subdir)));
        }
        else return pathFact;
    }
    return QString();
}

void MainWindow::deletegarbage() {
    //Удалить исходный файл
    if(QFile::exists(destinfileplan))
        QFile::remove(destinfileplan);

    //Удалить распакованные файлы
    QFile file(pathdestin + "_$pipeline.tmp");
    if(QFile::exists(pathdestin + "_$pipeline.tmp")) {
        file.open(QIODevice::ReadOnly);
        QTextStream out(&file);
        QString str = out.readAll();
        QStringList strlist = str.split("\r\n");

        for(int i = 1; i < strlist.length()-1; i++) {
            QFile::remove(pathdestin + strlist[i]);
        }
        file.close();
    }
    //Удалить файл маршрутов
    if(QFile::exists(pathdestin + "_$pipeline.tmp"))
        QFile::remove(pathdestin + "_$pipeline.tmp");
}


//Расшифровка ключа
QString crypto(QString str) {
    QString cryptostr;
    for(int i = 0; i < str.length(); i++) {
        char ch = str.at(i).toLatin1();
        //Меняем биты
        ch ^= 1;
        cryptostr.append(QString(ch));
    }
    return cryptostr;
}

bool MainWindow::on_pushButtonStart_clicked()
{
    QApplication::processEvents();

    //Отключить активность кнопок
    ui->pushButtonStart->setEnabled(false);
    ui->dateEdit->setEnabled(false);
    ui->comboBoxPlant->setEnabled(false);

    //Очистка
    rowListMain.clear();
    rowListProduct.clear();
    rowListStep.clear();
    rowListRun.clear();
    rowListProductFlow.clear();
    rowListStock.clear();
    rowListFact.clear();
    rowListFactPrevous.clear();

    listMilkPlan.clear();
    listSkMilkPlan.clear();
    listCreamPlan.clear();
    listMilkFact.clear();
    listSkMilkFact.clear();
    listCreamFact.clear();

    pathdirplan.clear();
    namefileplanmask.clear();
    user.clear();
    admin.clear();

    //Чтение настроек пользователя
    QString fileName = QDir::tempPath() + "/settings.cfg";
    QFile fileSetting(fileName);
    if(QFile::exists(fileName)) {
        if(!fileSetting.open(QIODevice::ReadOnly | QIODevice::Text)) {
            labelstatus->setText("Не найден файл settings");
            return false;
        }
        QTextStream in(&fileSetting);
        in.setCodec("UTF-8");
        //Чтение пути
        pathFact = in.readAll();
        if(pathFact.isEmpty()) {
            QMessageBox::warning(0,"Ошибка", "Не найдена директория с фактом");
            labelstatus->setText("Ошибка");
            return false;
        }
    }
    fileSetting.close();


    //Чтение настроек приложения
    QString pathData;
    pathData = pathFact + "/planfactmilk/settings_app.cfg";
    QFile f(pathData);
    if(QFile::exists(pathData)) {
        if(!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::warning(0,"Ошибка", "Невозможно прочитать файл settings_app.cfg");
            labelstatus->setText("Ошибка");
            return false;
        }
        QTextStream stream(&f);
        stream.setCodec("UTF-8");
        //Чтение
        while (!stream.atEnd()) {
            QString str = stream.readLine();
            if(!str.isEmpty()) {
                QStringList strlist;
                strlist = str.split('\t');

                if(!strlist.isEmpty()) {
                    //Юзеры
                    if(strlist[0] == "user") {
                        user << strlist;
                        user.pop_front();
                        //Проверка доступа юзеров
                        foreach(QString s, user) {
                            //Проверка юзера
                            if(crypto(s) == QDir::home().dirName())
                                break;
                            else if(s == user.last()) {
                                QMessageBox::warning(0,"Нет доступа", "Отказано в доступе. "
                                                                      "Пожалуйста, запросите доступ у администратора");
                                labelstatus->setText("Нет доступа");
                                //Включить активность кнопок
                                ui->pushButtonStart->setEnabled(true);
                                ui->dateEdit->setEnabled(true);
                                ui->comboBoxPlant->setEnabled(true);
                                return false;
                            }
                        }
                    }
                    //Путь к директории плана
                    if(strlist[0] == "pathdirplan") {
                        pathdirplan = strlist[1];
                    }
                    //Маска файла с планом
                    if(strlist[0] == "namefileplanmask") {
                        namefileplanmask = strlist[1];
                    }
                    //Список админов
                    if(strlist[0] == "admin") {
                        admin << strlist;
                        admin.pop_front();
                    }
                }
            }
        }
        f.close();
    }


    //Поиск данных и запуск распаковки
    if(selectDate != ui->dateEdit->date()) {
        selectDate = ui->dateEdit->date();
        selectPlant = ui->comboBoxPlant->currentText();
        //Поиск файла с фактом
        if(!findFileFact()) {
            ui->pushButtonStart->setEnabled(true);
            ui->dateEdit->setEnabled(true);
            ui->comboBoxPlant->setEnabled(true);
            QMessageBox::warning(0,"Ошибка", "Ошибка поиска факта");
            labelstatus->setText("Ошибка");
            return false;
        }
        //Загрузка файла с фактом
        if(!loadFileFact(pathFactFile, pathFactFilePrevious)) {
            ui->pushButtonStart->setEnabled(true);
            ui->dateEdit->setEnabled(true);
            ui->comboBoxPlant->setEnabled(true);
            QMessageBox::warning(0,"Ошибка", "Ошибка загрузки факта");
            labelstatus->setText("Ошибка");
            return false;
        }

        //Поиск плановых данных
        pathfileplan = findSourceFile(pathdirplan);
        if(pathfileplan.isEmpty()) {
            ui->pushButtonStart->setEnabled(true);
            ui->dateEdit->setEnabled(true);
            ui->comboBoxPlant->setEnabled(true);
            QMessageBox::warning(0,"Ошибка", "Ошибка поиска плана");
            labelstatus->setText("Ошибка");
            return false;
        }

        //Удалить буферные файлы
        deletegarbage();

        //Место распаковки
        pathdestin = QDir::tempPath() + "/";

        //Удалить старый файл
        if(QFile::exists(destinfileplan))
            QFile::remove(destinfileplan);

        destinfileplan = pathdestin + "/" + pathfileplan.right(40);
        QDir dir(pathdestin);
        if(QDir(pathdestin).exists()) {
            if(!pathfileplan.isEmpty() && !destinfileplan.isEmpty()) {
                //Копируем файл исходник
                if(QFile::exists(destinfileplan)) {
                    QFile::remove(destinfileplan);
                    QFile::copy(pathfileplan, destinfileplan);
                }
                else
                    QFile::copy(pathfileplan, destinfileplan);

                //Создать файл передачи параметров
                QFile file(pathdestin + "_$pipeline.tmp");
                if(QFile::exists(pathdestin + "_$pipeline.tmp")) {
                    QFile::remove(pathdestin + "_$pipeline.tmp");
                }
                file.open(QIODevice::WriteOnly);
                QTextStream out(&file);
                out << "_%start_process";
                file.close();

                //Путь к распаковщику
                pathprogram = QDir::currentPath() + "/unpack.exe";
                if(!QFile::exists(pathprogram)) {
                    labelstatus->setText("Распаковщик отсутствует. Проверьте целостность файлов.");
                    ui->pushButtonStart->setEnabled(true);
                    ui->dateEdit->setEnabled(true);
                    ui->comboBoxPlant->setEnabled(true);
                    return false;
                }

                //Запуск распаковщика
                startUnpack(pathprogram, destinfileplan, pathdestin);
                return true;
            }
            else {
                labelstatus->setText("Ошибка. Ошибка загрузки плана");
                ui->pushButtonStart->setEnabled(true);
                ui->dateEdit->setEnabled(true);
                ui->comboBoxPlant->setEnabled(true);
                return false;
            }
        }
        else {
            labelstatus->setText("Директория для распаковки не найдена.");
            ui->pushButtonStart->setEnabled(true);
            ui->dateEdit->setEnabled(true);
            ui->comboBoxPlant->setEnabled(true);
            return false;
        }
    }
    else {
        selectPlant = ui->comboBoxPlant->currentText();
        //Поиск файлов с фактом
        if(!findFileFact()) {
            ui->pushButtonStart->setEnabled(true);
            ui->dateEdit->setEnabled(true);
            ui->comboBoxPlant->setEnabled(true);
            QMessageBox::warning(0,"Ошибка", "Ошибка поиска факта");
            labelstatus->setText("Ошибка");
            return false;
        }
        //Загрузка файлов с фактом
        if(!loadFileFact(pathFactFile, pathFactFilePrevious)) {
            ui->pushButtonStart->setEnabled(true);
            ui->dateEdit->setEnabled(true);
            ui->comboBoxPlant->setEnabled(true);
            QMessageBox::warning(0,"Ошибка", "Ошибка загрузки факта");
            labelstatus->setText("Ошибка");
            return false;
        }

        //Поиск плановых данных
        pathfileplan = findSourceFile(pathdirplan);
        if(pathfileplan.isEmpty()) {
            ui->pushButtonStart->setEnabled(true);
            ui->dateEdit->setEnabled(true);
            ui->comboBoxPlant->setEnabled(true);
            QMessageBox::warning(0,"Ошибка", "Ошибка поиска плана");
            labelstatus->setText("Ошибка");
            return false;
        }

        //Выполнить расчеты
        labelstatus->setText("Анализирую данные. Пожалуйста, подождите...");
        calculation();
        //Включить активность кнопок
        ui->pushButtonStart->setEnabled(true);
        ui->dateEdit->setEnabled(true);
        ui->comboBoxPlant->setEnabled(true);
        return true;
    }
}


//Поиск фактических данных
bool MainWindow::findFileFact() {
    QApplication::processEvents();
    fileFact = "";
    pathFactFile = "";

    fileFactPrevious = "";
    pathFactFilePrevious = "";

    QStringList listFiles;
    //Поиск файла с фактом
    listFiles = QDir(pathFact + "/ZLRUOPS_TRN_PRORD/" + selectPlant).entryList(QStringList()
                << "ZLRUOPS_TRN_PRORD_" + selectDate.toString("dd.MM.yyyy") + "_21_??_??.csv",
                QDir::Files);
    if(!listFiles.isEmpty()) {
        fileFact = listFiles[0];
        //Записать полный путь к файлу
        pathFactFile = pathFact + "/ZLRUOPS_TRN_PRORD/" + selectPlant + "/" + fileFact;
    }
    else {
        labelstatus->setText("Ошибка. Фактические данные не найдены.");
        return false;
    }
    listFiles.clear();
    //Поиск прошлого файла с фактом
    listFiles = QDir(pathFact + "/ZLRUOPS_TRN_PRORD/" + selectPlant).entryList(QStringList()
                << "ZLRUOPS_TRN_PRORD_" + selectDate.addDays(-1).toString("dd.MM.yyyy") + "_21_??_??.csv",
                QDir::Files);
    if(!listFiles.isEmpty()) {
        fileFactPrevious = listFiles[0];
        //Записать полный путь к файлу
        pathFactFilePrevious = pathFact + "/ZLRUOPS_TRN_PRORD/" + selectPlant + "/" + fileFactPrevious;
    }
    else {
        QMessageBox::warning(0,"Не найден факт предыдущего дня", "Внимание, часть фактических данных найти не удалось.\n"
                                                                 "Для некоторых заказов, точность расчета может быть снижена.");
        return true;
    }
    return true;
}


//Загрузка фактические данных
bool MainWindow::loadFileFact(const QString &pathfile, const QString &pathfileprevious) {
    //Загрузка факта
    QFile f(pathfile);
    if(QFile::exists(pathfile)) {
        if(!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "Error read file pathfile";
            return false;
        }
        QTextStream stream(&f);
        stream.setCodec("UTF-8");

        //Прочитать первую строку в мусор
        if(!stream.atEnd()) {
            QString str = stream.readLine();
            str = stream.readLine();
        }
        //Чтение данных
        while (!stream.atEnd()) {
            QString str = stream.readLine();
            if(!str.isEmpty()) {
                QStringList strlist;
                strlist = str.split('\t');
                rowListFact.push_back(strlist);
            }
        }
        f.close();
    }

    //факт предыдущего дня
    f.setFileName(pathfileprevious);
    if(QFile::exists(pathfileprevious)) {
        if(!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "Error read file pathfileprevious";
            return false;
        }
        QTextStream stream(&f);
        stream.setCodec("UTF-8");

        //Прочитать первую строку в мусор
        if(!stream.atEnd()) {
            QString str = stream.readLine();
            str = stream.readLine();
        }
        //Чтение данных
        while (!stream.atEnd()) {
            QString str = stream.readLine();
            if(!str.isEmpty()) {
                QStringList strlist;
                strlist = str.split('\t');
                rowListFactPrevous.push_back(strlist);
            }
        }
        f.close();
    }
    return true;
}

QString MainWindow::findSourceFile(const QString &pathdir)
{
    QApplication::processEvents();
    //Log Debug
//    QFile file(QDir::tempPath() + "/" + "deb");
//    if(QFile::exists(QDir::tempPath() + "/" + "deb")) {
//        QFile::remove(QDir::tempPath() + "/" + "deb");
//    }
//    file.open(QIODevice::WriteOnly);
//    QTextStream out(&file);
//    out.setCodec("UTF-8");
//    out << "start log\n";

    //Передаем путь к папке поиска
    QDir dir(pathdir);
    //Поиск папки по выбранной дате
    QStringList listDirs = dir.entryList(QDir::NoDot | QDir::NoDotDot | QDir::Dirs);

    foreach(QString subdir, listDirs) {
        QDir source(pathdir + subdir);
        if(selectDate.toString("yyyyMMdd") == source.dirName().left(source.dirName().length() - 6)) {
            QStringList listFiles = source.entryList(QStringList() << "*PlanFactProductionOPR.xlsx", QDir::Files);
            if(!listFiles.isEmpty()) {
                foreach(QString file, listFiles) {
                    QString hour = file.mid(8, 2);
                    if(hour.toInt() >= 17 && hour.toInt() <= 23) {
                        QString fileFinal = source.absolutePath()+ "/" + file;
                        return fileFinal;
                    }
                }
            }
        }
    }
    return QString();
}


class QDetachableProcess
        : public QProcess {
public:
    QDetachableProcess(QObject *parent = 0)
        : QProcess(parent) {
    }
    void detach() {
       waitForStarted();
       setProcessState(QProcess::NotRunning);
    }
};


void MainWindow::startUnpack(const QString &pathprogram,
                            const QString &pathdirplan,
                            const QString &pathdestiny)
{
    QApplication::processEvents();
    QDetachableProcess p;
    QString program = "cmd.exe";
    QStringList arguments = QStringList() << "/C" << pathprogram << pathdirplan << pathdestiny;
    p.setCreateProcessArgumentsModifier(
                [](QProcess::CreateProcessArguments *args) {
        args->flags |= CREATE_NEW_CONSOLE;
        args->flags &= CREATE_NO_WINDOW;
        args->startupInfo->dwFlags &= ~STARTF_USESTDHANDLES;
        args->startupInfo->dwFlags |= STARTF_USEFILLATTRIBUTE;
    });
    p.start(program, arguments);
    p.detach();

    //Проверка готовности распаковки
    timerId = this->startTimer(1000);
}


//Событие таймера
void MainWindow::timerEvent(QTimerEvent*)
{
    QApplication::processEvents();
    int t = QTime::currentTime().second();
    if(t % 2 == 0) {
        labelstatus->setText("Распаковка данных .....");
    }
    else labelstatus->setText("Распаковка данных");

     QFile file(pathdestin + "_$pipeline.tmp");
     //Проверка буферного файла на окончание распаковки
     if(QFile::exists(pathdestin + "_$pipeline.tmp")) {
         file.open(QIODevice::ReadOnly);
         QTextStream in(&file);
         QString str = in.readAll();
         QStringList strlist = str.split('\n');
         if(strlist[strlist.length()-1] == "_%end_process") {

            //Уничтожить таймер
            this->killTimer(timerId);

            //Выполнить расчеты
            labelstatus->setText("Анализирую данные. Пожалуйста, подождите...");
            calculation();

            //Включить активность кнопок
            ui->pushButtonStart->setEnabled(true);
            ui->dateEdit->setEnabled(true);
            ui->comboBoxPlant->setEnabled(true);
         }
         file.close();
     }
}


//Подготовка плановых данных
bool MainWindow::prepeadTables() {
    QApplication::processEvents();
    QFile file(pathdestin + "_$pipeline.tmp");
    if(QFile::exists(pathdestin + "_$pipeline.tmp")) {
        if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "No file _$pipeline.tmp";
            return false;
        }
        QTextStream in(&file);
        in.setCodec("UTF-8");
        QString strdata = in.readAll();
        QStringList strlistdata = strdata.split('\n');
        foreach(QString fileName, strlistdata) {
            QStringList listSheet = { "Product", "Step", "Run", "ProductFlow", "Stock" };
            for(int i = 0; i < listSheet.length(); i++) {
                if(fileName == listSheet[i]) {
                    QFile f(pathdestin + fileName);
                    if(QFile::exists(pathdestin + fileName)) {
                        if(!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                            qDebug() << "No found file " + fileName;
                            return false;
                        }
                        QTextStream stream(&f);
                        stream.setCodec("UTF-8");

                        //Чтение данных
                        while (!stream.atEnd()) {
                            QString strfull = stream.readLine();
                            QStringList strlist;
                            strlist = strfull.split("), (");
                            foreach(QString s, strlist) {
                                while(s.indexOf("datetime.datetime") >= 0) {
                                    int pstart;
                                    int pend;
                                    pstart = s.indexOf("(");
                                    pend = s.indexOf(')');
                                    QString stringbuffer;
                                    stringbuffer = s.mid(pstart+1, pend - pstart-1);
                                    QStringList m = stringbuffer.split(',');
                                    //Если нет секунд
                                    if(m.length() == 5) {
                                        m.append("0");
                                    }
                                    QDateTime d(QDate(m[0].toInt(), m[1].toInt(), m[2].toInt()), QTime(m[3].toInt(), m[4].toInt(), m[5].toInt()));
                                    stringbuffer = d.toString("yyyy-MM-dd hh:mm:ss");
                                    s.replace(pstart-17, pend - pstart+1+17, QString('\'' + stringbuffer + '\''));
                                }
                                s.replace(")", "\'");
                                s.replace("(", "");
                                s.replace('\'', "");
                                s.replace(", ", "\t");
                                QStringList str;
                                str = s.split("\t");
                                switch(listSheet.indexOf(listSheet[i])) {
                                    case 0: rowListProduct.push_back(str); break;
                                    case 1: rowListStep.push_back(str); break;
                                    case 2: rowListRun.push_back(str); break;
                                    case 3: rowListProductFlow.push_back(str); break;
                                    case 4: rowListStock.push_back(str); break;
                                }
                            }
                        }
                        f.close();
                    }
                }
            }
        }
        file.close();
    }
    return true;
}

bool MainWindow::calculation() {
    QApplication::processEvents();

    if(!prepeadTables()) { return false; }

    //Загрузка списка молочных компонентов для отслеживания
    if(!loadCompopentsData()) { return false; }

    //Загрузка нормативных отклонений
    if(!loadCriteria()) { return false; }

    QRegExp rxOperation("(/0020)");

    //Обработка данных
    for(int rStep = 1; rStep < rowListStep.length(); rStep++) {
        //Фильтр
        if(rowListStep[rStep].at(rowListStep[0].indexOf("MachineId")).left(4) == selectPlant) {
            QRegExp rxProductLocation("(CIP|3400|QI|Test)");
            QRegExp rxMachine("(ZETA|WALDNER|SELO|DARBO|TERLET)");
            if(rxProductLocation.indexIn(rowListStep[rStep].at(rowListStep[0].indexOf("ProductLocationId"))) == -1) {
                if(rxMachine.indexIn(rowListStep[rStep].at(rowListStep[0].indexOf("MachineId"))) == -1) {
                    //if(rxOperation.indexIn(rowListStep[rStep].at(rowListStep[0].indexOf("OperationId"))) != -1) {
                        QStringList s;
                        s << rowListStep[rStep].at(rowListStep[0].indexOf("StepId"))
                        << rowListStep[rStep].at(rowListStep[0].indexOf("RunId"))
                        << rowListStep[rStep].at(rowListStep[0].indexOf("MachineId")).mid(rowListStep[rStep].at(rowListStep[0].indexOf("MachineId")).indexOf("/")+1);
                        s << rowListStep[rStep].at(rowListStep[0].indexOf("ProductLocationId")).left(rowListStep[rStep].at(rowListStep[0].indexOf("ProductLocationId")).indexOf("/"));

                        //Поиск описания материала
                        for(int rProduct = 1; rProduct < rowListProduct.length(); rProduct++) {
                            if(rowListStep[rStep].at(rowListStep[0].indexOf("ProductLocationId")).left(rowListStep[rStep].at(rowListStep[0].indexOf("ProductLocationId")).indexOf("/"))
                            == rowListProduct[rProduct].at(rowListProduct[0].indexOf("ProductId"))) {
                                s << rowListProduct[rProduct].at(rowListProduct[0].indexOf("DESCRIPTION"));
                                break;
                            }
                            if(rProduct == rowListProduct.length()-1) {
                                s << "";
                            }
                        }

                        //Маркер
                        s << "";

                        //Версия
                        if(rowListStep[rStep].at(rowListStep[0].indexOf("OperationId")).left(4) != "PRO/") {
                            if(rowListStep[rStep].at(rowListStep[0].indexOf("OperationId")).indexOf("/") != -1) {
                                int Pos = rowListStep[rStep].at(rowListStep[0].indexOf("OperationId")).indexOf("/");
                                s << rowListStep[rStep].at(rowListStep[0].indexOf("OperationId")).left(Pos);
                            }
                            else s << "";
                        }
                        else s << "";

                        //Операция
                        s << rowListStep[rStep].at(rowListStep[0].indexOf("OperationId"));

                        //Поиск номера PLO
                        for(int rRun = 1; rRun < rowListRun.length(); rRun++) {
                            if(rowListStep[rStep].at(rowListStep[0].indexOf("RunId")) == rowListRun[rRun].at(rowListRun[0].indexOf("RunId"))) {
                                if(rowListRun[rRun].at(rowListRun[0].indexOf("ERPOrderId")).left(4) == "PLO/" || rowListRun[rRun].at(rowListRun[0].indexOf("ERPOrderId")).left(4) == "PRO/") {
                                    s << rowListRun[rRun].at(rowListRun[0].indexOf("ERPOrderId")).left(4) +
                                    rowListRun[rRun].at(rowListRun[0].indexOf("ERPOrderId")).mid(rowListRun[rRun].at(rowListRun[0].indexOf("ERPOrderId")).indexOf("RFCOMP")+7);
                                    break;
                                }
                                else {
                                    s << rowListRun[rRun].at(rowListRun[0].indexOf("ERPOrderId"));
                                    break;
                                }
                            }
                            if(rRun == rowListRun.length()-1) {
                                s << "";
                            }
                        }

                        QDateTime dt;
                        dt = QDateTime::fromString(rowListStep[rStep].at(rowListStep[0].indexOf("StartDate")), "yyyy-MM-dd hh:mm:ss");
                        s << dt.toString("dd.MM.yyyy hh:mm:ss");

                        dt = QDateTime::fromString(rowListStep[rStep].at(rowListStep[0].indexOf("EndDate")), "yyyy-MM-dd hh:mm:ss");
                        s << dt.toString("dd.MM.yyyy hh:mm:ss");

                        float PlanQty = rowListStep[rStep].at(rowListStep[0].indexOf("PlannedQuantity")).toFloat();
                        QString str;
                        s << str.number(PlanQty);
                        rowListMain.append(s);
                    //}
                }
            }
        }
    }


    //Поиск номера PRO
    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
        if(rowListMain[rMain].at(ColumnHeader.indexOf("OrderPlan")).left(4) == "PLO/" || rowListMain[rMain].at(ColumnHeader.indexOf("OrderPlan")).left(4) == "PRO/") {
            bool ok;
            int Pos = rowListMain[rMain].at(ColumnHeader.indexOf("OrderPlan")).indexOf("/")+1;
            long long ORDER1 = rowListMain[rMain].at(ColumnHeader.indexOf("OrderPlan")).mid(Pos).toLongLong(&ok, 10);
            if(ok) {
                long long ORDER2;
                for(int rFact = 1; rFact < rowListFact.length(); rFact++) {
                    if(rowListMain[rMain].at(ColumnHeader.indexOf("OrderPlan")).left(4) == "PLO/") {
                        ORDER2 = rowListFact[rFact].at(rowListFact[0].indexOf("PLNUM")).toLongLong(&ok, 10);
                    }
                    else {
                        ORDER2 = rowListFact[rFact].at(rowListFact[0].indexOf("AUFNR")).toLongLong(&ok, 10);
                    }
                    if(ok) {
                        if(ORDER1 == ORDER2) {
                            long long PRO;
                            //Технологический номер
                            PRO = rowListFact[rFact].at(rowListFact[0].indexOf("AUFNR")).toLongLong();
                            rowListMain[rMain] << QString::number(PRO);

                            //Статус
                            rowListMain[rMain] << rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4);
                            //Дата Время начала
                            QDateTime dt;
                            dt = QDateTime::fromString(rowListFact[rFact].at(rowListFact[0].indexOf("GSTRP")) + " " + rowListFact[rFact].at(rowListFact[0].indexOf("GSUZP")), "dd.MM.yyyy hh:mm:ss");
                            if(!dt.isValid()) dt = QDateTime::fromString(rowListFact[rFact].at(rowListFact[0].indexOf("GSTRP")) + " 00:00:00", "dd.MM.yyyy hh:mm:ss");
                                rowListMain[rMain] << dt.toString("dd.MM.yyyy hh:mm:ss");
                            //Дата Время окончания
                            dt = QDateTime::fromString(rowListFact[rFact].at(rowListFact[0].indexOf("GLTRP")) + " " + rowListFact[rFact].at(rowListFact[0].indexOf("GLUZP")), "dd.MM.yyyy hh:mm:ss");
                            if(!dt.isValid()) dt = QDateTime::fromString(rowListFact[rFact].at(rowListFact[0].indexOf("GLTRP")) + " 00:00:00", "dd.MM.yyyy hh:mm:ss");
                                rowListMain[rMain] << dt.toString("dd.MM.yyyy hh:mm:ss");
                            //Кол-во PRO
                            float Qty;
                            Qty = rowListFact[rFact].at(rowListFact[0].indexOf("GAMNG_KG")).toFloat(&ok);
                            if(ok)
                                rowListMain[rMain] << QString::number(Qty);
                            //Выпуск
                            Qty = rowListFact[rFact].at(rowListFact[0].indexOf("GWEMG_KG")).toFloat(&ok);
                            if(ok)
                                rowListMain[rMain] << QString::number(Qty);
                            break;
                        }
                    }
                    if(rFact == rowListFact.length()-1) {
                        rowListMain[rMain] << "";
                        rowListMain[rMain] << "";
                        rowListMain[rMain] << "";
                        rowListMain[rMain] << "";
                        rowListMain[rMain] << "";
                        rowListMain[rMain] << "";
                    }
                }
            }
        }
        else {
            rowListMain[rMain] << "";
            rowListMain[rMain] << "";
            rowListMain[rMain] << "";
            rowListMain[rMain] << "";
            rowListMain[rMain] << "";
            rowListMain[rMain] << "";
        }
    }


    //Добавить PRO созданные вручную
    QString ORDER = "";
    for(int rFact = 1; rFact < rowListFact.length(); rFact++) {
         QRegExp rxMaterial("(3400)");
         if(rxMaterial.indexIn(rowListFact[rFact].at(rowListFact[0].indexOf("PLNBEZ"))) == -1) {
             if(rowListFact[rFact].at(rowListFact[0].indexOf("PLNUM")) == "") {
                 QDateTime createdate;
                 QDateTime changedate;
                 createdate = QDateTime::fromString(rowListFact[rFact].at(rowListFact[0].indexOf("ERDAT")) + " " + rowListFact[rFact].at(rowListFact[0].indexOf("ERFZEIT")), "dd.MM.yyyy hh:mm:ss");
                 changedate = QDateTime::fromString(rowListFact[rFact].at(rowListFact[0].indexOf("AEDAT")) + " " + rowListFact[rFact].at(rowListFact[0].indexOf("AEZEIT")), "dd.MM.yyyy hh:mm:ss");
                 if(ORDER.isEmpty() || ORDER != rowListFact[rFact].at(rowListFact[0].indexOf("AUFNR"))) {
                    //Заказы которые созданы в день оценки
                    if(createdate.addSecs(60 * 60 *2) >= QDateTime(selectDate)) {
                         ORDER = rowListFact[rFact].at(rowListFact[0].indexOf("AUFNR"));
                         QStringList s;
                         //StepId
                         s << "SAP"
                         //RunId
                         << "Вручную SAP"
                         //Машина
                         << rowListFact[rFact].at(rowListFact[0].indexOf("MDV01"));
                         //Материал
                         s << rowListFact[rFact].at(rowListFact[0].indexOf("PLNBEZ")).right(9);
                         //Описание материала
                         s << rowListFact[rFact].at(rowListFact[0].indexOf("MATXT"));
                         //Маркер
                         s << "";
                         //Версия
                         s << rowListFact[rFact].at(rowListFact[0].indexOf("VERID"));
                         //Операция
                         s << "";
                         //PLO номер
                         s << "";
                         //PLO Начало
                         s << "";
                         //PLO Конец
                         s << "";
                         //PLO Кол-во
                         s << "";
                         //PRO номер
                         s << QString::number(ORDER.toLongLong());
                         // Статус
                         s << rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4);
                         //Дата Время начала
                         QDateTime dt;
                         dt = QDateTime::fromString(rowListFact[rFact].at(rowListFact[0].indexOf("GSTRP")) + " " + rowListFact[rFact].at(rowListFact[0].indexOf("GSUZP")), "dd.MM.yyyy hh:mm:ss");
                         if(!dt.isValid()) dt = QDateTime::fromString(rowListFact[rFact].at(rowListFact[0].indexOf("GSTRP")) + " 00:00:00", "dd.MM.yyyy hh:mm:ss");
                             s << dt.toString("dd.MM.yyyy hh:mm:ss");
                         //Дата Время окончания
                         dt = QDateTime::fromString(rowListFact[rFact].at(rowListFact[0].indexOf("GLTRP")) + " " + rowListFact[rFact].at(rowListFact[0].indexOf("GLUZP")), "dd.MM.yyyy hh:mm:ss");
                         if(!dt.isValid()) dt = QDateTime::fromString(rowListFact[rFact].at(rowListFact[0].indexOf("GLTRP")) + " 00:00:00", "dd.MM.yyyy hh:mm:ss");
                             s << dt.toString("dd.MM.yyyy hh:mm:ss");

                         //PRO Кол-во
                         float Qty;
                         Qty = rowListFact[rFact].at(rowListFact[0].indexOf("GAMNG_KG")).toFloat();
                         s << QString::number(Qty);

                         //Выпуск
                         Qty = rowListFact[rFact].at(rowListFact[0].indexOf("GWEMG_KG")).toFloat();
                         s << QString::number(Qty);

                         rowListMain.append(s);
                     }
                    //Заказы которые созданы в прошлом, но скорректированы в день оценки
                    else if(createdate.addSecs(60 * 60 *2) <= QDateTime(selectDate) && changedate.addSecs(60 * 60 *2) >= QDateTime(selectDate)) {
                        QStringList s;
                        ORDER = rowListFact[rFact].at(rowListFact[0].indexOf("AUFNR"));
                        //StepId
                        s << "SAP"
                        //RunId
                        << "Вручную SAP"
                        //Машина
                        << rowListFact[rFact].at(rowListFact[0].indexOf("MDV01"));
                        //Материал
                        s << rowListFact[rFact].at(rowListFact[0].indexOf("PLNBEZ")).right(9);
                        //Описание материала
                        s << rowListFact[rFact].at(rowListFact[0].indexOf("MATXT"));

                        //Маркер
                        s << "";

                        //ВИ
                        s << rowListFact[rFact].at(rowListFact[0].indexOf("VERID"));
                        //Операция
                        s << "";

                        //PLO номер
                        s << "";
                        //PLO Начало
                        s << "";
                        //PLO Конец
                        s << "";
                        //PLO Кол-во
                        s << "";
                        //PRO номер
                        s << QString::number(ORDER.toLongLong());
                        //Статус
                        s << rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4);
                        //Дата Время начала
                        QDateTime dt;
                        dt = QDateTime::fromString(rowListFact[rFact].at(rowListFact[0].indexOf("GSTRP")) + " " + rowListFact[rFact].at(rowListFact[0].indexOf("GSUZP")), "dd.MM.yyyy hh:mm:ss");
                        if(!dt.isValid()) dt = QDateTime::fromString(rowListFact[rFact].at(rowListFact[0].indexOf("GSTRP")) + " 00:00:00", "dd.MM.yyyy hh:mm:ss");
                            s << dt.toString("dd.MM.yyyy hh:mm:ss");
                        //Дата Время окончания
                        dt = QDateTime::fromString(rowListFact[rFact].at(rowListFact[0].indexOf("GLTRP")) + " " + rowListFact[rFact].at(rowListFact[0].indexOf("GLUZP")), "dd.MM.yyyy hh:mm:ss");
                        if(!dt.isValid()) dt = QDateTime::fromString(rowListFact[rFact].at(rowListFact[0].indexOf("GLTRP")) + " 00:00:00", "dd.MM.yyyy hh:mm:ss");
                            s << dt.toString("dd.MM.yyyy hh:mm:ss");

                        //Проверить что изменилось в заказе относительно предыдущего снэпшота
                        if(rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "CMPL" ||
                        rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "TECO") {
                            QString ORDER2 = "";
                            for(int rFactPrev = 1; rFactPrev < rowListFactPrevous.length(); rFactPrev++) {
                                if(ORDER2.isEmpty() || ORDER2 != rowListFactPrevous[rFactPrev].at(rowListFactPrevous[0].indexOf("AUFNR"))) {
                                    ORDER2 = rowListFactPrevous[rFactPrev].at(rowListFactPrevous[0].indexOf("AUFNR"));
                                    if(ORDER.toLongLong() == ORDER2.toLongLong()) {

                                        //PRO Кол-во
                                        float Qty = rowListFact[rFact].at(rowListFact[0].indexOf("GAMNG_KG")).toFloat();
                                        float QtyPrev = rowListFactPrevous[rFactPrev].at(rowListFactPrevous[0].indexOf("GAMNG_KG")).toFloat();
                                        s << QString::number(Qty - QtyPrev);

                                        //Поставка
                                        Qty = rowListFact[rFact].at(rowListFact[0].indexOf("GWEMG_KG")).toFloat();
                                        QtyPrev = rowListFactPrevous[rFactPrev].at(rowListFactPrevous[0].indexOf("GWEMG_KG")).toFloat();
                                        //Внести маркер
                                        if((Qty - QtyPrev) != 0) {
                                            s << QString::number(Qty - QtyPrev);
                                            //Маркер для сигнала, что заказ был изменен
                                            s[ColumnHeader.indexOf("Marker")].append("?");
                                        }
                                        else {
                                            s << QString::number(Qty - QtyPrev);
                                            //Маркер для сигнала, что заказ не был изменен
                                            s[ColumnHeader.indexOf("Marker")].append("!");
                                        }
                                        break;
                                    }
                                }
                            }
                        }
                        else {
                            QString ORDER2 = "";
                            for(int rFactPrev = 1; rFactPrev < rowListFactPrevous.length(); rFactPrev++) {
                                if(ORDER2.isEmpty() || ORDER2 != rowListFactPrevous[rFactPrev].at(rowListFactPrevous[0].indexOf("AUFNR"))) {
                                    ORDER2 = rowListFactPrevous[rFactPrev].at(rowListFactPrevous[0].indexOf("AUFNR"));
                                    if(ORDER.toLongLong() == ORDER2.toLongLong()) {

                                        float Qty = rowListFact[rFact].at(rowListFact[0].indexOf("GAMNG_KG")).toFloat();
                                        float QtyPrev = rowListFactPrevous[rFactPrev].at(rowListFactPrevous[0].indexOf("GAMNG_KG")).toFloat();
                                        //PRO Кол-во
                                        s << QString::number(Qty - QtyPrev);

                                        //Поставка
                                        Qty = rowListFact[rFact].at(rowListFact[0].indexOf("GWEMG_KG")).toFloat();
                                        QtyPrev = rowListFactPrevous[rFactPrev].at(rowListFactPrevous[0].indexOf("GWEMG_KG")).toFloat();
                                        s << QString::number(Qty - QtyPrev);
                                        break;
                                    }
                                }
                            }
                        }
                        rowListMain.append(s);
                    }
                 }
             }
         }
     }


    //Поиск плана расхода молока
    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
        const QString cat = "plan";
        const QString type = "milk";
        float SumQty = 0.0;
        for(int rFlow = 1; rFlow < rowListProductFlow.length(); rFlow++) {
            int Pos = rowListProductFlow[rFlow].at(rowListProductFlow[0].indexOf("WhId")).indexOf("/");
            if(rowListProductFlow[rFlow].at(rowListProductFlow[0].indexOf("WhId")).mid(Pos+1) != "$Outside") {
                if(rowListMain[rMain].at(ColumnHeader.indexOf("StepId")) == rowListProductFlow[rFlow].at(rowListProductFlow[0].indexOf("StepId"))) {
                    const QString material = rowListProductFlow[rFlow].at(rowListProductFlow[0].indexOf("ProductId"));
                    const QString plant = rowListProductFlow[rFlow].at(rowListProductFlow[0].indexOf("LocationId"));
                    if(isComponent(&cat, &type, &material, &plant)) {
                        SumQty += rowListProductFlow[rFlow].at(rowListProductFlow[0].indexOf("Quantity")).toFloat();
                    }
                }
            }
            if(rFlow == rowListProductFlow.length()-1) {
                rowListMain[rMain] << QString::number(SumQty);
            }
        }
    }


    //Поиск плана расхода обрата
    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
        const QString cat = "plan";
        const QString type = "skmilk";
        float SumQty = 0.0;
        for(int rFlow = 1; rFlow < rowListProductFlow.length(); rFlow++) {
            int Pos = rowListProductFlow[rFlow].at(rowListProductFlow[0].indexOf("WhId")).indexOf("/");
            if(rowListProductFlow[rFlow].at(rowListProductFlow[0].indexOf("WhId")).mid(Pos+1) != "$Outside") {
                if(rowListMain[rMain].at(ColumnHeader.indexOf("StepId")) == rowListProductFlow[rFlow].at(rowListProductFlow[0].indexOf("StepId"))) {
                    const QString material = rowListProductFlow[rFlow].at(rowListProductFlow[0].indexOf("ProductId"));
                    const QString plant = rowListProductFlow[rFlow].at(rowListProductFlow[0].indexOf("LocationId"));
                    if(isComponent(&cat, &type, &material, &plant)) {
                        SumQty += rowListProductFlow[rFlow].at(rowListProductFlow[0].indexOf("Quantity")).toFloat();
                    }
                }
            }
            if(rFlow == rowListProductFlow.length()-1) {
                rowListMain[rMain] << QString::number(SumQty);
            }
        }
    }


    //Поиск плана расхода сливок
    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
        const QString cat = "plan";
        const QString type = "cream";
        float SumQty = 0.0;
        for(int rFlow = 1; rFlow < rowListProductFlow.length(); rFlow++) {
            int Pos = rowListProductFlow[rFlow].at(rowListProductFlow[0].indexOf("WhId")).indexOf("/");
            if(rowListProductFlow[rFlow].at(rowListProductFlow[0].indexOf("WhId")).mid(Pos+1) != "$Outside") {
                if(rowListMain[rMain].at(ColumnHeader.indexOf("StepId")) == rowListProductFlow[rFlow].at(rowListProductFlow[0].indexOf("StepId"))) {
                    const QString material = rowListProductFlow[rFlow].at(rowListProductFlow[0].indexOf("ProductId"));
                    const QString plant = rowListProductFlow[rFlow].at(rowListProductFlow[0].indexOf("LocationId"));
                    if(isComponent(&cat, &type, &material, &plant)) {
                        SumQty += rowListProductFlow[rFlow].at(rowListProductFlow[0].indexOf("Quantity")).toFloat();
                    }
                }
            }
            if(rFlow == rowListProductFlow.length()-1) {
                rowListMain[rMain] << QString::number(SumQty);
            }
        }
    }


    //Факт молока
    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
        if(rowListMain[rMain].at(ColumnHeader.indexOf("Marker")).indexOf("!") == -1) {
            float SumQtyIn = 0.0;
            float QtyOut = 0.0;
            for(int rFact = 1; rFact < rowListFact.length(); rFact++) {
                bool ok;
                long long ORDER1;
                long long ORDER2;
                ORDER1 = rowListMain[rMain].at(ColumnHeader.indexOf("OrderFact")).toLongLong(&ok, 10);
                if(ok) {
                    ORDER2 = rowListFact[rFact].at(rowListFact[0].indexOf("AUFNR")).toLongLong(&ok, 10);
                    if(ok) {
                        if(ORDER1 == ORDER2) {
                            if(rowListMain[rMain].at(ColumnHeader.indexOf("OperationId")).isEmpty() || rxOperation.indexIn(rowListMain[rMain].at(ColumnHeader.indexOf("OperationId"))) != -1) {
                                //Факт расхода молока
                                const QString cat = "fact";
                                const QString type = "milk";
                                QString material = QString::number(rowListFact[rFact].at(rowListFact[0].indexOf("MATNR_COM")).toInt());
                                if(isComponent(&cat, &type, &material, &selectPlant)) {
                                    if(rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "CMPL" ||
                                    rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "TECO") {
                                        SumQtyIn += -1.0 * rowListFact[rFact].at(rowListFact[0].indexOf("COM_DENMNG_KG")).toFloat();
                                    }
                                    else if(rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "INTL" ||
                                    rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "UNTE") {
                                        float QtyPlan;
                                        float QtyFact;
                                        float QtyDelivFact;
                                        QtyPlan = rowListMain[rMain].at(ColumnHeader.indexOf("QuantityPlan")).toFloat();
                                        QtyFact = rowListMain[rMain].at(ColumnHeader.indexOf("QuantityFact")).toFloat();
                                        QtyDelivFact = rowListMain[rMain].at(ColumnHeader.indexOf("DeliveryFact")).toFloat();
                                        float Proc;
                                        if(QtyPlan) {
                                            if(QtyFact > QtyDelivFact)
                                                Proc = QtyFact * 100 / QtyPlan;
                                            else
                                                Proc = QtyDelivFact * 100 / QtyPlan;
                                            SumQtyIn = rowListMain[rMain].at(ColumnHeader.indexOf("MilkReqPlan")).toFloat() * Proc / 100;
                                        }
                                        else  {
                                            SumQtyIn = 0.01;
                                        }
                                    }
                                }
                                //Факт производства молока
                                material = QString::number(rowListFact[rFact].at(rowListFact[0].indexOf("PLNBEZ")).toInt());
                                if(isComponent(&cat, &type, &material, &selectPlant)) {
                                    if(QtyOut == 0.0) {
                                        if(rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "INTL" ||
                                        rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "UNTE")
                                            QtyOut = rowListFact[rFact].at(rowListFact[0].indexOf("GAMNG_KG")).toFloat();
                                        else if(rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "CMPL" ||
                                        rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "TECO")
                                            QtyOut = rowListFact[rFact].at(rowListFact[0].indexOf("GWEMG_KG")).toFloat();
                                    }
                                }
                            }
                        }

                    }
                }
                if(rFact == rowListFact.length()-1) {
                    rowListMain[rMain] << QString::number(SumQtyIn + QtyOut);
                }
            }
        }
        else {
            rowListMain[rMain] << "0";
        }
    }


    //Факт обрата
    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
        if(rowListMain[rMain].at(ColumnHeader.indexOf("Marker")).indexOf("!") == -1) {
            float SumQtyIn = 0.0;
            float QtyOut = 0.0;
            for(int rFact = 1; rFact < rowListFact.length(); rFact++) {
                bool ok;
                long long ORDER1;
                long long ORDER2;
                ORDER1 = rowListMain[rMain].at(ColumnHeader.indexOf("OrderFact")).toLongLong(&ok, 10);
                if(ok) {
                    ORDER2 = rowListFact[rFact].at(rowListFact[0].indexOf("AUFNR")).toLongLong(&ok, 10);
                    if(ok) {
                        if(ORDER1 == ORDER2) {
                            if(rowListMain[rMain].at(ColumnHeader.indexOf("OperationId")).isEmpty() || rxOperation.indexIn(rowListMain[rMain].at(ColumnHeader.indexOf("OperationId"))) != -1) {
                                //Факт расхода обрата
                                const QString cat = "fact";
                                const QString type = "skmilk";
                                QString material = QString::number(rowListFact[rFact].at(rowListFact[0].indexOf("MATNR_COM")).toInt());
                                if(isComponent(&cat, &type, &material, &selectPlant)) {
                                    if(rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "CMPL" ||
                                    rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "TECO") {
                                        SumQtyIn += -1.0 * rowListFact[rFact].at(rowListFact[0].indexOf("COM_DENMNG_KG")).toFloat();
                                    }
                                    else if(rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "INTL" ||
                                    rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "UNTE") {
                                        float QtyPlan;
                                        float QtyFact;
                                        float QtyDelivFact;
                                        QtyPlan = rowListMain[rMain].at(ColumnHeader.indexOf("QuantityPlan")).toFloat();
                                        QtyFact = rowListMain[rMain].at(ColumnHeader.indexOf("QuantityFact")).toFloat();
                                        QtyDelivFact = rowListMain[rMain].at(ColumnHeader.indexOf("DeliveryFact")).toFloat();
                                        float Proc;
                                        if(QtyPlan) {
                                            if(QtyFact > QtyDelivFact)
                                                Proc = QtyFact * 100 / QtyPlan;
                                            else
                                                Proc = QtyDelivFact * 100 / QtyPlan;
                                            SumQtyIn = rowListMain[rMain].at(ColumnHeader.indexOf("SkMilkReqPlan")).toFloat() * Proc / 100;
                                        }
                                        else  {
                                            SumQtyIn = 0.01;
                                        }
                                    }
                                }
                                //Факт производства обрата
                                material = QString::number(rowListFact[rFact].at(rowListFact[0].indexOf("PLNBEZ")).toInt());
                                if(isComponent(&cat, &type, &material, &selectPlant)) {
                                    if(QtyOut == 0.0) {
                                        if(rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "INTL" ||
                                        rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "UNTE")
                                            QtyOut = rowListFact[rFact].at(rowListFact[0].indexOf("GAMNG_KG")).toFloat();
                                        else if(rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "CMPL" ||
                                        rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "TECO")
                                            QtyOut = rowListFact[rFact].at(rowListFact[0].indexOf("GWEMG_KG")).toFloat();
                                    }
                                }
                            }
                        }
                    }
                }
                if(rFact == rowListFact.length()-1) {
                    rowListMain[rMain] << QString::number(SumQtyIn + QtyOut);
                }
            }
        }
        else {
            rowListMain[rMain] << "0";
        }
    }


    //Факт сливок
    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
        if(rowListMain[rMain].at(ColumnHeader.indexOf("Marker")).indexOf("!") == -1) {
            float SumQtyIn = 0.0;
            float QtyOut = 0.0;
            for(int rFact = 1; rFact < rowListFact.length(); rFact++) {
                bool ok;
                long long ORDER1;
                long long ORDER2;
                ORDER1 = rowListMain[rMain].at(ColumnHeader.indexOf("OrderFact")).toLongLong(&ok, 10);
                if(ok) {
                    ORDER2 = rowListFact[rFact].at(rowListFact[0].indexOf("AUFNR")).toLongLong(&ok, 10);
                    if(ok) {
                        if(ORDER1 == ORDER2) {
                            if(rowListMain[rMain].at(ColumnHeader.indexOf("OperationId")).isEmpty() || rxOperation.indexIn(rowListMain[rMain].at(ColumnHeader.indexOf("OperationId"))) != -1) {
                                //Факт расхода сливок
                                const QString cat = "fact";
                                const QString type = "cream";
                                QString material = QString::number(rowListFact[rFact].at(rowListFact[0].indexOf("MATNR_COM")).toInt());
                                if(isComponent(&cat, &type, &material, &selectPlant)) {
                                    if(rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "CMPL" ||
                                    rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "TECO") {
                                        SumQtyIn += -1.0 * rowListFact[rFact].at(rowListFact[0].indexOf("COM_DENMNG_KG")).toFloat();
                                    }
                                    else if(rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "INTL" ||
                                    rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "UNTE") {
                                        float QtyPlan;
                                        float QtyFact;
                                        float QtyDelivFact;
                                        QtyPlan = rowListMain[rMain].at(ColumnHeader.indexOf("QuantityPlan")).toFloat();
                                        QtyFact = rowListMain[rMain].at(ColumnHeader.indexOf("QuantityFact")).toFloat();
                                        QtyDelivFact = rowListMain[rMain].at(ColumnHeader.indexOf("DeliveryFact")).toFloat();
                                        float Proc;
                                        if(QtyPlan) {
                                            if(QtyFact > QtyDelivFact)
                                                Proc = QtyFact * 100 / QtyPlan;
                                            else
                                                Proc = QtyDelivFact * 100 / QtyPlan;
                                            SumQtyIn = rowListMain[rMain].at(ColumnHeader.indexOf("CreamReqPlan")).toFloat() * Proc / 100;
                                        }
                                        else  {
                                            SumQtyIn = 0.01;
                                        }
                                    }
                                }
                                //Факт производства сливок
                                material = QString::number(rowListFact[rFact].at(rowListFact[0].indexOf("PLNBEZ")).toInt());
                                if(isComponent(&cat, &type, &material, &selectPlant)) {
                                    if(QtyOut == 0.0) {
                                        if(rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "INTL" ||
                                        rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "UNTE")
                                            QtyOut = rowListFact[rFact].at(rowListFact[0].indexOf("GAMNG_KG")).toFloat();
                                        else if(rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "CMPL" ||
                                        rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "TECO")
                                            QtyOut = rowListFact[rFact].at(rowListFact[0].indexOf("GWEMG_KG")).toFloat();
                                    }
                                }
                            }
                        }
                    }
                }
                if(rFact == rowListFact.length()-1) {
                    rowListMain[rMain] << QString::number(SumQtyIn + QtyOut);
                }
            }
        }
        else {
            rowListMain[rMain] << "0";
        }
    }


    //Поставки молока
    //Добавить пустую запись
    QStringList lstr;
    for(int i = 0; i < rowListMain[0].length(); i++) {
        lstr.append("");
    }
    rowListMain.append(lstr);

    QStringList strlist;
    strlist << "MilkReqPlan" << "MilkReqFact"
            <<"SkMilkReqPlan" << "SkMilkReqFact"
            << "CreamReqPlan" << "CreamReqFact";

    //Обнулить цифровые значения в строке
    foreach(QString c, strlist) {
        rowListMain[rowListMain.length()-1][ColumnHeader.indexOf(c)] = "0";
    }

    float Qty = 0.0;
    for(int rStock = 1; rStock < rowListStock.length(); rStock++) {
        const QString cat = "plan";
        const QString type = "milk";
        const QString mat = rowListStock[rStock].at(rowListStock[0].indexOf("ProductId"));
        if(isComponent(&cat, &type, &mat, &selectPlant)) {
            if(rowListStock[rStock].at(rowListStock[0].indexOf("StockType")) == "movement") {
                if(rowListStock[rStock].at(rowListStock[0].indexOf("ToWhId")).left(4) == selectPlant) {
                    QDateTime dt;
                    dt = QDateTime::fromString(rowListStock[rStock].at(rowListStock[0].indexOf("StartDate")), "yyyy-MM-dd hh:mm:ss");
                    if(dt < QDateTime(selectDate.addDays(1))) {
                        Qty += rowListStock[rStock].at(rowListStock[0].indexOf("Quantity")).toFloat();
                    }
                }
            }
        }
    }
    rowListMain[rowListMain.length()-1][ColumnHeader.indexOf("ProductLocationId")] = "milk";
    //rowListMain[rowListMain.length()-1][ColumnHeader.indexOf("StartPlan")] = selectDate.toString("dd.MM.yyyy 00:00:00");
    rowListMain[rowListMain.length()-1][ColumnHeader.indexOf("EndPlan")] = selectDate.toString("dd.MM.yyyy 23:59:59");
    rowListMain[rowListMain.length()-1][ColumnHeader.indexOf("OrderPlan")] = "ПРИВОЗ";
    rowListMain[rowListMain.length()-1][ColumnHeader.indexOf("QuantityPlan")] = QString::number(Qty);
    rowListMain[rowListMain.length()-1][ColumnHeader.indexOf("MilkReqPlan")] = QString::number(Qty);

    //Поставки обрата
    //Добавить пустую запись
    rowListMain.append(lstr);

    //Обнулить цифровые значения в строке
    foreach(QString c, strlist) {
        rowListMain[rowListMain.length()-1][ColumnHeader.indexOf(c)] = "0";
    }
    Qty = 0.0;
    for(int rStock = 1; rStock < rowListStock.length(); rStock++) {
        const QString cat = "plan";
        const QString type = "skmilk";
        const QString mat = rowListStock[rStock].at(rowListStock[0].indexOf("ProductId"));
        if(isComponent(&cat, &type, &mat, &selectPlant)) {
            if(rowListStock[rStock].at(rowListStock[0].indexOf("StockType")) == "movement") {
                if(rowListStock[rStock].at(rowListStock[0].indexOf("ToWhId")).left(4) == selectPlant) {
                    QDateTime dt;
                    dt = QDateTime::fromString(rowListStock[rStock].at(rowListStock[0].indexOf("StartDate")), "yyyy-MM-dd hh:mm:ss");
                    if(dt < QDateTime(selectDate.addDays(1))) {
                        Qty += rowListStock[rStock].at(rowListStock[0].indexOf("Quantity")).toFloat();
                    }
                }
            }
        }
    }
    rowListMain[rowListMain.length()-1][ColumnHeader.indexOf("ProductLocationId")] = "skimmed milk";
    //rowListMain[rowListMain.length()-1][ColumnHeader.indexOf("StartPlan")] = selectDate.toString("dd.MM.yyyy 00:00:00");
    rowListMain[rowListMain.length()-1][ColumnHeader.indexOf("EndPlan")] = selectDate.toString("dd.MM.yyyy 23:59:59");
    rowListMain[rowListMain.length()-1][ColumnHeader.indexOf("OrderPlan")] = "ПРИВОЗ";
    rowListMain[rowListMain.length()-1][ColumnHeader.indexOf("QuantityPlan")] = QString::number(Qty);
    rowListMain[rowListMain.length()-1][ColumnHeader.indexOf("SkMilkReqPlan")] = QString::number(Qty);


    //Поставки сливок
    //Добавить пустую запись
    rowListMain.append(lstr);

    //Обнулить цифровые значения в строке
    foreach(QString c, strlist) {
        rowListMain[rowListMain.length()-1][ColumnHeader.indexOf(c)] = "0";
    }
    Qty = 0.0;
    for(int rStock = 1; rStock < rowListStock.length(); rStock++) {
        const QString cat = "plan";
        const QString type = "cream";
        const QString mat = rowListStock[rStock].at(rowListStock[0].indexOf("ProductId"));
        if(isComponent(&cat, &type, &mat, &selectPlant)) {
            if(rowListStock[rStock].at(rowListStock[0].indexOf("StockType")) == "movement") {
                if(rowListStock[rStock].at(rowListStock[0].indexOf("ToWhId")).left(4) == selectPlant) {
                    QDateTime dt;
                    dt = QDateTime::fromString(rowListStock[rStock].at(rowListStock[0].indexOf("StartDate")), "yyyy-MM-dd hh:mm:ss");
                    if(dt < QDateTime(selectDate.addDays(1))) {
                        Qty += rowListStock[rStock].at(rowListStock[0].indexOf("Quantity")).toFloat();
                    }
                }
            }
        }
    }
    rowListMain[rowListMain.length()-1][ColumnHeader.indexOf("ProductLocationId")] = "Сливки";
    //rowListMain[rowListMain.length()-1][ColumnHeader.indexOf("StartPlan")] = selectDate.toString("dd.MM.yyyy 00:00:00");
    rowListMain[rowListMain.length()-1][ColumnHeader.indexOf("EndPlan")] = selectDate.toString("dd.MM.yyyy 23:59:59");
    rowListMain[rowListMain.length()-1][ColumnHeader.indexOf("OrderPlan")] = "ПРИВОЗ";
    rowListMain[rowListMain.length()-1][ColumnHeader.indexOf("QuantityPlan")] = QString::number(Qty);
    rowListMain[rowListMain.length()-1][ColumnHeader.indexOf("CreamReqPlan")] = QString::number(Qty);

    //Фильтр по молочным компонентам
    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
        if(rowListMain[rMain].at(ColumnHeader.indexOf("MilkReqPlan")) == "0" &&
           rowListMain[rMain].at(ColumnHeader.indexOf("SkMilkReqPlan")) == "0" &&
           rowListMain[rMain].at(ColumnHeader.indexOf("CreamReqPlan")) == "0" &&
           rowListMain[rMain].at(ColumnHeader.indexOf("MilkReqFact")) == "0" &&
           rowListMain[rMain].at(ColumnHeader.indexOf("SkMilkReqFact")) == "0" &&
           rowListMain[rMain].at(ColumnHeader.indexOf("CreamReqFact")) == "0" ) {
                rowListMain.removeAt(rMain);
                rMain--;
        }
    }

    //Фильтр по датам
    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
        QDateTime startPlan;
        QDateTime startFact;
        startPlan = QDateTime::fromString(rowListMain[rMain].at(ColumnHeader.indexOf("StartPlan")), "dd.MM.yyyy hh:mm:ss");
        startFact = QDateTime::fromString(rowListMain[rMain].at(ColumnHeader.indexOf("StartFact")), "dd.MM.yyyy hh:mm:ss");
        if(!rowListMain[rMain].at(ColumnHeader.indexOf("OrderPlan")).isEmpty() && rowListMain[rMain].at(ColumnHeader.indexOf("OrderFact")).isEmpty()) {
            if(startPlan >= QDateTime(selectDate.addDays(1))) {
                rowListMain.removeAt(rMain);
                rMain--;
            }
        }
        else if(!rowListMain[rMain].at(ColumnHeader.indexOf("OrderPlan")).isEmpty() && !rowListMain[rMain].at(ColumnHeader.indexOf("OrderFact")).isEmpty()) {
            if(startPlan >= QDateTime(selectDate.addDays(1)) && startFact >= QDateTime(selectDate.addDays(1))) {
                rowListMain.removeAt(rMain);
                rMain--;
            }
        }
        else if(rowListMain[rMain].at(ColumnHeader.indexOf("OrderPlan")).isEmpty() && !rowListMain[rMain].at(ColumnHeader.indexOf("OrderFact")).isEmpty()) {
            if(rowListMain[rMain].at(ColumnHeader.indexOf("Status")) == "INTL" || rowListMain[rMain].at(ColumnHeader.indexOf("Status")) == "UNTE") {
                if(startFact >= QDateTime(selectDate.addDays(1))) {
                    rowListMain.removeAt(rMain);
                    rMain--;
                }
            }
        }
        else {
            if(startPlan >= QDateTime(selectDate.addDays(1))) {
                rowListMain.removeAt(rMain);
                rMain--;
            }
        }
    }

    //Срез по времени
    slicetime();

    //Дельта по запасу
    deltaTotalStock("milk");
    deltaTotalStock("skmilk");
    deltaTotalStock("cream");

    //Сортировка
    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
        for(int i = rMain; i < rowListMain.length(); i++) {
            if(rowListMain[rMain].at(ColumnHeader.indexOf("ProductLocationId")) > rowListMain[i].at(ColumnHeader.indexOf("ProductLocationId"))) {
                QStringList buffer;
                buffer = rowListMain[i];
                rowListMain[i] = rowListMain[rMain];
                rowListMain[rMain] = buffer;
            }
        }
    }

    //Итоговый расход
    addTotalReq();

    //Добавить промежуточные итоги
    addSubTotalRow();

    //Добавить итоги
    addTotalRow();

    labelstatus->setText("План: " + QFileInfo(pathfileplan).birthTime().toString("dd.MM.yyyy hh:mm:ss") + "\t" +
                         + "Факт: " + selectDate.addDays(1).toString("dd.MM.yyyy 00:00:00") + "\t");

    TableModelMain *model = new TableModelMain();
    model->rowList.append(rowListMain);
    model->Plant = &selectPlant;
    model->CheckDate = &selectDate;
    model->MilkCriteria = &MilkCriteria;
    model->SkMilkCriteria = &SkMilkCriteria;
    model->CreamCriteria = &CreamCriteria;

    ui->tableView->setModel(model);
    ui->tableView->hideColumn(0);
    ui->tableView->hideColumn(1);
    ui->tableView->hideColumn(5);
    ui->tableView->hideColumn(7);

    //Размеры
    ui->tableView->resizeColumnsToContents();
    ui->tableView->resizeRowsToContents();
    ui->tableView->setColumnWidth(TableModelMain::Column::Description, 200);
    for(int i = 0; i < rowListMain.length(); i++)
        ui->tableView->setRowHeight(i, 10);
    ui->tableView->update();

    return true;
}


void MainWindow::resizeEvent(QResizeEvent*)
{
    QRect r;
    r = this->rect();
    r.moveTop(50);
    r.moveLeft(10);
    r.setRight(r.right()-20);
    r.setBottom(r.bottom()-100);
    ui->tableView->setGeometry(r);
}


void MainWindow::on_action_About_triggered()
{
    QMessageBox::about(0, "О программе", "Анализ расхода молочного сырья. " + version + "\n"
                                         "Developed by: Gribov Artem");
}


//void MainWindow::on_action_MasterData_triggered()
//{
//    masterdata window;
//    window.setModal(true);
//    window.exec();
//}

void MainWindow::addTotalReq() {
    //Добавить пустую запись
    QStringList s;
    for(int i = 0; i < ColumnHeader.length(); i++) {
        s.append("");
    }
    rowListMain.append(s);
    rowListMain.append(s);

    QStringList strlist;
    strlist << "MilkReqPlan" << "MilkReqFact" << "MilkDelta"
            << "SkMilkReqPlan" << "SkMilkReqFact" << "SkMilkDelta"
            << "CreamReqPlan" << "CreamReqFact" << "CreamDelta";
    //Обнулить цифровые значения в строке
    foreach(QString c, strlist) {
        float QtyMinus = 0;
        float QtyPlus = 0;
        for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
            if(rowListMain[rMain].at(ColumnHeader.indexOf(c)).left(1) == "-") {
                QtyMinus += rowListMain[rMain].at(ColumnHeader.indexOf(c)).toFloat();
            }
            else {
                QtyPlus += rowListMain[rMain].at(ColumnHeader.indexOf(c)).toFloat();
            }
        }

        rowListMain[rowListMain.length()-2][ColumnHeader.indexOf("DeliveryFact")] = "Расход:";
        rowListMain[rowListMain.length()-2][ColumnHeader.indexOf(c)] = QString::number(QtyMinus / 1000, 'f', 1) + "т.";

        rowListMain[rowListMain.length()-1][ColumnHeader.indexOf("DeliveryFact")] = "Приход:";
        rowListMain[rowListMain.length()-1][ColumnHeader.indexOf(c)] = QString::number(QtyPlus / 1000, 'f', 1) + "т.";
    }
}


void MainWindow::deltaTotalStock(QString type) {
    //Дельта по молоку
    if(type == "milk") {
        for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
            float QtyPlan = 0;
            float QtyFact = 0;
            QtyPlan = rowListMain[rMain].at(ColumnHeader.indexOf("MilkReqPlan")).toFloat();
            QtyFact = rowListMain[rMain].at(ColumnHeader.indexOf("MilkReqFact")).toFloat();
            rowListMain[rMain] << QString::number(-1.0*(QtyPlan - QtyFact));
        }
    }
    //Дельта по обрату
    if(type == "skmilk") {
        for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
            float QtyPlan = 0;
            float QtyFact = 0;
            QtyPlan = rowListMain[rMain].at(ColumnHeader.indexOf("SkMilkReqPlan")).toFloat();
            QtyFact = rowListMain[rMain].at(ColumnHeader.indexOf("SkMilkReqFact")).toFloat();
            rowListMain[rMain] << QString::number(-1.0*(QtyPlan - QtyFact));
        }
    }
    //Дельта по сливкам
    if(type == "cream") {
        for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
            float QtyPlan = 0;
            float QtyFact = 0;
            QtyPlan = rowListMain[rMain].at(ColumnHeader.indexOf("CreamReqPlan")).toFloat();
            QtyFact = rowListMain[rMain].at(ColumnHeader.indexOf("CreamReqFact")).toFloat();
            rowListMain[rMain] << QString::number(-1.0*(QtyPlan - QtyFact));
        }
    }
}


//Проверка на принадлежность отслеживаемому компоненту
bool MainWindow::isComponent(const QString *Category, const QString *Type, const QString *Material, const QString *Plant) {
    if(*Category == "plan") {
        //int Pos = Material.indexOf("/");
        if(Plant == selectPlant) {
            if(*Type == "milk") {
                foreach(QString m, listMilkPlan) {
                    if(Material == m) return true;
                }
            }
            else if(*Type == "skmilk") {
                foreach(QString m, listSkMilkPlan) {
                    if(Material == m) return true;
                }
            }
            else if(*Type == "cream") {
                foreach(QString m, listCreamPlan) {
                    if(Material == m) return true;
                }
            }
        }
    }
    else if(*Category == "fact") {
        if(*Type == "milk") {
            foreach(QString m, listMilkFact) {
                if(Material == m) return true;
            }
        }
        if(*Type == "skmilk") {
            foreach(QString m, listSkMilkFact) {
                if(Material == m) return true;
            }
        }
        if(*Type == "cream") {
            foreach(QString m, listCreamFact) {
                if(Material == m) return true;
            }
        }
    }
    return false;
}


//Промежуточные итоги
void MainWindow::addSubTotalRow() {
    for(int rMain = 0; rMain < rowListMain.length()-1; rMain++) {
        if(rowListMain[rMain].at(ColumnHeader.indexOf("ProductLocationId")) != rowListMain[rMain+1].at(ColumnHeader.indexOf("ProductLocationId"))) {
            //Добавить пустую запись
            QStringList lstr;
            for(int i = 0; i < ColumnHeader.length(); i++) {
                lstr.append("");
            }
            rowListMain.insert(rMain+1, lstr);

            QStringList strlist;
            strlist << "MilkReqPlan" << "MilkReqFact" <<"SkMilkReqPlan"
                    << "SkMilkReqFact" << "CreamReqPlan" << "CreamReqFact"
                    << "MilkDelta" << "SkMilkDelta" << "CreamDelta";
            //Обнулить цифровые значения в строке
            foreach(QString s, strlist) {
                rowListMain[rMain+1][ColumnHeader.indexOf(s)] = "0";
            }

            float QtyTotalMilkPlan = 0;
            float QtyTotalMilkFact = 0;
            float QtyTotalSkMilkPlan = 0;
            float QtyTotalSkMilkFact = 0;
            float QtyTotalCreamPlan = 0;
            float QtyTotalCreamFact = 0;
            float QtyTotalMilkDelta = 0;
            float QtyTotalSkMilkDelta = 0;
            float QtyTotalCreamDelta = 0;

            for(int i = rMain; i >= 0; i--) {
                if(!rowListMain[i].at(ColumnHeader.indexOf("ProductLocationId")).isEmpty()) {
                    QtyTotalMilkPlan += rowListMain[i].at(ColumnHeader.indexOf("MilkReqPlan")).toFloat();
                    QtyTotalMilkFact += rowListMain[i].at(ColumnHeader.indexOf("MilkReqFact")).toFloat();
                    QtyTotalSkMilkPlan += rowListMain[i].at(ColumnHeader.indexOf("SkMilkReqPlan")).toFloat();
                    QtyTotalSkMilkFact += rowListMain[i].at(ColumnHeader.indexOf("SkMilkReqFact")).toFloat();
                    QtyTotalCreamPlan += rowListMain[i].at(ColumnHeader.indexOf("CreamReqPlan")).toFloat();
                    QtyTotalCreamFact += rowListMain[i].at(ColumnHeader.indexOf("CreamReqFact")).toFloat();
                    QtyTotalMilkDelta += rowListMain[i].at(ColumnHeader.indexOf("MilkDelta")).toFloat();
                    QtyTotalSkMilkDelta += rowListMain[i].at(ColumnHeader.indexOf("SkMilkDelta")).toFloat();
                    QtyTotalCreamDelta += rowListMain[i].at(ColumnHeader.indexOf("CreamDelta")).toFloat();
                }
                else break;
            }
            rowListMain[rMain+1][ColumnHeader.indexOf("MilkReqPlan")] = QString::number(QtyTotalMilkPlan);
            rowListMain[rMain+1][ColumnHeader.indexOf("MilkReqFact")] = QString::number(QtyTotalMilkFact);
            rowListMain[rMain+1][ColumnHeader.indexOf("SkMilkReqPlan")] = QString::number(QtyTotalSkMilkPlan);
            rowListMain[rMain+1][ColumnHeader.indexOf("SkMilkReqFact")] = QString::number(QtyTotalSkMilkFact);
            rowListMain[rMain+1][ColumnHeader.indexOf("CreamReqPlan")] = QString::number(QtyTotalCreamPlan);
            rowListMain[rMain+1][ColumnHeader.indexOf("CreamReqFact")] = QString::number(QtyTotalCreamFact);
            rowListMain[rMain+1][ColumnHeader.indexOf("MilkDelta")] = QString::number(QtyTotalMilkDelta);
            rowListMain[rMain+1][ColumnHeader.indexOf("SkMilkDelta")] = QString::number(QtyTotalSkMilkDelta);
            rowListMain[rMain+1][ColumnHeader.indexOf("CreamDelta")] = QString::number(QtyTotalCreamDelta);
            rMain++;
        }
    }
}


//Итоги
void MainWindow::addTotalRow() {
    //Добавить пустую запись
    QStringList s;
    for(int i = 0; i < ColumnHeader.length(); i++) {
        s.append("");
    }
    rowListMain.append(s);
    rowListMain[rowListMain.length()-1][ColumnHeader.indexOf("DeliveryFact")] = "Итог:";

    QStringList strlist;
    strlist << "MilkReqPlan" << "MilkReqFact"
            <<"SkMilkReqPlan" << "SkMilkReqFact"
            << "CreamReqPlan" << "CreamReqFact"
            << "MilkDelta" << "SkMilkDelta" << "CreamDelta";
    //Обнулить цифровые значения в строке
    foreach(QString s, strlist) {
        rowListMain[rowListMain.length()-1][ColumnHeader.indexOf(s)] = "0";
    }

    float QtyTotal = 0;
    QtyTotal = 0;
    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
        if(!rowListMain[rMain].at(ColumnHeader.indexOf("ProductLocationId")).isEmpty()) {
            QtyTotal += rowListMain[rMain].at(ColumnHeader.indexOf("MilkReqPlan")).toFloat();
        }
    }
    rowListMain[rowListMain.length()-1][ColumnHeader.indexOf("MilkReqPlan")] = QString::number(QtyTotal / 1000, 'f', 1) + "т.";


    QtyTotal = 0;
    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
        if(!rowListMain[rMain].at(ColumnHeader.indexOf("ProductLocationId")).isEmpty()) {
            QtyTotal += rowListMain[rMain].at(ColumnHeader.indexOf("MilkReqFact")).toFloat();
        }
    }
    rowListMain[rowListMain.length()-1][ColumnHeader.indexOf("MilkReqFact")] = QString::number(QtyTotal / 1000, 'f', 1) + "т.";


    QtyTotal = 0;
    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
        if(!rowListMain[rMain].at(ColumnHeader.indexOf("ProductLocationId")).isEmpty()) {
            QtyTotal += rowListMain[rMain].at(ColumnHeader.indexOf("SkMilkReqPlan")).toFloat();
        }
    }
    rowListMain[rowListMain.length()-1][ColumnHeader.indexOf("SkMilkReqPlan")] = QString::number(QtyTotal / 1000, 'f', 1) + "т.";

    QtyTotal = 0;
    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
        if(!rowListMain[rMain].at(ColumnHeader.indexOf("ProductLocationId")).isEmpty()) {
            QtyTotal += rowListMain[rMain].at(ColumnHeader.indexOf("SkMilkReqFact")).toFloat();
        }
    }
    rowListMain[rowListMain.length()-1][ColumnHeader.indexOf("SkMilkReqFact")] = QString::number(QtyTotal / 1000, 'f', 1) + "т.";

    QtyTotal = 0;
    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
        if(!rowListMain[rMain].at(ColumnHeader.indexOf("ProductLocationId")).isEmpty()) {
            QtyTotal += rowListMain[rMain].at(ColumnHeader.indexOf("CreamReqPlan")).toFloat();
        }
    }
    rowListMain[rowListMain.length()-1][ColumnHeader.indexOf("CreamReqPlan")] = QString::number(QtyTotal / 1000, 'f', 1) + "т.";


    QtyTotal = 0;
    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
        if(!rowListMain[rMain].at(ColumnHeader.indexOf("ProductLocationId")).isEmpty()) {
            QtyTotal += rowListMain[rMain].at(ColumnHeader.indexOf("CreamReqFact")).toFloat();
        }
    }
    rowListMain[rowListMain.length()-1][ColumnHeader.indexOf("CreamReqFact")] = QString::number(QtyTotal / 1000, 'f', 1) + "т.";


    QtyTotal = 0;
    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
        if(!rowListMain[rMain].at(ColumnHeader.indexOf("ProductLocationId")).isEmpty()) {
            QtyTotal += rowListMain[rMain].at(ColumnHeader.indexOf("MilkDelta")).toFloat();
        }
    }
    rowListMain[rowListMain.length()-1][ColumnHeader.indexOf("MilkDelta")] = QString::number(QtyTotal / 1000, 'f', 1) + "т.";


    QtyTotal = 0;
    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
        if(!rowListMain[rMain].at(ColumnHeader.indexOf("ProductLocationId")).isEmpty()) {
            QtyTotal += rowListMain[rMain].at(ColumnHeader.indexOf("SkMilkDelta")).toFloat();
        }
    }
    rowListMain[rowListMain.length()-1][ColumnHeader.indexOf("SkMilkDelta")] = QString::number(QtyTotal / 1000, 'f', 1) + "т.";


    QtyTotal = 0;
    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
        if(!rowListMain[rMain].at(ColumnHeader.indexOf("ProductLocationId")).isEmpty()) {
            QtyTotal += rowListMain[rMain].at(ColumnHeader.indexOf("CreamDelta")).toFloat();
        }
    }
    rowListMain[rowListMain.length()-1][ColumnHeader.indexOf("CreamDelta")] = QString::number(QtyTotal / 1000, 'f', 1) + "т.";
}


//Загрузка списка молочных компонентов для отслеживания
bool MainWindow::loadCompopentsData() {

    if(pathFact.isEmpty()) {
        qDebug() << "Error. Path fact no set";
        QMessageBox::warning(0,"Ошибка", "Не найдена директория с фактическими данными. Пожалуйста выполните Поиск через меню.");
        labelstatus->setText("Ошибка");
        return false;
    }

    QString pathData;
    pathData = pathFact + "/planfactmilk/components.cfg";
    QFile f(pathData);
    if(QFile::exists(pathData)) {
        if(!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "Error read components.cfg";
            QMessageBox::warning(0,"Ошибка", "Невозможно прочитать файл components.cfg");
            labelstatus->setText("Ошибка");
            return false;
        }

        QTextStream stream(&f);
        stream.setCodec("UTF-8");

        //Прочитать первую строку в заголовок
        QStringList listHead;
        if(!stream.atEnd()) {
            QString str = stream.readLine();
            listHead = str.split('\t');
        }
        //Чтение
        while (!stream.atEnd()) {
            QString str = stream.readLine();
            if(!str.isEmpty()) {
                QStringList strlist;
                strlist = str.split('\t');

                if(!strlist.isEmpty()) {
                    if(strlist.at(listHead.indexOf("Plant")) == selectPlant) {
                        if(strlist.at(listHead.indexOf("Type")) == "milk") {
                            if(strlist.at(listHead.indexOf("Category")) == "plan") {
                                listMilkPlan << strlist[3];
                            }
                            if(strlist.at(listHead.indexOf("Category")) == "fact") {
                                listMilkFact << strlist[3];
                            }
                        }
                        if(strlist.at(listHead.indexOf("Type")) == "skmilk") {
                            if(strlist.at(listHead.indexOf("Category")) == "plan") {
                                listSkMilkPlan << strlist[3];
                            }
                            if(strlist.at(listHead.indexOf("Category")) == "fact") {
                                listSkMilkFact << strlist[3];
                            }
                        }
                        if(strlist.at(listHead.indexOf("Type")) == "cream") {
                            if(strlist.at(listHead.indexOf("Category")) == "plan") {
                                listCreamPlan << strlist[3];
                            }
                            if(strlist.at(listHead.indexOf("Category")) == "fact") {
                                listCreamFact << strlist[3];
                            }
                        }
                    }
                }
            }
        }
        f.close();
    }
    else {
        QMessageBox::warning(0,"Ошибка", "Не найден файл components.cfg");
        labelstatus->setText("Ошибка");
        return false;
    }
    return true;
}


//Загрузка нормативных критериев
bool MainWindow::loadCriteria() {

    if(pathFact.isEmpty()) {
        qDebug() << "Error. Path fact no set";
        QMessageBox::warning(0,"Ошибка", "Не найдена директория с фактическими данными. Пожалуйста выполните Поиск через меню.");
        labelstatus->setText("Ошибка");
        return false;
    }

    QString pathData;
    pathData = pathFact + "/planfactmilk/criteria.cfg";
    QFile f(pathData);
    if(QFile::exists(pathData)) {
        if(!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "Error read file criteria.cfg";
            QMessageBox::warning(0,"Ошибка", "Невозможно прочитать файл criteria.cfg");
            labelstatus->setText("Ошибка");
            return false;
        }

        QTextStream stream(&f);
        stream.setCodec("UTF-8");

        //Прочитать первую строку в заголовок
        QStringList listHead;
        if(!stream.atEnd()) {
            QString str = stream.readLine();
            listHead = str.split('\t');
        }
        //Чтение
        while (!stream.atEnd()) {
            QString str = stream.readLine();
            if(!str.isEmpty()) {
                QStringList strlist;
                strlist = str.split('\t');

                if(!strlist.isEmpty()) {
                    if(strlist.at(listHead.indexOf("Plant")) == selectPlant) {
                        if(strlist.at(listHead.indexOf("Type")) == "milk") {
                            MilkCriteria = strlist[2].toFloat();
                        }
                        if(strlist.at(listHead.indexOf("Type")) == "skmilk") {
                            SkMilkCriteria = strlist[2].toFloat();
                        }
                        if(strlist.at(listHead.indexOf("Type")) == "cream") {
                            CreamCriteria = strlist[2].toFloat();
                        }
                    }
                }
            }
        }
        f.close();
    }
    else {
        QMessageBox::warning(0,"Ошибка", "Не найден файл criteria.cfg");
        labelstatus->setText("Ошибка");
        return false;
    }
    return true;
}


//Срез по времени
void MainWindow::slicetime() {
    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
        if(!rowListMain[rMain].at(ColumnHeader.indexOf("OrderPlan")).isEmpty()) {
            QDateTime start;
            QDateTime end;
            start = QDateTime::fromString(rowListMain[rMain].at(ColumnHeader.indexOf("StartPlan")), "dd.MM.yyyy hh:mm:ss");
            end = QDateTime::fromString(rowListMain[rMain].at(ColumnHeader.indexOf("EndPlan")), "dd.MM.yyyy hh:mm:ss");
            if(start < QDateTime(selectDate.addDays(1)) && end > QDateTime(selectDate.addDays(1))) {
                float Qty;
                int Duration;
                float QtyInSec;

                //Общая длительность заказа
                Duration = start.secsTo(end);

                //Плановое кол-во
                //Qty = rowListMain[rMain].at(ColumnHeader.indexOf("QuantityPlan")).toFloat();
                //QtyInSec = 1 * Qty / Duration;
                //Qty = start.secsTo(QDateTime(selectDate.addDays(1))) * QtyInSec;
                //rowListMain[rMain][ColumnHeader.indexOf("QuantityPlan")] = QString::number(Qty);

                //Плановый расход молока
                Qty = rowListMain[rMain].at(ColumnHeader.indexOf("MilkReqPlan")).toFloat();
                QtyInSec = 1 * Qty / Duration;
                Qty = start.secsTo(QDateTime(selectDate.addDays(1))) * QtyInSec;
                rowListMain[rMain][ColumnHeader.indexOf("MilkReqPlan")] = QString::number(Qty);

                //Плановый расход обрата
                Qty = rowListMain[rMain].at(ColumnHeader.indexOf("SkMilkReqPlan")).toFloat();
                QtyInSec = 1 * Qty / Duration;
                Qty = start.secsTo(QDateTime(selectDate.addDays(1))) * QtyInSec;
                rowListMain[rMain][ColumnHeader.indexOf("SkMilkReqPlan")] = QString::number(Qty);

                //Плановый расход сливок
                Qty = rowListMain[rMain].at(ColumnHeader.indexOf("CreamReqPlan")).toFloat();
                QtyInSec = 1 * Qty / Duration;
                Qty = start.secsTo(QDateTime(selectDate.addDays(1))) * QtyInSec;
                rowListMain[rMain][ColumnHeader.indexOf("CreamReqPlan")] = QString::number(Qty);
            }
            if(start >= QDateTime(selectDate.addDays(1))) {
                //Плановый расход молока
                rowListMain[rMain][ColumnHeader.indexOf("MilkReqPlan")] = "0";
                //Плановый расход обрата
                rowListMain[rMain][ColumnHeader.indexOf("SkMilkReqPlan")] = "0";
                //Плановый расход сливок
                rowListMain[rMain][ColumnHeader.indexOf("CreamReqPlan")] = "0";
            }
        }
        if(!rowListMain[rMain].at(ColumnHeader.indexOf("OrderFact")).isEmpty() &&
        (rowListMain[rMain].at(ColumnHeader.indexOf("Status")) == "INTL" || rowListMain[rMain].at(ColumnHeader.indexOf("Status")) == "UNTE")) {
            QDateTime start;
            QDateTime end;
            start = QDateTime::fromString(rowListMain[rMain].at(ColumnHeader.indexOf("StartFact")), "dd.MM.yyyy hh:mm:ss");
            end = QDateTime::fromString(rowListMain[rMain].at(ColumnHeader.indexOf("EndFact")), "dd.MM.yyyy hh:mm:ss");
            if(start < QDateTime(selectDate.addDays(1)) && end > QDateTime(selectDate.addDays(1))) {
                float Qty;
                int Duration;
                float QtyInSec;
                //Общая длительность заказа
                Duration = start.secsTo(end);

                //Кол-во заказа
                //Qty = rowListMain[rMain].at(ColumnHeader.indexOf("QuantityFact")).toFloat();
                //QtyInSec = 1 * Qty / Duration;
                //Qty = start.secsTo(QDateTime(selectDate.addDays(1))) * QtyInSec;
                //rowListMain[rMain][ColumnHeader.indexOf("QuantityFact")] = QString::number(Qty);

                //Фактический расход молока
                Qty = rowListMain[rMain].at(ColumnHeader.indexOf("MilkReqFact")).toFloat();
                QtyInSec = 1 * Qty / Duration;
                Qty = start.secsTo(QDateTime(selectDate.addDays(1))) * QtyInSec;
                rowListMain[rMain][ColumnHeader.indexOf("MilkReqFact")] = QString::number(Qty);

                //Фактический расход обрата
                Qty = rowListMain[rMain].at(ColumnHeader.indexOf("SkMilkReqFact")).toFloat();
                QtyInSec = 1 * Qty / Duration;
                Qty = start.secsTo(QDateTime(selectDate.addDays(1))) * QtyInSec;
                rowListMain[rMain][ColumnHeader.indexOf("SkMilkReqFact")] = QString::number(Qty);

                //Фактический расход сливок
                Qty = rowListMain[rMain].at(ColumnHeader.indexOf("CreamReqFact")).toFloat();
                QtyInSec = 1 * Qty / Duration;
                Qty = start.secsTo(QDateTime(selectDate.addDays(1))) * QtyInSec;
                rowListMain[rMain][ColumnHeader.indexOf("CreamReqFact")] = QString::number(Qty);
            }
            if(start >= QDateTime(selectDate.addDays(1))) {
                //Фактический расход молока
                rowListMain[rMain][ColumnHeader.indexOf("MilkReqFact")] = "0";
                //Фактический расход обрата
                rowListMain[rMain][ColumnHeader.indexOf("SkMilkReqFact")] = "0";
                //Фактический расход сливок
                rowListMain[rMain][ColumnHeader.indexOf("CreamReqFact")] = "0";
            }
        }
    }
}

void MainWindow::on_action_Exit_triggered()
{
    QApplication::exit();
}
