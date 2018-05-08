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
    return 8;
}

QVariant PartitionsTableModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole)
    {
        PartitionDataPtr data = m_table[index.row()];
        switch (index.column())
        {
        case 0 : return data->partitionName;
        case 1 : return data->location;
        case 2 : return data->type;
        case 3 : return data->FSType;
        case 4 : return data->state;
        case 5 : return humanSize(data->size);
        case 6 : return humanSize(data->freeSpace);
        case 7 : return QString::number((int)(data->freeSpace / data->size * 100)) + " %";
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
        case 1 : return QString::fromUtf8("Расположение");
        case 2 : return QString::fromUtf8("Тип");
        case 3 : return QString::fromUtf8("Файловая система");
        case 4 : return QString::fromUtf8("Состояние");
        case 5 : return QString::fromUtf8("Емкость");
        case 6 : return QString::fromUtf8("Свободно");
        case 7 : return QString::fromUtf8("Свободно %");
        }
        return QVariant();
    }
    else
        return QVariant();
}
