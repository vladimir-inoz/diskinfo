#ifndef WINUTILS_H
#define WINUTILS_H

#include <QStringList>
#include <QVariant>
#include <vector>
#include <tuple>
#include <windows.h>

//возвращение строки с размером файла в читаемом формате
QString humanSize(double size);

void parsePartitionEntry(const PARTITION_INFORMATION_EX &pdg);

//парсинг структуры с информацией о разделе диска
void parseDriveLayout(const DRIVE_LAYOUT_INFORMATION_EX &pdg);

BOOL GetDriveGeometry(LPWSTR wszPath, DISK_GEOMETRY *pdg);

QStringList getPhysicalDisks(const QStringList &drivesList);

QStringList getDrivesList();

//информация о разделах для отображения
struct PartitionData
{
    QString partitionName;
    QString location;
    QString type;
    QString fs_type;
    QString state;
    double capacity;
    double free_space;
};

using PartitionTable = std::vector<PartitionData>;

//получение информации о всех разделах
PartitionTable getAllPartitions();

#endif // WINUTILS_H
