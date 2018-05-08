#ifndef PARTITIONSTABLEMODEL_H
#define PARTITIONSTABLEMODEL_H

#include <QObject>
#include <QAbstractTableModel>

#include "winutils.h"

class PartitionsTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit PartitionsTableModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    void setData(const PartitionTable &table) {m_table = table;}
private:
    PartitionTable m_table;
};

#endif // PARTITIONSTABLEMODEL_H
