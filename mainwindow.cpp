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
    version = "2.4.5";

    //Дата
    ui->dateEdit->setDate(QDateTime::currentDateTime().date().addDays(-1));
    //ui->dateEdit->setDate(QDate(2023, 12, 12));

    //Инициализация дисков для поиска маршрута
    const QStringList lstDisk = { "D", "C", "E" };

    //Инициализация строки состояния
    labelstatus = new QLabel(this);
    statusBar()->addWidget(labelstatus);
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

//Конфигуратор настроек пользователя
bool MainWindow::on_action_FindDirectoryFact_triggered()
{
    QApplication::processEvents();

    pathFact = "";
    //Проверка на дисках
    foreach(QString disk, lstDisk) {
        dirFact.setPath(disk + ":/");
        //Поиск директории
        labelstatus->setText("Поиск директории c фактическими данными. "
                             "Пожалуйста, подождите...");
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
QString MainWindow::findDirFact(const QDir dirFact) {
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

void MainWindow::activategui(bool stat) {
    ui->pushButtonStart->setEnabled(stat);
    ui->dateEdit->setEnabled(stat);
    ui->comboBoxPlant->setEnabled(stat);
}

bool MainWindow::on_pushButtonStart_clicked()
{
    QApplication::processEvents();

    //Отключить активность кнопок
    activategui(false);

    //Очистка
    rowListMain.clear();
    rowListProduct.clear();
    rowListStep.clear();
    rowListRun.clear();
    rowListProductFlow.clear();
    rowListStock.clear();
    rowListFact.clear();
    rowListFactPrevous.clear();
    rowListFactSupply.clear();

    listMilkPlan.clear();
    listSkMilkPlan.clear();
    listCreamPlan.clear();
    listMilkFact.clear();
    listSkMilkFact.clear();
    listCreamFact.clear();
    listExcludeInternalMove.clear();

    pathdirplan.clear();
    namefileplanmask.clear();
    internalpathdirfact.clear();
    internalpathdirsupply.clear();
    timecorrection = 0;
    user.clear();
    admin.clear();

    //Чтение настроек пользователя
    QString fileName = QDir::tempPath() + "/settings.cfg";
    QFile fileSetting(fileName);
    if(!QFile::exists(fileName)) {
        QMessageBox::warning(0,"Ошибка", "Не найден файл settings");
        activategui(true);
        return false;
    }
    if(!fileSetting.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(0,"Ошибка", "Не удается открыть файл settings");
        activategui(true);
        return false;
    }
    QTextStream in(&fileSetting);
    in.setCodec("UTF-8");
    //Чтение пути
    pathFact = in.readAll();
    if(pathFact.isEmpty()) {
        QMessageBox::warning(0,"Ошибка", "Не найдена директория с фактом");
        activategui(true);
        return false;
    }
    fileSetting.close();


    //Чтение настроек приложения
    QString pathData;
    pathData = pathFact + "/planfactmilk/settings_app.cfg";
    QFile f(pathData);
    if(QFile::exists(pathData)) {
        if(!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::warning(0,"Ошибка", "Невозможно прочитать файл settings_app.cfg");
            activategui(true);
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
                                QMessageBox::warning(0,"Нет доступа",
                                                     "Нет доступа к данным. "
                                                     "Пожалуйста, запросите доступ у администратора");
                                //Включить активность кнопок
                                activategui(true);
                                return false;
                            }
                        }
                    }
                    //Коррекция по сдвигу времени UTC
                    if(strlist[0] == "timecorrection") {
                        bool ok;
                        timecorrection = 3600 * strlist[1].toInt(&ok);
                        if(!ok) {
                            QMessageBox::warning(0,"Не корректные настройки времени",
                                                 "Проверьте данные в конфигураторе");
                             activategui(true);
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
                    //Внутренняя директория факта
                    if(strlist[0] == "internalpathdirfact") {
                        internalpathdirfact = strlist[1];
                    }
                    //Внутренняя директория приходов
                    if(strlist[0] == "internalpathdirsupply") {
                        internalpathdirsupply = strlist[1];
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
            activategui(true);
            return false;
        }
        //Загрузка файла с фактом
        if(!loadFileFact(pathFactFile, pathFactFilePrevious, pathFactFileSupply)) {
            QMessageBox::warning(0,"Ошибка", "Ошибка загрузки факта");
            activategui(true);
            return false;
        }

        //Поиск плановых данных
        pathfileplan = findSourceFile(pathdirplan);
        if(pathfileplan.isEmpty()) {
            QMessageBox::warning(0,"Ошибка", "Ошибка поиска плана");
            activategui(true);
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
        if(!QDir(pathdestin).exists()) {
            QMessageBox::warning(0,"Ошибка", "Директория для распаковки не найдена.");
            activategui(true);
            return false;
        }
        if(pathfileplan.isEmpty() || destinfileplan.isEmpty()) {
            QMessageBox::warning(0,"Ошибка", "Ошибка загрузки плана");
            activategui(true);
            return false;
        }
        //Копируем файл исходник
        if(QFile::exists(destinfileplan)) {
            QFile::remove(destinfileplan);
            QFile::copy(pathfileplan, destinfileplan);
        }
        else QFile::copy(pathfileplan, destinfileplan);

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
            QMessageBox::warning(0,"Ошибка", "Распаковщик отсутствует. Проверьте целостность файлов.");
            activategui(true);
            return false;
        }

        //Запуск распаковщика
        startUnpack(pathprogram, destinfileplan, pathdestin);
        return true;
    }
    else {
        selectPlant = ui->comboBoxPlant->currentText();
        //Поиск файлов с фактом
        if(!findFileFact()) {
            activategui(true);
            return false;
        }
        //Загрузка файлов с фактом
        if(!loadFileFact(pathFactFile, pathFactFilePrevious, pathFactFileSupply)) {
            QMessageBox::warning(0,"Ошибка", "Ошибка загрузки факта");
            activategui(true);
            return false;
        }

        //Поиск плановых данных
        pathfileplan = findSourceFile(pathdirplan);
        if(pathfileplan.isEmpty()) {
            QMessageBox::warning(0,"Ошибка", "Ошибка поиска плана");
            activategui(true);
            return false;
        }

        //Выполнить расчеты
        labelstatus->setText("Анализирую данные. Пожалуйста, подождите...");
        calculation();
        activategui(true);
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

    labelstatus->setText("Поиск фактических данных. Пожалуйста, подождите...");

    //Поиск файла с фактом
    QStringList listFiles;
    listFiles = QDir(pathFact + "/" + internalpathdirfact + "/" + selectPlant).entryList(QStringList()
                << internalpathdirfact + "_" + selectDate.toString("dd.MM.yyyy") + "_21_??_??.csv",
                QDir::Files);
    if(listFiles.isEmpty()) {
        QMessageBox::warning(0,"Ошибка", "Фактические данные не найдены");
        labelstatus->setText("Ошибка");
        return false;
    }
    fileFact = listFiles[0];
    //Записать полный путь к файлу
    pathFactFile = pathFact + "/" + internalpathdirfact + "/" + selectPlant + "/" + fileFact;

    //Поиск прошлого файла с фактом
    listFiles.clear();
    listFiles = QDir(pathFact + "/" + internalpathdirfact + "/" + selectPlant).entryList(QStringList()
                << internalpathdirfact + "_" + selectDate.addDays(-1).toString("dd.MM.yyyy") + "_21_??_??.csv",
                QDir::Files);
    if(listFiles.isEmpty()) {
        QMessageBox::warning(0,"Не найден факт предыдущего дня",
                             "Внимание, часть фактических данных найти не удалось.\n"
                             "Для некоторых заказов, точность расчета может быть снижена.");
        return true;
    }
    fileFactPrevious = listFiles[0];
    //Записать полный путь к файлу
    pathFactFilePrevious = pathFact + "/" + internalpathdirfact + "/" + selectPlant + "/" + fileFactPrevious;

    //Поиск файла фактических приходов
    listFiles.clear();
    listFiles = QDir(pathFact + "/" + internalpathdirsupply).entryList(QStringList()
                << internalpathdirsupply + "_" + selectDate.toString("dd.MM.yyyy") + "_21_??_??.csv",
                QDir::Files);
    if(listFiles.isEmpty()) {
        QMessageBox::warning(0,"Не найден факт приходов",
                             "Внимание, часть фактических данных по приходам не найдена.\n"
                             "Точность расчета может быть снижена.");
        return true;
    }
    fileFactSupply = listFiles[0];
    //Записать полный путь к файлу
    pathFactFileSupply = pathFact + "/" + internalpathdirsupply + "/" + fileFactSupply;
    return true;
}


//Загрузка фактических данных
bool MainWindow::loadFileFact(const QString pathfile,
                              const QString pathfileprevious,
                              const QString pathfilesupply) {
    //Загрузка факта
    QFile f(pathfile);
    if(QFile::exists(pathfile)) {
        if(!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "Error read file pathfile";
            labelstatus->setText("Ошибка");
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

    //Корректировка пустых значений времени
    for(int i = 1; i < rowListFact.length(); i++) {
        //Фактичекое время начала
        if(rowListFact[i].at(rowListFact[0].indexOf("GSUZP")).isEmpty()) {
            rowListFact[i].replace(rowListFact[0].indexOf("GSUZP"), "00:00:00");
        }
        //Фактическое время конца
        if(rowListFact[i].at(rowListFact[0].indexOf("GLUZP")).isEmpty()) {
            rowListFact[i].replace(rowListFact[0].indexOf("GLUZP"), "00:00:01");
        }
        //Плановое время начала
        if(rowListFact[i].at(rowListFact[0].indexOf("GSUZP")).isEmpty()) {
            rowListFact[i].replace(rowListFact[0].indexOf("GSUZP"), "00:00:00");
        }
        //Фактическое время конца
        if(rowListFact[i].at(rowListFact[0].indexOf("GLUZP")).isEmpty()) {
            rowListFact[i].replace(rowListFact[0].indexOf("GLUZP"), "00:00:01");
        }
        //Время создания
        if(rowListFact[i].at(rowListFact[0].indexOf("ERFZEIT")).isEmpty()) {
            rowListFact[i].replace(rowListFact[0].indexOf("ERFZEIT"), "00:00:00");
        }
        //Время изменения
        if(rowListFact[i].at(rowListFact[0].indexOf("AEZEIT")).isEmpty()) {
            rowListFact[i].replace(rowListFact[0].indexOf("AEZEIT"), "00:00:00");
        }
    }

    //факт предыдущего дня
    f.setFileName(pathfileprevious);
    if(QFile::exists(pathfileprevious)) {
        if(!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "Error read file pathfileprevious";
            return true;
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

    //Корректировка пустых значений времени
    for(int i = 1; i < rowListFactPrevous.length(); i++) {
        //Фактичекое время начала
        if(rowListFactPrevous[i].at(rowListFactPrevous[0].indexOf("GSUZP")).isEmpty()) {
            rowListFactPrevous[i].replace(rowListFactPrevous[0].indexOf("GSUZP"), "00:00:00");
        }
        //Фактическое время конца
        if(rowListFactPrevous[i].at(rowListFactPrevous[0].indexOf("GLUZP")).isEmpty()) {
            rowListFactPrevous[i].replace(rowListFactPrevous[0].indexOf("GLUZP"), "00:00:01");
        }
        //Плановое время начала
        if(rowListFactPrevous[i].at(rowListFactPrevous[0].indexOf("GSUZP")).isEmpty()) {
            rowListFactPrevous[i].replace(rowListFactPrevous[0].indexOf("GSUZP"), "00:00:00");
        }
        //Фактическое время конца
        if(rowListFactPrevous[i].at(rowListFactPrevous[0].indexOf("GLUZP")).isEmpty()) {
            rowListFactPrevous[i].replace(rowListFactPrevous[0].indexOf("GLUZP"), "00:00:01");
        }
        //Время создания
        if(rowListFactPrevous[i].at(rowListFactPrevous[0].indexOf("ERFZEIT")).isEmpty()) {
            rowListFactPrevous[i].replace(rowListFactPrevous[0].indexOf("ERFZEIT"), "00:00:00");
        }
        //Время изменения
        if(rowListFactPrevous[i].at(rowListFactPrevous[0].indexOf("AEZEIT")).isEmpty()) {
            rowListFactPrevous[i].replace(rowListFactPrevous[0].indexOf("AEZEIT"), "00:00:00");
        }
    }

    //факт приходов
    f.setFileName(pathfilesupply);
    if(!QFile::exists(pathfilesupply)) {
        QMessageBox::warning(0,"Предупреждение", "Внимание, не найден путь к данным приходов.\n"
                                "Точность расчета может быть снижена.");
        return true;
    }
    if(!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(0,"Предупреждение", "Внимание, не удается прочить файл приходов\n"
                                "Точность расчета может быть снижена.");
        return true;
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
            rowListFactSupply.push_back(strlist);
        }
    }
    f.close();
    return true;
}

QString MainWindow::findSourceFile(const QString pathdir)
{
    QApplication::processEvents();

    //Передаем путь к папке поиска
    QDir dir(pathdir);
    //Поиск папки по выбранной дате
    QStringList listDirs = dir.entryList(QDir::NoDot | QDir::NoDotDot | QDir::Dirs);

    foreach(QString subdir, listDirs) {
        QDir source(pathdir + subdir);
        if(selectDate.toString("yyyyMMdd") == source.dirName().left(source.dirName().length() - 6)) {
            QStringList listFiles = source.entryList(QStringList() << namefileplanmask, QDir::Files);
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


void MainWindow::startUnpack(const QString pathprogram,
                            const QString pathdirplan,
                            const QString pathdestiny)
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

    //Обработка данных
    for(int rStep = 1; rStep < rowListStep.length(); rStep++) {
        //Фильтр
        if(rowListStep[rStep].at(rowListStep[0].indexOf("MachineId")).left(4) == selectPlant) {
            QRegExp rxProductLocation("(CIP|3400|QI|Test)");
            QRegExp rxMachine("(ZETA|WALDNER|SELO|DARBO|TERLET)");
            if(rxProductLocation.indexIn(rowListStep[rStep].at(rowListStep[0].indexOf("ProductLocationId"))) == -1) {
                if(rxMachine.indexIn(rowListStep[rStep].at(rowListStep[0].indexOf("MachineId"))) == -1) {
                    addRowEmpty();
                    rowListMain[rowListMain.length()-1][StepId] = rowListStep[rStep].at(rowListStep[0].indexOf("StepId"));
                    rowListMain[rowListMain.length()-1][RunId] = rowListStep[rStep].at(rowListStep[0].indexOf("RunId"));
                    rowListMain[rowListMain.length()-1][MachineId] = rowListStep[rStep].at(rowListStep[0].indexOf("MachineId")).mid(rowListStep[rStep].at(rowListStep[0].indexOf("MachineId")).indexOf("/")+1);
                    rowListMain[rowListMain.length()-1][ProductLocationId] = rowListStep[rStep].at(rowListStep[0].indexOf("ProductLocationId")).left(rowListStep[rStep].at(rowListStep[0].indexOf("ProductLocationId")).indexOf("/"));

                    //Поиск описания материала
                    for(int rProduct = 1; rProduct < rowListProduct.length(); rProduct++) {
                        if(rowListStep[rStep].at(rowListStep[0].indexOf("ProductLocationId")).left(rowListStep[rStep].at(rowListStep[0].indexOf("ProductLocationId")).indexOf("/"))
                        == rowListProduct[rProduct].at(rowListProduct[0].indexOf("ProductId"))) {
                            rowListMain[rowListMain.length()-1][Description] = rowListProduct[rProduct].at(rowListProduct[0].indexOf("DESCRIPTION"));
                            break;
                        }
                        if(rProduct == rowListProduct.length()-1) {
                            rowListMain[rowListMain.length()-1][Description] = "";
                        }
                    }

                    //Маркер
                    rowListMain[rowListMain.length()-1][Marker] = "";

                    //Версия
                    if(rowListStep[rStep].at(rowListStep[0].indexOf("OperationId")).left(4) != "PRO/") {
                        if(rowListStep[rStep].at(rowListStep[0].indexOf("OperationId")).indexOf("/") != -1) {
                            int Pos = rowListStep[rStep].at(rowListStep[0].indexOf("OperationId")).indexOf("/");
                            rowListMain[rowListMain.length()-1][Version] = rowListStep[rStep].at(rowListStep[0].indexOf("OperationId")).left(Pos);
                        }
                        else rowListMain[rowListMain.length()-1][Version] = "";
                    }
                    else rowListMain[rowListMain.length()-1][Version] = "";

                    //Операция
                    rowListMain[rowListMain.length()-1][OperationId] = rowListStep[rStep].at(rowListStep[0].indexOf("OperationId"));

                    //Поиск номера PLO
                    for(int rRun = 1; rRun < rowListRun.length(); rRun++) {
                        if(rowListStep[rStep].at(rowListStep[0].indexOf("RunId")) == rowListRun[rRun].at(rowListRun[0].indexOf("RunId"))) {
                            if(rowListRun[rRun].at(rowListRun[0].indexOf("ERPOrderId")).left(4) == "PLO/" || rowListRun[rRun].at(rowListRun[0].indexOf("ERPOrderId")).left(4) == "PRO/") {
                                rowListMain[rowListMain.length()-1][OrderPlan] = rowListRun[rRun].at(rowListRun[0].indexOf("ERPOrderId")).left(4) +
                                rowListRun[rRun].at(rowListRun[0].indexOf("ERPOrderId")).mid(rowListRun[rRun].at(rowListRun[0].indexOf("ERPOrderId")).indexOf("RFCOMP")+7);
                                break;
                            }
                            else {
                                rowListMain[rowListMain.length()-1][OrderPlan] = rowListRun[rRun].at(rowListRun[0].indexOf("ERPOrderId"));
                                break;
                            }
                        }
                        if(rRun == rowListRun.length()-1) {
                            rowListMain[rowListMain.length()-1][OrderPlan] = "";
                        }
                    }

                    QDateTime dt;
                    dt = QDateTime::fromString(rowListStep[rStep].at(rowListStep[0].indexOf("StartDate")), "yyyy-MM-dd hh:mm:ss");
                    rowListMain[rowListMain.length()-1][StartPlan] = dt.toString("dd.MM.yyyy hh:mm:ss");

                    dt = QDateTime::fromString(rowListStep[rStep].at(rowListStep[0].indexOf("EndDate")), "yyyy-MM-dd hh:mm:ss");
                    rowListMain[rowListMain.length()-1][EndPlan] = dt.toString("dd.MM.yyyy hh:mm:ss");

                    float PlanQty = rowListStep[rStep].at(rowListStep[0].indexOf("PlannedQuantity")).toFloat();
                    QString str;
                    rowListMain[rowListMain.length()-1][QuantityPlan] = str.number(PlanQty);
                }
            }
        }
    }

    //Поиск номера PRO
    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
        if(rowListMain[rMain].at(OrderPlan).left(4) == "PLO/" || rowListMain[rMain].at(OrderPlan).left(4) == "PRO/") {
            bool ok;
            int Pos = rowListMain[rMain].at(OrderPlan).indexOf("/")+1;
            long long ORDER1 = rowListMain[rMain].at(OrderPlan).mid(Pos).toLongLong(&ok, 10);
            if(ok) {
                long long ORDER2;
                for(int rFact = 1; rFact < rowListFact.length(); rFact++) {
                    if(rowListMain[rMain].at(OrderPlan).left(4) == "PLO/") {
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
                            rowListMain[rMain][OrderFact] = QString::number(PRO);

                            //Статус
                            rowListMain[rMain][Status] = rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4);
                            //Дата Время начала
                            QDateTime dt;
                            dt = QDateTime::fromString(rowListFact[rFact].at(rowListFact[0].indexOf("GSTRP")) + " " + rowListFact[rFact].at(rowListFact[0].indexOf("GSUZP")), "dd.MM.yyyy hh:mm:ss");
                            if(!dt.isValid()) dt = QDateTime::fromString(rowListFact[rFact].at(rowListFact[0].indexOf("GSTRP")) + " 00:00:00", "dd.MM.yyyy hh:mm:ss");
                                rowListMain[rMain][StartFact] = dt.toString("dd.MM.yyyy hh:mm:ss");
                            //Дата Время окончания
                            dt = QDateTime::fromString(rowListFact[rFact].at(rowListFact[0].indexOf("GLTRP")) + " " + rowListFact[rFact].at(rowListFact[0].indexOf("GLUZP")), "dd.MM.yyyy hh:mm:ss");
                            if(!dt.isValid()) dt = QDateTime::fromString(rowListFact[rFact].at(rowListFact[0].indexOf("GLTRP")) + " 00:00:00", "dd.MM.yyyy hh:mm:ss");
                                rowListMain[rMain][EndFact] = dt.toString("dd.MM.yyyy hh:mm:ss");
                            //Кол-во PRO
                            float Qty;
                            Qty = rowListFact[rFact].at(rowListFact[0].indexOf("GAMNG_KG")).toFloat(&ok);
                            if(ok)
                                rowListMain[rMain][QuantityFact] = QString::number(Qty);
                            //Выпуск
                            Qty = rowListFact[rFact].at(rowListFact[0].indexOf("GWEMG_KG")).toFloat(&ok);
                            if(ok)
                                rowListMain[rMain][DeliveryFact] = QString::number(Qty);
                            break;
                        }
                    }
                }
            }
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
                 createdate = QDateTime::fromString(rowListFact[rFact].at(rowListFact[0].indexOf("ERDAT")) + " " +
                         rowListFact[rFact].at(rowListFact[0].indexOf("ERFZEIT")), "dd.MM.yyyy hh:mm:ss");
                 changedate = QDateTime::fromString(rowListFact[rFact].at(rowListFact[0].indexOf("AEDAT")) + " " +
                         rowListFact[rFact].at(rowListFact[0].indexOf("AEZEIT")), "dd.MM.yyyy hh:mm:ss");
                 if(ORDER.isEmpty() || ORDER != rowListFact[rFact].at(rowListFact[0].indexOf("AUFNR"))) {
                    //Заказы которые созданы в день оценки
                    if(createdate.addSecs(timecorrection) >= QDateTime(selectDate)) {
                         addRowEmpty();
                         ORDER = rowListFact[rFact].at(rowListFact[0].indexOf("AUFNR"));
                         //PRO Кол-во
                         float Qty;
                         Qty = rowListFact[rFact].at(rowListFact[0].indexOf("GAMNG_KG")).toFloat();
                         rowListMain[rowListMain.length()-1][QuantityFact] = QString::number(Qty);

                         //Выпуск
                         Qty = rowListFact[rFact].at(rowListFact[0].indexOf("GWEMG_KG")).toFloat();
                         rowListMain[rowListMain.length()-1][DeliveryFact] = QString::number(Qty);
                     }
                    //Заказы которые созданы в прошлом, но скорректированы в день оценки
                    else if(createdate.addSecs(timecorrection) <= QDateTime(selectDate) &&
                    changedate.addSecs(timecorrection) >= QDateTime(selectDate)) {
                        addRowEmpty();
                        ORDER = rowListFact[rFact].at(rowListFact[0].indexOf("AUFNR"));
                        //Проверить что изменилось в заказе относительно предыдущего снэпшота
                        if(!rowListFactPrevous.isEmpty()) {
                            //Для закрытых заказов
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
                                            rowListMain[rowListMain.length()-1][OrderFact] = QString::number(QtyPrev);
                                            //Поставка
                                            Qty = rowListFact[rFact].at(rowListFact[0].indexOf("GWEMG_KG")).toFloat();
                                            QtyPrev = rowListFactPrevous[rFactPrev].at(rowListFactPrevous[0].indexOf("GWEMG_KG")).toFloat();
                                            //Внести маркер
                                            if((Qty - QtyPrev) != 0) {
                                                rowListMain[rowListMain.length()-1][DeliveryFact] = QString::number(Qty);
                                                //Маркер, что заказ был изменен
                                                rowListMain[rowListMain.length()-1][Marker].append("?");
                                            }
                                            else {
                                                rowListMain[rowListMain.length()-1][DeliveryFact] = "0";
                                                //Маркер, что заказ не был изменен
                                                rowListMain[rowListMain.length()-1][Marker].append("!");
                                            }
                                            break;
                                        }
                                    }
                                }
                            }
                            //Для открытых заказов
                            else {
                                QString ORDER2 = "";
                                for(int rFactPrev = 1; rFactPrev < rowListFactPrevous.length(); rFactPrev++) {
                                    if(ORDER2.isEmpty() || ORDER2 != rowListFactPrevous[rFactPrev].at(rowListFactPrevous[0].indexOf("AUFNR"))) {
                                        ORDER2 = rowListFactPrevous[rFactPrev].at(rowListFactPrevous[0].indexOf("AUFNR"));
                                        if(ORDER.toLongLong() == ORDER2.toLongLong()) {

                                            float Qty = rowListFact[rFact].at(rowListFact[0].indexOf("GAMNG_KG")).toFloat();
                                            float QtyPrev = rowListFactPrevous[rFactPrev].at(rowListFactPrevous[0].indexOf("GAMNG_KG")).toFloat();
                                            //PRO Кол-во
                                            rowListMain[rowListMain.length()-1][QuantityFact] = QString::number(Qty - QtyPrev);

                                            //Поставка
                                            Qty = rowListFact[rFact].at(rowListFact[0].indexOf("GWEMG_KG")).toFloat();
                                            QtyPrev = rowListFactPrevous[rFactPrev].at(rowListFactPrevous[0].indexOf("GWEMG_KG")).toFloat();
                                            rowListMain[rowListMain.length()-1][DeliveryFact] =  QString::number(Qty - QtyPrev);
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else continue;

                    //Машина
                    rowListMain[rowListMain.length()-1][MachineId] = rowListFact[rFact].at(rowListFact[0].indexOf("MDV01"));
                    //Материал
                    rowListMain[rowListMain.length()-1][ProductLocationId] = rowListFact[rFact].at(rowListFact[0].indexOf("PLNBEZ")).right(9);
                    //Описание материала
                    rowListMain[rowListMain.length()-1][Description] = rowListFact[rFact].at(rowListFact[0].indexOf("MATXT"));
                    //Версия
                    rowListMain[rowListMain.length()-1][Version] = rowListFact[rFact].at(rowListFact[0].indexOf("VERID"));
                    //PRO номер
                    rowListMain[rowListMain.length()-1][OrderFact] = QString::number(ORDER.toLongLong());
                    // Статус
                    rowListMain[rowListMain.length()-1][Status] = rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4);
                    //Дата Время начала
                    QDateTime dt;
                    dt = QDateTime::fromString(rowListFact[rFact].at(rowListFact[0].indexOf("GSTRP")) + " " + rowListFact[rFact].at(rowListFact[0].indexOf("GSUZP")), "dd.MM.yyyy hh:mm:ss");
                    if(!dt.isValid()) dt = QDateTime::fromString(rowListFact[rFact].at(rowListFact[0].indexOf("GSTRP")) + " 00:00:00", "dd.MM.yyyy hh:mm:ss");
                        rowListMain[rowListMain.length()-1][StartFact] = dt.toString("dd.MM.yyyy hh:mm:ss");
                    //Дата Время окончания
                    dt = QDateTime::fromString(rowListFact[rFact].at(rowListFact[0].indexOf("GLTRP")) + " " + rowListFact[rFact].at(rowListFact[0].indexOf("GLUZP")), "dd.MM.yyyy hh:mm:ss");
                    if(!dt.isValid()) dt = QDateTime::fromString(rowListFact[rFact].at(rowListFact[0].indexOf("GLTRP")) + " 00:00:00", "dd.MM.yyyy hh:mm:ss");
                        rowListMain[rowListMain.length()-1][EndFact] = dt.toString("dd.MM.yyyy hh:mm:ss");

                 }
             }
             //Проверка если плановый был удален
             else {
                 if(ORDER.isEmpty() || ORDER != QString::number(rowListFact[rFact].at(rowListFact[0].indexOf("AUFNR")).toLongLong())) {
                    ORDER = QString::number(rowListFact[rFact].at(rowListFact[0].indexOf("AUFNR")).toLongLong());
                    QString PLOORDER = QString::number(rowListFact[rFact].at(rowListFact[0].indexOf("PLNUM")).toLongLong());
                    QDateTime createdate;
                    QDateTime changedate;
                    createdate = QDateTime::fromString(rowListFact[rFact].at(rowListFact[0].indexOf("ERDAT")) + " " + rowListFact[rFact].at(rowListFact[0].indexOf("ERFZEIT")), "dd.MM.yyyy hh:mm:ss");
                    changedate = QDateTime::fromString(rowListFact[rFact].at(rowListFact[0].indexOf("AEDAT")) + " " + rowListFact[rFact].at(rowListFact[0].indexOf("AEZEIT")), "dd.MM.yyyy hh:mm:ss");
                    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
                        int Pos = rowListMain[rMain].at(OrderPlan).indexOf("/");
                        if(Pos != -1) {
                            if(PLOORDER == rowListMain[rMain].at(OrderPlan).mid(Pos+1)) {
                                break;
                            }
                        }
                        else if(rMain == rowListMain.length()-1) {
                            //Заказы которые созданы в день оценки
                            if(createdate.addSecs(timecorrection) >= QDateTime(selectDate)) {
                                addRowEmpty();
                                 //Машина
                                 rowListMain[rowListMain.length()-1][MachineId] = rowListFact[rFact].at(rowListFact[0].indexOf("MDV01"));
                                 //Материал
                                 rowListMain[rowListMain.length()-1][ProductLocationId] = rowListFact[rFact].at(rowListFact[0].indexOf("PLNBEZ")).right(9);
                                 //Описание материала
                                 rowListMain[rowListMain.length()-1][Description] = rowListFact[rFact].at(rowListFact[0].indexOf("MATXT"));
                                 //Версия
                                 rowListMain[rowListMain.length()-1][Version] = rowListFact[rFact].at(rowListFact[0].indexOf("VERID"));
                                 //PLO номер
                                 rowListMain[rowListMain.length()-1][OrderPlan] = "DEL: " + PLOORDER;
                                 //PRO номер
                                 rowListMain[rowListMain.length()-1][OrderFact] = QString::number(ORDER.toLongLong());
                                 // Статус
                                 rowListMain[rowListMain.length()-1][Status] = rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4);
                                 //Дата Время начала
                                 QDateTime dt;
                                 dt = QDateTime::fromString(rowListFact[rFact].at(rowListFact[0].indexOf("GSTRP")) + " " + rowListFact[rFact].at(rowListFact[0].indexOf("GSUZP")), "dd.MM.yyyy hh:mm:ss");
                                 if(!dt.isValid()) dt = QDateTime::fromString(rowListFact[rFact].at(rowListFact[0].indexOf("GSTRP")) + " 00:00:00", "dd.MM.yyyy hh:mm:ss");
                                     rowListMain[rowListMain.length()-1][StartFact] = dt.toString("dd.MM.yyyy hh:mm:ss");
                                 //Дата Время окончания
                                 dt = QDateTime::fromString(rowListFact[rFact].at(rowListFact[0].indexOf("GLTRP")) + " " + rowListFact[rFact].at(rowListFact[0].indexOf("GLUZP")), "dd.MM.yyyy hh:mm:ss");
                                 if(!dt.isValid()) dt = QDateTime::fromString(rowListFact[rFact].at(rowListFact[0].indexOf("GLTRP")) + " 00:00:00", "dd.MM.yyyy hh:mm:ss");
                                     rowListMain[rowListMain.length()-1][EndFact] = dt.toString("dd.MM.yyyy hh:mm:ss");

                                 //PRO Кол-во
                                 float Qty;
                                 Qty = rowListFact[rFact].at(rowListFact[0].indexOf("GAMNG_KG")).toFloat();
                                 rowListMain[rowListMain.length()-1][QuantityFact] = QString::number(Qty);

                                 //Выпуск
                                 Qty = rowListFact[rFact].at(rowListFact[0].indexOf("GWEMG_KG")).toFloat();
                                 rowListMain[rowListMain.length()-1][DeliveryFact] = QString::number(Qty);
                                 break;
                             }
                         }
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
                if(rowListMain[rMain][StepId] == rowListProductFlow[rFlow].at(rowListProductFlow[0].indexOf("StepId"))) {
                    const QString material = rowListProductFlow[rFlow].at(rowListProductFlow[0].indexOf("ProductId"));
                    const QString plant = rowListProductFlow[rFlow].at(rowListProductFlow[0].indexOf("LocationId"));
                    if(isComponent(cat, type, material, plant)) {
                        SumQty += rowListProductFlow[rFlow].at(rowListProductFlow[0].indexOf("Quantity")).toFloat();
                    }
                }
            }
            if(rFlow == rowListProductFlow.length()-1) {
                rowListMain[rMain][MilkReqPlan] = QString::number(SumQty);
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
                if(rowListMain[rMain][StepId] == rowListProductFlow[rFlow].at(rowListProductFlow[0].indexOf("StepId"))) {
                    const QString material = rowListProductFlow[rFlow].at(rowListProductFlow[0].indexOf("ProductId"));
                    const QString plant = rowListProductFlow[rFlow].at(rowListProductFlow[0].indexOf("LocationId"));
                    if(isComponent(cat, type, material, plant)) {
                        SumQty += rowListProductFlow[rFlow].at(rowListProductFlow[0].indexOf("Quantity")).toFloat();
                    }
                }
            }
            if(rFlow == rowListProductFlow.length()-1) {
                rowListMain[rMain][SkMilkReqPlan] = QString::number(SumQty);
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
                if(rowListMain[rMain][StepId] == rowListProductFlow[rFlow].at(rowListProductFlow[0].indexOf("StepId"))) {
                    const QString material = rowListProductFlow[rFlow].at(rowListProductFlow[0].indexOf("ProductId"));
                    const QString plant = rowListProductFlow[rFlow].at(rowListProductFlow[0].indexOf("LocationId"));
                    if(isComponent(cat, type, material, plant)) {
                        SumQty += rowListProductFlow[rFlow].at(rowListProductFlow[0].indexOf("Quantity")).toFloat();
                    }

                }
            }
            if(rFlow == rowListProductFlow.length()-1) {
                rowListMain[rMain][CreamReqPlan] = QString::number(SumQty);
            }
        }
    }


    //Факт молока
    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
        if(rowListMain[rMain][Marker].indexOf("!") == -1) {
            float SumQtyIn = 0.0;
            float QtyOut = 0.0;
            for(int rFact = 1; rFact < rowListFact.length(); rFact++) {
                bool ok;
                long long ORDER1;
                long long ORDER2;
                ORDER1 = rowListMain[rMain][OrderFact].toLongLong(&ok, 10);
                if(ok) {
                    ORDER2 = rowListFact[rFact].at(rowListFact[0].indexOf("AUFNR")).toLongLong(&ok, 10);
                    if(ok) {
                        if(ORDER1 == ORDER2) {
                            QRegExp rxOperation("(/0020)");
                            if(rowListMain[rMain][OperationId].isEmpty() || rxOperation.indexIn(rowListMain[rMain][OperationId]) != -1) {
                                //Факт расхода молока
                                const QString cat = "fact";
                                const QString type = "milk";
                                QString material = QString::number(rowListFact[rFact].at(rowListFact[0].indexOf("MATNR_COM")).toInt());
                                if(isComponent(cat, type, material, selectPlant)) {
                                    if(rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "CMPL" ||
                                    rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "TECO") {
                                        SumQtyIn += -1.0 * rowListFact[rFact].at(rowListFact[0].indexOf("COM_DENMNG_KG")).toFloat();
                                    }
                                    else if(rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "INTL" ||
                                    rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "UNTE") {
                                        float QtyPlan;
                                        float QtyFact;
                                        float QtyDelivFact;
                                        QtyPlan = rowListMain[rMain][QuantityPlan].toFloat();
                                        QtyFact = rowListMain[rMain][QuantityFact].toFloat();
                                        QtyDelivFact = rowListMain[rMain][DeliveryFact].toFloat();
                                        float Proc;
                                        if(QtyPlan) {
                                            if(QtyFact > QtyDelivFact)
                                                Proc = QtyFact * 100 / QtyPlan;
                                            else
                                                Proc = QtyDelivFact * 100 / QtyPlan;
                                            SumQtyIn = rowListMain[rMain][MilkReqPlan].toFloat() * Proc / 100;
                                        }
                                        else  {
                                            SumQtyIn = 0.01;
                                        }
                                    }
                                }
                                //Факт производства молока
                                material = QString::number(rowListFact[rFact].at(rowListFact[0].indexOf("PLNBEZ")).toInt());
                                if(isComponent(cat, type, material, selectPlant)) {
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
                    //Для заказов без корректировки
                    if(rowListMain[rMain][Marker].indexOf("?") == -1) {
                        rowListMain[rMain][MilkReqFact] = QString::number(SumQtyIn + QtyOut);
                    }
                    //Для скорректированных закрытых заказов
                    else {
                        float Qty1 = rowListMain[rMain][QuantityFact].toFloat();
                        float Qty2 = rowListMain[rMain][DeliveryFact].toFloat();
                        if(Qty1 > Qty2) {
                            float Proc = 100 - (Qty2 * 100 / Qty1);
                            rowListMain[rMain][MilkReqFact] = QString::number(-1*(SumQtyIn + QtyOut) * Proc / 100);
                        }
                        else {
                            float Proc = 100 - (Qty1 * 100 / Qty2);
                            rowListMain[rMain][MilkReqFact] = QString::number(-1*(SumQtyIn + QtyOut) * Proc / 100);
                        }
                    }
                }
            }
        }
    }


    //Факт обрата
    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
        if(rowListMain[rMain][Marker].indexOf("!") == -1) {
            float SumQtyIn = 0.0;
            float QtyOut = 0.0;
            for(int rFact = 1; rFact < rowListFact.length(); rFact++) {
                bool ok;
                long long ORDER1;
                long long ORDER2;
                ORDER1 = rowListMain[rMain][OrderFact].toLongLong(&ok, 10);
                if(ok) {
                    ORDER2 = rowListFact[rFact].at(rowListFact[0].indexOf("AUFNR")).toLongLong(&ok, 10);
                    if(ok) {
                        if(ORDER1 == ORDER2) {
                            QRegExp rxOperation("(/0020)");
                            if(rowListMain[rMain][OperationId].isEmpty() || rxOperation.indexIn(rowListMain[rMain][OperationId]) != -1) {
                                //Факт расхода обрата
                                const QString cat = "fact";
                                const QString type = "skmilk";
                                QString material = QString::number(rowListFact[rFact].at(rowListFact[0].indexOf("MATNR_COM")).toInt());
                                if(isComponent(cat, type, material, selectPlant)) {
                                    if(rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "CMPL" ||
                                    rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "TECO") {
                                        SumQtyIn += -1.0 * rowListFact[rFact].at(rowListFact[0].indexOf("COM_DENMNG_KG")).toFloat();
                                    }
                                    else if(rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "INTL" ||
                                    rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "UNTE") {
                                        float QtyPlan;
                                        float QtyFact;
                                        float QtyDelivFact;
                                        QtyPlan = rowListMain[rMain][QuantityPlan].toFloat();
                                        QtyFact = rowListMain[rMain][QuantityFact].toFloat();
                                        QtyDelivFact = rowListMain[rMain][DeliveryFact].toFloat();
                                        float Proc;
                                        if(QtyPlan) {
                                            if(QtyFact > QtyDelivFact)
                                                Proc = QtyFact * 100 / QtyPlan;
                                            else
                                                Proc = QtyDelivFact * 100 / QtyPlan;
                                            SumQtyIn = rowListMain[rMain][SkMilkReqPlan].toFloat() * Proc / 100;
                                        }
                                        else  {
                                            SumQtyIn = 0.01;
                                        }
                                    }
                                }
                                //Факт производства обрата
                                material = QString::number(rowListFact[rFact].at(rowListFact[0].indexOf("PLNBEZ")).toInt());
                                if(isComponent(cat, type, material, selectPlant)) {
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
                    //Для заказов без корректировки
                    if(rowListMain[rMain][Marker].indexOf("?") == -1) {
                        rowListMain[rMain][SkMilkReqFact] = QString::number(SumQtyIn + QtyOut);
                    }
                    //Для скорректированных закрытых заказов
                    else {
                        float Qty1 = rowListMain[rMain][QuantityFact].toFloat();
                        float Qty2 = rowListMain[rMain][DeliveryFact].toFloat();
                        if(Qty1 > Qty2) {
                            float Proc = 100 - (Qty2 * 100 / Qty1);
                            rowListMain[rMain][SkMilkReqFact] = QString::number(-1*(SumQtyIn + QtyOut) * Proc / 100);
                        }
                        else {
                            float Proc = 100 - (Qty1 * 100 / Qty2);
                            rowListMain[rMain][SkMilkReqFact] = QString::number(-1*(SumQtyIn + QtyOut) * Proc / 100);
                        }
                    }
                }
            }
        }
    }


    //Факт сливок
    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
        if(rowListMain[rMain][Marker].indexOf("!") == -1) {
            float SumQtyIn = 0.0;
            float QtyOut = 0.0;
            for(int rFact = 1; rFact < rowListFact.length(); rFact++) {
                bool ok;
                long long ORDER1;
                long long ORDER2;
                ORDER1 = rowListMain[rMain][OrderFact].toLongLong(&ok, 10);
                if(ok) {
                    ORDER2 = rowListFact[rFact].at(rowListFact[0].indexOf("AUFNR")).toLongLong(&ok, 10);
                    if(ok) {
                        if(ORDER1 == ORDER2) {
                            QRegExp rxOperation("(/0020)");
                            if(rowListMain[rMain][OperationId].isEmpty() || rxOperation.indexIn(rowListMain[rMain][OperationId]) != -1) {
                                //Факт расхода сливок
                                const QString cat = "fact";
                                const QString type = "cream";
                                QString material = QString::number(rowListFact[rFact].at(rowListFact[0].indexOf("MATNR_COM")).toInt());
                                if(isComponent(cat, type, material, selectPlant)) {
                                    if(rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "CMPL" ||
                                    rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "TECO") {
                                        SumQtyIn += -1.0 * rowListFact[rFact].at(rowListFact[0].indexOf("COM_DENMNG_KG")).toFloat();
                                    }
                                    else if(rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "INTL" ||
                                    rowListFact[rFact].at(rowListFact[0].indexOf("ASTTX")).left(4) == "UNTE") {
                                        float QtyPlan;
                                        float QtyFact;
                                        float QtyDelivFact;
                                        QtyPlan = rowListMain[rMain][QuantityPlan].toFloat();
                                        QtyFact = rowListMain[rMain][QuantityFact].toFloat();
                                        QtyDelivFact = rowListMain[rMain][DeliveryFact].toFloat();
                                        float Proc;
                                        if(QtyPlan) {
                                            if(QtyFact > QtyDelivFact)
                                                Proc = QtyFact * 100 / QtyPlan;
                                            else
                                                Proc = QtyDelivFact * 100 / QtyPlan;
                                            SumQtyIn = rowListMain[rMain][CreamReqPlan].toFloat() * Proc / 100;
                                        }
                                        else  {
                                            SumQtyIn = 0.01;
                                        }
                                    }
                                }
                                //Факт производства сливок
                                material = QString::number(rowListFact[rFact].at(rowListFact[0].indexOf("PLNBEZ")).toInt());
                                if(isComponent(cat, type, material, selectPlant)) {
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
                    //Для заказов без корректировки
                    if(rowListMain[rMain][Marker].indexOf("?") == -1) {
                        rowListMain[rMain][CreamReqFact] = QString::number(SumQtyIn + QtyOut);
                    }
                    //Для скорректированных закрытых заказов
                    else {
                        float Qty1 = rowListMain[rMain][QuantityFact].toFloat();
                        float Qty2 = rowListMain[rMain][DeliveryFact].toFloat();
                        if(Qty1 > Qty2) {
                            float Proc = 100 - (Qty2 * 100 / Qty1);
                            rowListMain[rMain][CreamReqFact] = QString::number(-1*(SumQtyIn + QtyOut) * Proc / 100);
                        }
                        else {
                            float Proc = 100 - (Qty1 * 100 / Qty2);
                            rowListMain[rMain][CreamReqFact] = QString::number(-1*(SumQtyIn + QtyOut) * Proc / 100);
                        }
                    }
                }
            }
        }
    }

    //План приходов
    setPlanTransfer("plan", "milk");
    setPlanTransfer("plan", "skmilk");
    setPlanTransfer("plan", "cream");

    //Корректировка запасов плана
    ManualCorrectionPlan("plan", "milk");
    ManualCorrectionPlan("plan", "skmilk");
    ManualCorrectionPlan("plan", "cream");

    //Факт приходов
    setFactTransfer("fact", "milk");
    setFactTransfer("fact", "skmilk");
    setFactTransfer("fact", "cream");


    //Фильтр по молочным компонентам
    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
        if(rowListMain[rMain][MilkReqPlan] == "0" &&
           rowListMain[rMain][SkMilkReqPlan] == "0" &&
           rowListMain[rMain][CreamReqPlan] == "0" &&
           rowListMain[rMain][MilkReqFact] == "0" &&
           rowListMain[rMain][SkMilkReqFact] == "0" &&
           rowListMain[rMain][CreamReqFact] == "0" ) {
                rowListMain.removeAt(rMain);
                rMain--;
        }
    }

    //Фильтр по датам
    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
        QDateTime startPlan;
        QDateTime startFact;
        startPlan = QDateTime::fromString(rowListMain[rMain][StartPlan], "dd.MM.yyyy hh:mm:ss");
        startFact = QDateTime::fromString(rowListMain[rMain][StartFact], "dd.MM.yyyy hh:mm:ss");
        if(!rowListMain[rMain][OrderPlan].isEmpty() && rowListMain[rMain][OrderFact].isEmpty()) {
            if(startPlan >= QDateTime(selectDate.addDays(1))) {
                rowListMain.removeAt(rMain);
                rMain--;
            }
        }
        else if(!rowListMain[rMain][OrderPlan].isEmpty() && !rowListMain[rMain][OrderFact].isEmpty()) {
            if(startPlan >= QDateTime(selectDate.addDays(1)) && startFact >= QDateTime(selectDate.addDays(1))) {
                rowListMain.removeAt(rMain);
                rMain--;
            }
        }
        else if(rowListMain[rMain][OrderPlan].isEmpty() && !rowListMain[rMain][OrderFact].isEmpty()) {
            if(rowListMain[rMain][Status] == "INTL" || rowListMain[rMain][Status] == "UNTE") {
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
            if(rowListMain[rMain][ProductLocationId] > rowListMain[i][ProductLocationId]) {
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

    TableModelMain *model = new TableModelMain();
    model->rowList.append(MainWindow::rowListMain);
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
    ui->tableView->setColumnWidth(Description, 200);
    for(int i = 0; i < rowListMain.length(); i++)
        ui->tableView->setRowHeight(i, 10);
    ui->tableView->update();

    labelstatus->setText("План: " + QFileInfo(pathfileplan).birthTime().toString("dd.MM.yyyy hh:mm:ss") + "\t" +
                         + "Факт: " + selectDate.addDays(1).toString("dd.MM.yyyy 00:00:00") + "\t");

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


void MainWindow::ManualCorrectionPlan(const QString cat, const QString type) {
    addRowEmpty();
    float Qty = 0.0;
    for(int rStock = 1; rStock < rowListStock.length(); rStock++) {
        const QString mat = rowListStock[rStock].at(rowListStock[0].indexOf("ProductId"));
        if(isComponent(cat, type, mat, selectPlant)) {
            if(rowListStock[rStock].at(rowListStock[0].indexOf("StockType")) == "movement") {
                if(rowListStock[rStock].at(rowListStock[0].indexOf("ToWhId")).left(4) == selectPlant) {
                    QDateTime dt;
                    dt = QDateTime::fromString(rowListStock[rStock].at(rowListStock[0].indexOf("StartDate")), "yyyy-MM-dd hh:mm:ss");
                    if(dt < QDateTime(selectDate.addDays(1))) {
                        if(rowListStock[rStock].at(rowListStock[0].indexOf("Source")) == "Manual added stock level") {
                            if(rowListStock[rStock].at(rowListStock[0].indexOf("ToWhId")).indexOf("$Outside") != -1) {
                                Qty += -1.0 * rowListStock[rStock].at(rowListStock[0].indexOf("Quantity")).toFloat();
                            }
                            else Qty += rowListStock[rStock].at(rowListStock[0].indexOf("Quantity")).toFloat();
                        }
                    }
                }
            }
        }
    }
    rowListMain[rowListMain.length()-1][ProductLocationId] = type;
    rowListMain[rowListMain.length()-1][EndPlan] = selectDate.toString("dd.MM.yyyy 23:59:59");
    rowListMain[rowListMain.length()-1][OrderPlan] = "movement";
    rowListMain[rowListMain.length()-1][QuantityPlan] = QString::number(Qty);
    if(type == "milk") {
        rowListMain[rowListMain.length()-1][MilkReqPlan] = QString::number(Qty);
    }
    else if(type == "skmilk") {
        rowListMain[rowListMain.length()-1][SkMilkReqPlan] = QString::number(Qty);
    }
    else if(type == "cream") {
        rowListMain[rowListMain.length()-1][CreamReqPlan] = QString::number(Qty);
    }
}


//Факт приходов
void MainWindow::setFactTransfer(const QString cat, const QString type) {
    addRowEmpty();
    float Qty = 0.0;
    for(int r = 1; r < rowListFactSupply.length(); r++) {
        const QString mat = QString::number(rowListFactSupply[r].at(rowListFactSupply[0].indexOf("MATNR")).toLong());
        if(isComponent(cat, type, mat, selectPlant)) {
            if(rowListFactSupply[r].at(rowListFactSupply[0].indexOf("WERKS")) == selectPlant) {
                if(!rowListFactSupply[r].at(rowListFactSupply[0].indexOf("LGORT")).isEmpty()) {
                    if(rowListFactSupply[r].at(rowListFactSupply[0].indexOf("BWART")) == "105" ||
                    rowListFactSupply[r].at(rowListFactSupply[0].indexOf("BWART")) == "ZI7" ||
                    rowListFactSupply[r].at(rowListFactSupply[0].indexOf("BWART")) == "641") {
                        QDate dt;
                        dt = QDate::fromString(rowListFactSupply[r].at(rowListFactSupply[0].indexOf("BUDAT")), "dd.MM.yyyy");
                        if(dt == selectDate) {
                            Qty += rowListFactSupply[r].at(rowListFactSupply[0].indexOf("ERFMG")).toFloat();
                        }
                    }
                }
            }
        }
    }
    rowListMain[rowListMain.length()-1][ProductLocationId] = type;
    rowListMain[rowListMain.length()-1][StartFact] = selectDate.toString("dd.MM.yyyy 00:00:00");
    rowListMain[rowListMain.length()-1][EndFact] = selectDate.toString("dd.MM.yyyy 23:59:59");
    rowListMain[rowListMain.length()-1][Description] = type;
    rowListMain[rowListMain.length()-1][OrderFact] = "ПЕРЕМЕЩ.";
    rowListMain[rowListMain.length()-1][QuantityFact] = QString::number(Qty);
    if(type == "milk") {
        rowListMain[rowListMain.length()-1][MilkReqFact] = QString::number(Qty);
    }
    else if(type == "skmilk") {
        rowListMain[rowListMain.length()-1][SkMilkReqFact] = QString::number(Qty);
    }
    else if(type == "cream") {
        rowListMain[rowListMain.length()-1][CreamReqFact] = QString::number(Qty);
    }
}


//План прихода молока
void MainWindow::setPlanTransfer(QString cat, QString type) {
    addRowEmpty();
    float Qty = 0.0;
    for(int rStock = 1; rStock < rowListStock.length(); rStock++) {
        const QString mat = rowListStock[rStock].at(rowListStock[0].indexOf("ProductId"));
        const QString internalMaterial = rowListStock[rStock].at(rowListStock[0].indexOf("StockId")).left(9);
        if(isComponent(cat, type, mat, selectPlant)) {
            if(moveIsExcludeinStock(selectPlant, internalMaterial)) {
                if(rowListStock[rStock].at(rowListStock[0].indexOf("StockType")) == "movement") {
                    if(rowListStock[rStock].at(rowListStock[0].indexOf("ToWhId")).left(4) == selectPlant) {
                        QDateTime dt;
                        dt = QDateTime::fromString(rowListStock[rStock].at(rowListStock[0].indexOf("StartDate")), "yyyy-MM-dd hh:mm:ss");
                        if(dt < QDateTime(selectDate.addDays(1))) {
                            if(rowListStock[rStock].at(rowListStock[0].indexOf("Source")) != "Manual added stock level") {
                                if(rowListStock[rStock].at(rowListStock[0].indexOf("ToWhId")).indexOf("$Outside") != -1) {
                                    Qty += -1.0 * rowListStock[rStock].at(rowListStock[0].indexOf("Quantity")).toFloat();
                                }
                                else Qty += rowListStock[rStock].at(rowListStock[0].indexOf("Quantity")).toFloat();
                            }
                        }
                    }
                }
            }
        }
    }
    rowListMain[rowListMain.length()-1][ProductLocationId] = type;
    rowListMain[rowListMain.length()-1][EndPlan] = selectDate.toString("dd.MM.yyyy 23:59:59");
    rowListMain[rowListMain.length()-1][Description] = type;
    rowListMain[rowListMain.length()-1][OrderPlan] = "ПЕРЕМЕЩ.";
    rowListMain[rowListMain.length()-1][QuantityPlan] = QString::number(Qty);
    if(type == "milk") {
        rowListMain[rowListMain.length()-1][MilkReqPlan] = QString::number(Qty);
    }
    else if(type == "skmilk") {
        rowListMain[rowListMain.length()-1][SkMilkReqPlan] = QString::number(Qty);
    }
    else if(type == "cream") {
        rowListMain[rowListMain.length()-1][CreamReqPlan] = QString::number(Qty);
    }
}


void MainWindow::addTotalReq() {
    //Добавить пустую запись
    addRowEmpty();
    addRowEmpty();

    //Обнулить цифровые значения в строке
    for(int i = Column::MilkReqPlan; i <= Column::CreamDelta; i++) {
        float QtyMinus = 0;
        float QtyPlus = 0;
        for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
            if(rowListMain[rMain][i].left(1) == "-") {
                QtyMinus += rowListMain[rMain][i].toFloat();
            }
            else {
                QtyPlus += rowListMain[rMain][i].toFloat();
            }
        }

        rowListMain[rowListMain.length()-2][DeliveryFact] = "Расход:";
        rowListMain[rowListMain.length()-2][i] = QString::number(QtyMinus / 1000, 'f', 1) + "т.";

        rowListMain[rowListMain.length()-1][DeliveryFact] = "Приход:";
        rowListMain[rowListMain.length()-1][i] = QString::number(QtyPlus / 1000, 'f', 1) + "т.";
    }
}


void MainWindow::deltaTotalStock(QString type) {
    //Дельта по молоку
    if(type == "milk") {
        for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
            float QtyPlan = 0;
            float QtyFact = 0;
            QtyPlan = rowListMain[rMain][MilkReqPlan].toFloat();
            QtyFact = rowListMain[rMain][MilkReqFact].toFloat();
            rowListMain[rMain][MilkDelta] = QString::number(-1.0*(QtyPlan - QtyFact));
        }
    }
    //Дельта по обрату
    if(type == "skmilk") {
        for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
            float QtyPlan = 0;
            float QtyFact = 0;
            QtyPlan = rowListMain[rMain][SkMilkReqPlan].toFloat();
            QtyFact = rowListMain[rMain][SkMilkReqFact].toFloat();
            rowListMain[rMain][SkMilkDelta] = QString::number(-1.0*(QtyPlan - QtyFact));
        }
    }
    //Дельта по сливкам
    if(type == "cream") {
        for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
            float QtyPlan = 0;
            float QtyFact = 0;
            QtyPlan = rowListMain[rMain][CreamReqPlan].toFloat();
            QtyFact = rowListMain[rMain][CreamReqFact].toFloat();
            rowListMain[rMain][CreamDelta] = QString::number(-1.0*(QtyPlan - QtyFact));
        }
    }
}

bool MainWindow::moveIsExcludeinStock(const QString Plant,
                                      const QString IntMaterial) {
    if(Plant == selectPlant) {
        foreach(QString m, listExcludeInternalMove) {
            if(IntMaterial == m) {
                return false;
            }
        }
    }
    return true;
}


//Проверка на принадлежность отслеживаемому компоненту
bool MainWindow::isComponent(const QString Category,
                             const QString Type,
                             const QString Material,
                             const QString Plant) {
    if(Category == "plan") {
        if(Plant == selectPlant) {
            if(Type == "milk") {
                foreach(QString m, listMilkPlan) {
                    if(Material == m) return true;
                }
            }
            else if(Type == "skmilk") {
                foreach(QString m, listSkMilkPlan) {
                    if(Material == m) return true;
                }
            }
            else if(Type == "cream") {
                foreach(QString m, listCreamPlan) {
                    if(Material == m) return true;
                }
            }
        }
    }
    else if(Category == "fact") {
        if(Type == "milk") {
            foreach(QString m, listMilkFact) {
                if(Material == m) return true;
            }
        }
        if(Type == "skmilk") {
            foreach(QString m, listSkMilkFact) {
                if(Material == m) return true;
            }
        }
        if(Type == "cream") {
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
        if(rowListMain[rMain][ProductLocationId] != rowListMain[rMain+1][ProductLocationId]) {

            //Добавить пустую запись
            QStringList lstr;
            for(int i = 0; i <= Column::CreamDelta; i++) {
                lstr.append("");
            }
            rowListMain.insert(rMain+1, lstr);

            //Обнулить цифровые значения в строке
            rowListMain[rMain+1][MilkReqPlan] = "0";
            rowListMain[rMain+1][MilkReqFact] = "0";
            rowListMain[rMain+1][SkMilkReqPlan] = "0";
            rowListMain[rMain+1][SkMilkReqFact] = "0";
            rowListMain[rMain+1][CreamReqPlan] = "0";
            rowListMain[rMain+1][CreamReqFact] = "0";
            rowListMain[rMain+1][MilkDelta] = "0";
            rowListMain[rMain+1][SkMilkDelta] = "0";
            rowListMain[rMain+1][CreamDelta] = "0";

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
                if(!rowListMain[i][ProductLocationId].isEmpty()) {
                    QtyTotalMilkPlan += rowListMain[i][MilkReqPlan].toFloat();
                    QtyTotalMilkFact += rowListMain[i][MilkReqFact].toFloat();
                    QtyTotalSkMilkPlan += rowListMain[i][SkMilkReqPlan].toFloat();
                    QtyTotalSkMilkFact += rowListMain[i][SkMilkReqFact].toFloat();
                    QtyTotalCreamPlan += rowListMain[i][CreamReqPlan].toFloat();
                    QtyTotalCreamFact += rowListMain[i][CreamReqFact].toFloat();
                    QtyTotalMilkDelta += rowListMain[i][MilkDelta].toFloat();
                    QtyTotalSkMilkDelta += rowListMain[i][SkMilkDelta].toFloat();
                    QtyTotalCreamDelta += rowListMain[i][CreamDelta].toFloat();
                }
                else break;
            }
            rowListMain[rMain+1][MilkReqPlan] = QString::number(QtyTotalMilkPlan);
            rowListMain[rMain+1][MilkReqFact] = QString::number(QtyTotalMilkFact);
            rowListMain[rMain+1][SkMilkReqPlan] = QString::number(QtyTotalSkMilkPlan);
            rowListMain[rMain+1][SkMilkReqFact] = QString::number(QtyTotalSkMilkFact);
            rowListMain[rMain+1][CreamReqPlan] = QString::number(QtyTotalCreamPlan);
            rowListMain[rMain+1][CreamReqFact] = QString::number(QtyTotalCreamFact);
            rowListMain[rMain+1][MilkDelta] = QString::number(QtyTotalMilkDelta);
            rowListMain[rMain+1][SkMilkDelta] = QString::number(QtyTotalSkMilkDelta);
            rowListMain[rMain+1][CreamDelta] = QString::number(QtyTotalCreamDelta);
            rMain++;
        }
    }
}


//Итоги
void MainWindow::addTotalRow() {
    addRowEmpty();

    rowListMain[rowListMain.length()-1][DeliveryFact] = "Итог:";

    float QtyTotal = 0;
    QtyTotal = 0;
    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
        if(!rowListMain[rMain][ProductLocationId].isEmpty()) {
            QtyTotal += rowListMain[rMain][MilkReqPlan].toFloat();
        }
    }
    rowListMain[rowListMain.length()-1][MilkReqPlan] = QString::number(QtyTotal / 1000, 'f', 1) + "т.";


    QtyTotal = 0;
    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
        if(!rowListMain[rMain][ProductLocationId].isEmpty()) {
            QtyTotal += rowListMain[rMain][MilkReqFact].toFloat();
        }
    }
    rowListMain[rowListMain.length()-1][MilkReqFact] = QString::number(QtyTotal / 1000, 'f', 1) + "т.";


    QtyTotal = 0;
    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
        if(!rowListMain[rMain][ProductLocationId].isEmpty()) {
            QtyTotal += rowListMain[rMain][SkMilkReqPlan].toFloat();
        }
    }
    rowListMain[rowListMain.length()-1][SkMilkReqPlan] = QString::number(QtyTotal / 1000, 'f', 1) + "т.";

    QtyTotal = 0;
    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
        if(!rowListMain[rMain][ProductLocationId].isEmpty()) {
            QtyTotal += rowListMain[rMain][SkMilkReqFact].toFloat();
        }
    }
    rowListMain[rowListMain.length()-1][SkMilkReqFact] = QString::number(QtyTotal / 1000, 'f', 1) + "т.";

    QtyTotal = 0;
    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
        if(!rowListMain[rMain][ProductLocationId].isEmpty()) {
            QtyTotal += rowListMain[rMain][CreamReqPlan].toFloat();
        }
    }
    rowListMain[rowListMain.length()-1][CreamReqPlan] = QString::number(QtyTotal / 1000, 'f', 1) + "т.";


    QtyTotal = 0;
    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
        if(!rowListMain[rMain][ProductLocationId].isEmpty()) {
            QtyTotal += rowListMain[rMain][CreamReqFact].toFloat();
        }
    }
    rowListMain[rowListMain.length()-1][CreamReqFact] = QString::number(QtyTotal / 1000, 'f', 1) + "т.";


    QtyTotal = 0;
    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
        if(!rowListMain[rMain][ProductLocationId].isEmpty()) {
            QtyTotal += rowListMain[rMain][MilkDelta].toFloat();
        }
    }
    rowListMain[rowListMain.length()-1][MilkDelta] = QString::number(QtyTotal / 1000, 'f', 1) + "т.";


    QtyTotal = 0;
    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
        if(!rowListMain[rMain][ProductLocationId].isEmpty()) {
            QtyTotal += rowListMain[rMain][SkMilkDelta].toFloat();
        }
    }
    rowListMain[rowListMain.length()-1][SkMilkDelta] = QString::number(QtyTotal / 1000, 'f', 1) + "т.";


    QtyTotal = 0;
    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
        if(!rowListMain[rMain][ProductLocationId].isEmpty()) {
            QtyTotal += rowListMain[rMain][CreamDelta].toFloat();
        }
    }
    rowListMain[rowListMain.length()-1][CreamDelta] = QString::number(QtyTotal / 1000, 'f', 1) + "т.";
}


//Загрузка списка молочных компонентов для отслеживания
bool MainWindow::loadCompopentsData() {

    if(pathFact.isEmpty()) {
        QMessageBox::warning(0,"Ошибка", "Не найдена директория с фактическими данными. Пожалуйста выполните Поиск через меню.");
        activategui(true);
        return false;
    }

    QString pathData;
    pathData = pathFact + "/planfactmilk/components.cfg";
    QFile f(pathData);
    if(!QFile::exists(pathData)) {
        QMessageBox::warning(0,"Ошибка", "Не найден файл components.cfg");
        activategui(true);
        return false;
    }
    if(!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(0,"Ошибка", "Невозможно прочитать файл components.cfg");
        activategui(true);
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


    //Загрузка файла исключений внутренних плановых перемещений
    pathData = pathFact + "/planfactmilk/excludeinternalmove.cfg";
    f.setFileName(pathData);
    if(!QFile::exists(pathData)) {
        QMessageBox::warning(0,"Ошибка", "Не найден файл excludeinternalmove.cfg");
        activategui(true);
        return false;
    }
    if(!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(0,"Ошибка", "Невозможно прочитать файл excludeinternalmove.cfg");
        activategui(true);
        return false;
    }

    stream.setDevice(&f);
    stream.setCodec("UTF-8");

    //Прочитать первую строку в заголовок
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
                        listExcludeInternalMove << strlist[1];
                    }
                }
            }
        }
    f.close();
    return true;
}


//Загрузка нормативных критериев
bool MainWindow::loadCriteria() {
    if(pathFact.isEmpty()) {
        QMessageBox::warning(0,"Ошибка", "Не найдена директория с фактическими данными. "
                                         "Пожалуйста выполните Поиск директории с фактом из через меню Настройки.");
        activategui(true);
        return false;
    }

    QString pathData;
    pathData = pathFact + "/planfactmilk/criteria.cfg";
    QFile f(pathData);
    if(!QFile::exists(pathData)) {
        QMessageBox::warning(0,"Ошибка", "Не найден файл criteria.cfg");
        activategui(true);
        return false;
    }
    if(!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(0,"Ошибка", "Невозможно прочитать файл criteria.cfg");
        activategui(true);
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
    return true;
}


//Срез по времени
void MainWindow::slicetime() {
    for(int rMain = 0; rMain < rowListMain.length(); rMain++) {
        if(!rowListMain[rMain][OrderPlan].isEmpty()) {
            QDateTime start;
            QDateTime end;
            start = QDateTime::fromString(rowListMain[rMain][StartPlan], "dd.MM.yyyy hh:mm:ss");
            end = QDateTime::fromString(rowListMain[rMain][EndPlan], "dd.MM.yyyy hh:mm:ss");
            if(start < QDateTime(selectDate.addDays(1)) && end > QDateTime(selectDate.addDays(1))) {
                float Qty;
                int Duration;
                float QtyInSec;

                //Общая длительность заказа
                Duration = start.secsTo(end);

                //Плановый расход молока
                Qty = rowListMain[rMain][MilkReqPlan].toFloat();
                QtyInSec = 1 * Qty / Duration;
                Qty = start.secsTo(QDateTime(selectDate.addDays(1))) * QtyInSec;
                rowListMain[rMain][MilkReqPlan] = QString::number(Qty);

                //Плановый расход обрата
                Qty = rowListMain[rMain][SkMilkReqPlan].toFloat();
                QtyInSec = 1 * Qty / Duration;
                Qty = start.secsTo(QDateTime(selectDate.addDays(1))) * QtyInSec;
                rowListMain[rMain][SkMilkReqPlan] = QString::number(Qty);

                //Плановый расход сливок
                Qty = rowListMain[rMain][CreamReqPlan].toFloat();
                QtyInSec = 1 * Qty / Duration;
                Qty = start.secsTo(QDateTime(selectDate.addDays(1))) * QtyInSec;
                rowListMain[rMain][CreamReqPlan] = QString::number(Qty);
            }
            if(start >= QDateTime(selectDate.addDays(1))) {
                //Плановый расход молока
                rowListMain[rMain][MilkReqPlan] = "0";
                //Плановый расход обрата
                rowListMain[rMain][SkMilkReqPlan] = "0";
                //Плановый расход сливок
                rowListMain[rMain][CreamReqPlan] = "0";
            }
        }
        if(!rowListMain[rMain][OrderFact].isEmpty() &&
        (rowListMain[rMain][Status] == "INTL" || rowListMain[rMain][Status] == "UNTE")) {
            QDateTime start;
            QDateTime end;
            start = QDateTime::fromString(rowListMain[rMain][StartFact], "dd.MM.yyyy hh:mm:ss");
            end = QDateTime::fromString(rowListMain[rMain][EndFact], "dd.MM.yyyy hh:mm:ss");
            if(start < QDateTime(selectDate.addDays(1)) && end > QDateTime(selectDate.addDays(1))) {
                float Qty;
                int Duration;
                float QtyInSec;
                //Общая длительность заказа
                Duration = start.secsTo(end);

                //Фактический расход молока
                Qty = rowListMain[rMain][MilkReqFact].toFloat();
                QtyInSec = 1 * Qty / Duration;
                Qty = start.secsTo(QDateTime(selectDate.addDays(1))) * QtyInSec;
                rowListMain[rMain][MilkReqFact] = QString::number(Qty);

                //Фактический расход обрата
                Qty = rowListMain[rMain][SkMilkReqFact].toFloat();
                QtyInSec = 1 * Qty / Duration;
                Qty = start.secsTo(QDateTime(selectDate.addDays(1))) * QtyInSec;
                rowListMain[rMain][SkMilkReqFact] = QString::number(Qty);

                //Фактический расход сливок
                Qty = rowListMain[rMain][CreamReqFact].toFloat();
                QtyInSec = 1 * Qty / Duration;
                Qty = start.secsTo(QDateTime(selectDate.addDays(1))) * QtyInSec;
                rowListMain[rMain][CreamReqFact] = QString::number(Qty);
            }
            if(start >= QDateTime(selectDate.addDays(1))) {
                //Фактический расход молока
                rowListMain[rMain][MilkReqFact] = "0";
                //Фактический расход обрата
                rowListMain[rMain][SkMilkReqFact] = "0";
                //Фактический расход сливок
                rowListMain[rMain][CreamReqFact] = "0";
            }
        }
    }
}


void MainWindow::on_action_Exit_triggered()
{
    QApplication::exit();
}


//Добавить пустую запись
void MainWindow::addRowEmpty() {
    QStringList lstr;

    for(int i = 0; i <= Column::CreamDelta; i++) {
        lstr.append("");
    }
    rowListMain.append(lstr);

    //Обнулить цифровые значения в строке
    rowListMain[rowListMain.length()-1][MilkReqPlan] = "0";
    rowListMain[rowListMain.length()-1][MilkReqFact] = "0";
    rowListMain[rowListMain.length()-1][SkMilkReqPlan] = "0";
    rowListMain[rowListMain.length()-1][SkMilkReqFact] = "0";
    rowListMain[rowListMain.length()-1][CreamReqPlan] = "0";
    rowListMain[rowListMain.length()-1][CreamReqFact] = "0";
    rowListMain[rowListMain.length()-1][MilkDelta] = "0";
    rowListMain[rowListMain.length()-1][SkMilkDelta] = "0";
    rowListMain[rowListMain.length()-1][CreamDelta] = "0";
}

