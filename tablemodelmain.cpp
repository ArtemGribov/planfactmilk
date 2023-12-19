#include "tablemodelmain.h"
#include "mainwindow.h"


TableModelMain::TableModelMain(int rows, int columns, QObject *parent)
    : QAbstractTableModel(parent)
{
    QStringList newList;

    for (int column = 0; column < qMax(0, columns); ++column) {
        newList.append("");
    }

    for (int row = 0; row < qMax(0, rows); ++row) {
        rowList.append(newList);
    }
}


int TableModelMain::rowCount(const QModelIndex &/*parent*/) const
{
    return rowList.size();
}


int TableModelMain::columnCount(const QModelIndex &/*parent*/) const
{
    return rowList[0].size();
}


//для заказов которые были скорректированы
QString checkQuestion(QString str) {
    if(str.indexOf("?") != -1) {
        return "?";
    }
    if(str.indexOf("!") != -1) {
        return "!";
    }
    return QString();
}

QVariant TableModelMain::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    if(role == Qt::DisplayRole || role == Qt::EditRole) {
       return rowList[index.row()][index.column()];
    }
    //Цвет фона
    if(role == Qt::BackgroundColorRole)
    {
        //Для заказов которые были скорректированы
        if(checkQuestion(rowList[index.row()][MainWindow::Column::Marker]) == "?") {
            if((index.column() == MainWindow::Column::QuantityFact) ||
            (index.column() == MainWindow::Column::DeliveryFact) ||
            (index.column() == MainWindow::Column::MilkReqFact) ||
            (index.column() == MainWindow::Column::SkMilkReqFact) ||
            (index.column() == MainWindow::Column::CreamReqFact) ||
            (index.column() == MainWindow::Column::MilkDelta) ||
            (index.column() == MainWindow::Column::SkMilkDelta) ||
            (index.column() == MainWindow::Column::CreamDelta)) {
                return QColor(250,200,250);
            }
        }

        //для общего итога
        if(index.row() == rowList.length()-1) {
            if(index.column() == MainWindow::Column::MilkDelta) {
                int pos = rowList[index.row()][MainWindow::Column::MilkDelta].indexOf("т.");
                if(abs(rowList[index.row()][MainWindow::Column::MilkDelta].left(pos).toFloat()) > *MilkCriteria)
                    return QColor(255,150,150);
                else
                    return QColor(Qt::green);
            }
            else if(index.column() == MainWindow::Column::SkMilkDelta) {
                int pos = rowList[index.row()][MainWindow::Column::SkMilkDelta].indexOf("т.");
                if(abs(rowList[index.row()][MainWindow::Column::SkMilkDelta].left(pos).toFloat()) > *SkMilkCriteria)
                    return QColor(255,150, 150);
                else
                    return QColor(Qt::green);
            }
            else if(index.column() == MainWindow::Column::CreamDelta) {
                int pos = rowList[index.row()][MainWindow::Column::CreamDelta].indexOf("т.");
                if(abs(rowList[index.row()][MainWindow::Column::CreamDelta].left(pos).toFloat()) > *CreamCriteria)
                    return QColor(255,150,150);
                else
                    return QColor(Qt::green);
            }
            else
                return QColor(173,216,230);
        }
        //для расхода и прихода
        else if (index.row() == rowList.length()-2 || index.row() == rowList.length()-3) {
            return QColor(173,216,230);
        }

        //для промежуточных итогов
        if(index.row() != rowList.length()-1 && rowList[index.row()][MainWindow::Column::ProductLocationId].isEmpty())
            return QColor(225,225,225);


        //для статуса INTL
        if(rowList[index.row()][MainWindow::Column::Status] == "INTL" || rowList[index.row()][MainWindow::Column::Status] == "UNTE") {
            QDateTime start = QDateTime::fromString(rowList[index.row()][MainWindow::Column::StartFact], "dd.MM.yyyy hh:mm:ss");
            QDateTime end = QDateTime::fromString(rowList[index.row()][MainWindow::Column::EndFact], "dd.MM.yyyy hh:mm:ss");
            if(rowList[index.row()][MainWindow::Column::OrderPlan].isEmpty()) {
                if((index.column() == MainWindow::Column::MilkReqFact)) {
                    return QColor(250,200,250);
                }
                if((index.column() == MainWindow::Column::MilkReqFact) ||
                (index.column() == MainWindow::Column::SkMilkReqFact) ||
                (index.column() == MainWindow::Column::CreamReqFact) ||
                (index.column() == MainWindow::Column::MilkDelta) ||
                (index.column() == MainWindow::Column::SkMilkDelta) ||
                (index.column() == MainWindow::Column::CreamDelta)) {
                    return QColor(250,200,250);
                }
            }
            if((start < QDateTime(CheckDate->addDays(1)) && end > QDateTime(CheckDate->addDays(1))) ||
            start >= QDateTime(CheckDate->addDays(1))) {
                if((index.column() == MainWindow::Column::MilkReqFact) && rowList[index.row()][MainWindow::Column::MilkReqFact].toFloat() != 0) {
                    return QColor(250,200,250);
                }
                if((index.column() == MainWindow::Column::SkMilkReqFact) && rowList[index.row()][MainWindow::Column::SkMilkReqFact].toFloat() != 0) {
                    return QColor(250,200,250);
                }
                if((index.column() == MainWindow::Column::CreamReqFact) && rowList[index.row()][MainWindow::Column::CreamReqFact].toFloat() != 0) {
                    return QColor(250,200,250);
                }
            }
            if(index.column() == MainWindow::Column::Status) {
                return QColor(165,180,255);
            }
        }

        if((index.column() == MainWindow::Column::MilkReqPlan) && rowList[index.row()][MainWindow::Column::MilkReqPlan].toFloat() != 0) {
            return QColor(110,180,255);
        }
        if((index.column() == MainWindow::Column::SkMilkReqPlan) && rowList[index.row()][MainWindow::Column::SkMilkReqPlan].toFloat() != 0) {
            return QColor(200,250,255);
        }
        if((index.column() == MainWindow::Column::CreamReqPlan) && rowList[index.row()][MainWindow::Column::CreamReqPlan].toFloat() != 0) {
            return QColor(255,255,200);
        }
        if((index.column() == MainWindow::Column::MilkReqFact) && rowList[index.row()][MainWindow::Column::MilkReqFact].toFloat() != 0) {
            return QColor(110,180,255);
        }
        if((index.column() == MainWindow::Column::SkMilkReqFact) && rowList[index.row()][MainWindow::Column::SkMilkReqFact].toFloat() != 0) {
            return QColor(200,250,255);
        }
        if((index.column() == MainWindow::Column::CreamReqFact) && rowList[index.row()][MainWindow::Column::CreamReqFact].toFloat() != 0) {
            return QColor(255,255,200);
        }
        if((index.column() == MainWindow::Column::MilkDelta) && rowList[index.row()][MainWindow::Column::MilkDelta].toFloat() != 0) {
            return QColor(110,180,255);
        }
        if((index.column() == MainWindow::Column::SkMilkDelta) && rowList[index.row()][MainWindow::Column::SkMilkDelta].toFloat() != 0) {
            return QColor(200,250,255);
        }
        if((index.column() == MainWindow::Column::CreamDelta) && rowList[index.row()][MainWindow::Column::CreamDelta].toFloat() != 0) {
            return QColor(255,255,200);
        }
    }
    //Шрифт
    if(role == Qt::FontRole) {
        if(index.row() != rowList.length()-1 && rowList[index.row()][MainWindow::Column::ProductLocationId].isEmpty()) {
            QFont font = QFont("Helvetica", 10, QFont::Bold);
            return QVariant(font);
        }
        if((index.row() == rowList.length()-1)) {
            QFont font = QFont("Helvetica", 10, QFont::Bold);
            return QVariant(font);
        }
    }
    return QVariant();
}


Qt::ItemFlags TableModelMain::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}


bool TableModelMain::setData(const QModelIndex &index,
                         const QVariant &value, int role)
{
    //Возможность редактирования
    if (index.isValid() && role == Qt::EditRole) {
        rowList[index.row()][index.column()] = value.toString();
        emit dataChanged(index, index);
        return true;
    }
    return false;
}


QVariant TableModelMain::headerData(int section, Qt::Orientation orientation,
                                int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if(orientation == Qt::Vertical) {
        return section;
    }
    //return rowList[0][section];
    switch(section) {
    case MainWindow::Column::StepId: return "StepId";
    case MainWindow::Column::RunId: return "RunId";
    case MainWindow::Column::MachineId: return "Машина";
    case MainWindow::Column::ProductLocationId: return "Код";
    case MainWindow::Column::Description: return "Описание";
    case MainWindow::Column::Marker: return "Маркер";
    case MainWindow::Column::Version: return "Версия";
    case MainWindow::Column::OperationId: return "Операция";
    case MainWindow::Column::OrderPlan: return "Номер план";
    case MainWindow::Column::StartPlan: return "Начало план";
    case MainWindow::Column::EndPlan: return "Конец план";
    case MainWindow::Column::QuantityPlan: return "Кол-во план";
    case MainWindow::Column::OrderFact: return "Номер факт";
    case MainWindow::Column::Status: return "Статус";
    case MainWindow::Column::StartFact: return "Начало факт";
    case MainWindow::Column::EndFact: return "Конец факт";
    case MainWindow::Column::QuantityFact: return "Кол-во факт";
    case MainWindow::Column::DeliveryFact: return "Выпуск факт";
    case MainWindow::Column::MilkReqPlan: return "МОЛ.план";
    case MainWindow::Column::SkMilkReqPlan: return "ОБР.план";
    case MainWindow::Column::CreamReqPlan: return "СЛ.план";
    case MainWindow::Column::MilkReqFact: return "МОЛ.факт";
    case MainWindow::Column::SkMilkReqFact: return "ОБР.факт";
    case MainWindow::Column::CreamReqFact: return "СЛ.факт";
    case MainWindow::Column::MilkDelta: return "ЗАПАС.МОЛ";
    case MainWindow::Column::SkMilkDelta: return "ЗАПАС.ОБР";
    case MainWindow::Column::CreamDelta: return "ЗАПАС.СЛ";
    }

    return QVariant();
}
