#ifndef WINUTILS_H
#define WINUTILS_H

#include <QStringList>
#include <QVariant>
#include <vector>
#include <memory>
#include <windows.h>

//возвращение строки с размером файла в читаемом формате
QString humanSize(double size);

//данные о физическом диске
struct DiskData
{
    unsigned int index;
    QString status;
    QString name;
    //съемный или несъемный диск
    QString mediaType;
    //объем диска
    double size;
};

//данные о разделах физического диска
struct PartitionData
{
    //информация для отображения в модели
    QString partitionName; //имя раздела для отображения

    QString location;
    QString type;
    QString FSType; //тип файловой системы
    QString state;
    double size; //объем
    double freeSpace; //свободное место

    long long length; //длина

    QString internalPartitionName;

    //информация о диске, на котором расположен раздел
    std::shared_ptr<DiskData> parentDisk;
};

using PartitionTable = std::vector<std::shared_ptr<PartitionData>>;

//получение информации о всех разделах
PartitionTable getAllPartitions();
using Win32_DiskDrive = std::map<QString, std::shared_ptr<DiskData>>;
//получение информации о дисках
Win32_DiskDrive getDisks();

#endif // WINUTILS_H
