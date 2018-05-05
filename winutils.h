#ifndef WINUTILS_H
#define WINUTILS_H

#include <QStringList>
#include <QVariant>
#include <vector>
#include <memory>
#include <windows.h>

//возвращение строки с размером файла в читаемом формате
QString humanSize(double size);

void parsePartitionEntry(const PARTITION_INFORMATION_EX &pdg);

//парсинг структуры с информацией о разделе диска
void parseDriveLayout(const DRIVE_LAYOUT_INFORMATION_EX &pdg);

BOOL GetDriveGeometry(LPWSTR wszPath, DISK_GEOMETRY *pdg);

QStringList getPhysicalDisks();

QStringList getDrivesList();

//информация о разделах для отображения
struct PartitionData
{
    //информация для отображения в модели
    QString partitionName; //имя раздела для отображения
    QString location;
    QString type;
    QString fs_type; //тип файловой системы
    QString state;
    double capacity; //объем
    double free_space; //свободное место

    //служебная информация
    bool is_mounted;
    DWORD disk_number; //номер физического диска, на котором расположен раздел
    LARGE_INTEGER offset; //начало
    LARGE_INTEGER length; //длина
};

using PartitionTable = std::vector<std::shared_ptr<PartitionData>>;

//получение информации о всех разделах
PartitionTable getAllPartitions();

#endif // WINUTILS_H
