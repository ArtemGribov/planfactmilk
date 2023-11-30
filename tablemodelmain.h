#ifndef TABLEMODELMAIN_H
#define TABLEMODELMAIN_H

#include <QAbstractTableModel>

class TableModelMain : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Column {
        StepId,
        RunId,
        MachineId,
        ProductLocationId,
        Description,
        OperationId,
        OrderPlan,
        StartPlan,
        EndPlan,
        QuantityPlan,
        OrderFact,
        Status,
        StartFact,
        EndFact,
        QuantityFact,
        DeliveryFact,
        MilkReqPlan,
        SkMilkReqPlan,
        CreamReqPlan,
        MilkReqFact,
        SkMilkReqFact,
        CreamReqFact,
        MilkDelta,
        SkMilkDelta,
        CreamDelta
    };

    TableModelMain(int rows = 0, int columns = 0, QObject *parent = 0);
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex& index) const;
    bool setData(const QModelIndex& index, const QVariant& value,
                 int role = Qt::EditRole);

public:
    QList<QStringList> rowList;
    const QString *Plant;
    const QDate *CheckDate;
    const float *MilkCriteria;
    const float *SkMilkCriteria;
    const float *CreamCriteria;
};

#endif // TABLEMODELMAIN_H
