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
bool checkQuestion(QString str) {
    if(str == "?") {
        return true;
    }
    return false;
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
        //для заказов которые были скорректированы
        if(checkQuestion(rowList[index.row()][OperationId].left(1))) {
            if((index.column() == MilkReqFact) ||
            (index.column() == SkMilkReqFact) ||
            (index.column() == CreamReqFact) ||
            (index.column() == MilkDelta) ||
            (index.column() == SkMilkDelta) ||
            (index.column() == CreamDelta)) {
                return QColor(250,200,250);
            }
        }

        //для промежуточных итогов
        if(index.row() != rowList.length()-1 && rowList[index.row()][ProductLocationId].isEmpty())
            return QColor(225,225,225);
        //для общего итога
        if((index.row() == rowList.length()-1)) {
            if(index.column() == MilkDelta) {
                if(abs(rowList[index.row()][MilkDelta].toFloat()) > *MilkCriteria)
                    return QColor(255, 150, 150);
                else
                    return QColor(Qt::green);
            }
            else if(index.column() == SkMilkDelta) {
                if(abs(rowList[index.row()][SkMilkDelta].toFloat()) > *SkMilkCriteria)
                    return QColor(255, 150, 150);
                else
                    return QColor(Qt::green);
            }
            else if(index.column() == CreamDelta) {
                if(abs(rowList[index.row()][CreamDelta].toFloat()) > *CreamCriteria)
                    return QColor(255, 150, 150);
                else
                    return QColor(Qt::green);
            }
            else
                return QColor(173, 216, 230);
        }
        //для статуса INTL
        if(rowList[index.row()][Status] == "INTL" || rowList[index.row()][Status] == "UNTE") {
            QDateTime start = QDateTime::fromString(rowList[index.row()][StartFact], "dd.MM.yyyy hh:mm:ss");
            QDateTime end = QDateTime::fromString(rowList[index.row()][EndFact], "dd.MM.yyyy hh:mm:ss");
            if(rowList[index.row()][OrderPlan].isEmpty()) {
                if((index.column() == MilkReqFact)) {
                    return QColor(250,200,250);
                }
                if((index.column() == MilkReqFact) ||
                (index.column() == SkMilkReqFact) ||
                (index.column() == CreamReqFact) ||
                (index.column() == MilkDelta) ||
                (index.column() == SkMilkDelta) ||
                (index.column() == CreamDelta)) {
                    return QColor(250,200,250);
                }
            }
            if((start < QDateTime(CheckDate->addDays(1)) && end > QDateTime(CheckDate->addDays(1))) ||
            start >= QDateTime(CheckDate->addDays(1))) {
                if((index.column() == MilkReqFact) && rowList[index.row()][MilkReqFact].toFloat() != 0) {
                    return QColor(250, 200, 250);
                }
                if((index.column() == SkMilkReqFact) && rowList[index.row()][SkMilkReqFact].toFloat() != 0) {
                    return QColor(250, 200, 250);
                }
                if((index.column() == CreamReqFact) && rowList[index.row()][CreamReqFact].toFloat() != 0) {
                    return QColor(250, 200, 250);
                }
            }
            if(index.column() == Status) {
                return QColor(165, 180, 255);
            }
        }

        if((index.column() == MilkReqPlan) && rowList[index.row()][MilkReqPlan].toFloat() != 0) {
            return QColor(110,180,255);
        }
        if((index.column() == SkMilkReqPlan) && rowList[index.row()][SkMilkReqPlan].toFloat() != 0) {
            return QColor(200,250,255);
        }
        if((index.column() == CreamReqPlan) && rowList[index.row()][CreamReqPlan].toFloat() != 0) {
            return QColor(255,255,200);
        }
        if((index.column() == MilkReqFact) && rowList[index.row()][MilkReqFact].toFloat() != 0) {
            return QColor(110,180,255);
        }
        if((index.column() == SkMilkReqFact) && rowList[index.row()][SkMilkReqFact].toFloat() != 0) {
            return QColor(200,250,255);
        }
        if((index.column() == CreamReqFact) && rowList[index.row()][CreamReqFact].toFloat() != 0) {
            return QColor(255,255,200);
        }
        if((index.column() == MilkDelta) && rowList[index.row()][MilkDelta].toFloat() != 0) {
            return QColor(110,180,255);
        }
        if((index.column() == SkMilkDelta) && rowList[index.row()][SkMilkDelta].toFloat() != 0) {
            return QColor(200,250,255);
        }
        if((index.column() == CreamDelta) && rowList[index.row()][CreamDelta].toFloat() != 0) {
            return QColor(255,255,200);
        }
    }
    //Шрифт
    if(role == Qt::FontRole) {
        if(index.row() != rowList.length()-1 && rowList[index.row()][ProductLocationId].isEmpty()) {
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
    case StepId: return "StepId";
    case RunId: return "RunId";
    case MachineId: return "Машина";
    case ProductLocationId: return "Код";
    case Description: return "Описание";
    case OperationId: return "ВИ";
    case OrderPlan: return "Номер план";
    case StartPlan: return "Начало план";
    case EndPlan: return "Конец план";
    case QuantityPlan: return "Кол-во план";
    case OrderFact: return "Номер факт";
    case Status: return "Статус";
    case StartFact: return "Начало факт";
    case EndFact: return "Конец факт";
    case QuantityFact: return "Кол-во факт";
    case DeliveryFact: return "Выпуск факт";
    case MilkReqPlan: return "МОЛ.план";
    case SkMilkReqPlan: return "ОБР.план";
    case CreamReqPlan: return "СЛ.план";
    case MilkReqFact: return "МОЛ.факт";
    case SkMilkReqFact: return "ОБР.факт";
    case CreamReqFact: return "СЛ.факт";
    case MilkDelta: return "ЗАПАС.МОЛ";
    case SkMilkDelta: return "ЗАПАС.ОБР";
    case CreamDelta: return "ЗАПАС.СЛ";
    }

    return QVariant();
}
