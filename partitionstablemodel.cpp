#include "partitionstablemodel.h"

PartitionsTableModel::PartitionsTableModel(QObject *parent) : QAbstractTableModel(parent)
{

}

int PartitionsTableModel::rowCount(const QModelIndex &parent) const
{
    return m_table.size();
}

int PartitionsTableModel::columnCount(const QModelIndex &parent) const
{
    return 6;
}

QVariant PartitionsTableModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole)
    {
        PartitionDataPtr data = m_table[index.row()];
        switch (index.column())
        {
        case 0 : return data->partitionName;
        case 1 : return data->FSType;
        case 2 : return data->state;
        case 3 : return humanSize(data->size);
        case 4 : return humanSize(data->freeSpace);
        case 5 : return QString::number((double)(data->freeSpace) / (double)(data->size) * 100.0, 'f', 1) + " %";
        }
        return QVariant();
    }
    else
        return QVariant();
}

QVariant PartitionsTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section)
        {
        case 0 : return QString::fromUtf8("Том");
        case 1 : return QString::fromUtf8("Файловая система");
        case 2 : return QString::fromUtf8("Состояние");
        case 3 : return QString::fromUtf8("Емкость");
        case 4 : return QString::fromUtf8("Свободно");
        case 5 : return QString::fromUtf8("Свободно %");
        }
        return QVariant();
    }
    else
        return QVariant();
}
