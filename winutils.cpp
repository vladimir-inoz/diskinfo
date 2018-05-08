#include "winutils.h"

#include "disphelper.h"

#include <iostream>

using namespace std;

//размер диска в читаемом формате
QString humanSize(long long size) {
    double m_size = size;
    int i = 0;
    QString units[] = {"B", "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
    while (m_size >= 1024) {
        m_size /= 1024;
        i++;
    }
    QString result = QString::number(m_size,'f',2) + " " + units[i];
    return result;
}

static LPWSTR getWmiStr(LPCWSTR szComputer)
{
    static WCHAR szWmiStr[256];

    wcscpy(szWmiStr, L"winmgmts:{impersonationLevel=impersonate}!\\\\");

    if (szComputer) wcsncat(szWmiStr, szComputer, 128);
    else            wcscat (szWmiStr, L".");

    wcscat(szWmiStr, L"\\root\\cimv2");

    return szWmiStr;
}


Win32_DiskDrive getDisks()
{
    Win32_DiskDrive result;
    try
    {
        CDispPtr wmiSvc, wmiResSvc;
        dhCheck( dhGetObject(getWmiStr(L"."), NULL, &wmiSvc) );

        WCHAR szWmiQuery[MAX_PATH+256] = L"SELECT * FROM Win32_DiskDrive";
        dhCheck(dhGetValue(L"%o", &wmiResSvc, wmiSvc, L".ExecQuery(%S)", szWmiQuery));

        FOR_EACH(wmiResSvc, wmiResSvc, NULL)
        {
            std::shared_ptr<DiskData> diskData = std::make_shared<DiskData>();

            CDhStringA status, name, mediaType, sz, idx;

            dhGetValue(L"%s", &status, wmiResSvc, L".Status");
            diskData->status = QString::fromLocal8Bit(status);
            dhGetValue(L"%s", &name, wmiResSvc, L".Name");
            diskData->name = QString::fromLocal8Bit(name).remove("\\").remove(".");
            dhGetValue(L"%s", &mediaType, wmiResSvc, L".MediaType");
            diskData->mediaType = QString::fromLocal8Bit(mediaType);
            dhGetValue(L"%s", &sz, wmiResSvc, L".Size");
            diskData->size = QString(sz).toLongLong();
            dhGetValue(L"%s", &idx, wmiResSvc, L".Index");
            diskData->index = QString(idx).toLongLong();

            result[diskData->name] = diskData;
        } NEXT_THROW(wmiResSvc);

    }
    catch (string errstr)
    {
        cerr << "Fatal error details:" << endl << errstr << endl;
    }

    return result;
}

PartitionTable getAllPartitions()
{
    PartitionTable result;

    //делаем запрос разделов
    try
    {
        CDispPtr wmiSvc, wmiResSvc;
        dhCheck( dhGetObject(getWmiStr(L"."), NULL, &wmiSvc) );

        WCHAR szWmiQuery[MAX_PATH+256] = L"SELECT * FROM Win32_DiskPartition";
        dhCheck(dhGetValue(L"%o", &wmiResSvc, wmiSvc, L".ExecQuery(%S)", szWmiQuery));

        FOR_EACH(wmiResSvc, wmiResSvc, NULL)
        {
            std::shared_ptr<PartitionData> pdata = std::make_shared<PartitionData>();

            CDhStringA partName, devID, size, description;

            dhGetValue(L"%s", &partName, wmiResSvc, L".Name");
            pdata->partitionName = QString::fromLocal8Bit(partName);

            //объем
            dhGetValue(L"%s", &size, wmiResSvc, L".Size");
            pdata->size = QString(size).toLongLong();
            //пока не нашли примонтированный диск,
            //считаем, что свободно 100% раздела
            pdata->freeSpace = pdata->size;

            dhGetValue(L"%s", &devID, wmiResSvc, L".DeviceID");
            pdata->internalPartitionName = QString::fromLocal8Bit(devID);

            dhGetValue(L"%s", &description, wmiResSvc, L".Description");
            pdata->state = QString::fromLocal8Bit(description);

            result.push_back(pdata);
        } NEXT_THROW(wmiResSvc);

    }
    catch (string errstr)
    {
        cerr << "Fatal error details:" << endl << errstr << endl;
    }

    std::map<QString, QString> logicalDiskToPartition;
    //запрос связки логических дисков и разделов
    try
    {
        CDispPtr wmiSvc, wmiResSvc;
        dhCheck( dhGetObject(getWmiStr(L"."), NULL, &wmiSvc) );

        WCHAR szWmiQuery[MAX_PATH+256] = L"SELECT * FROM Win32_LogicalDiskToPartition";
        dhCheck(dhGetValue(L"%o", &wmiResSvc, wmiSvc, L".ExecQuery(%S)", szWmiQuery));

        FOR_EACH(wmiResSvc, wmiResSvc, NULL)
        {

            CDhStringA ant, dep;

            dhGetValue(L"%s", &ant, wmiResSvc, L".Antecedent");
            QString antStr = QString::fromLocal8Bit(ant).section('=',1).remove("\"");
            dhGetValue(L"%s", &dep, wmiResSvc, L".Dependent");
            QString depStr = QString::fromLocal8Bit(dep).section('=',1).remove("\"");

            logicalDiskToPartition[depStr] = antStr;
        } NEXT_THROW(wmiResSvc);

    }
    catch (string errstr)
    {
        cerr << "Fatal error details:" << endl << errstr << endl;
    }

    //делаем запрос логических томов
    try
    {
        CDispPtr wmiSvc, wmiResSvc;
        dhCheck( dhGetObject(getWmiStr(L"."), NULL, &wmiSvc) );

        WCHAR szWmiQuery[MAX_PATH+256] = L"SELECT * FROM Win32_LogicalDisk";
        dhCheck(dhGetValue(L"%o", &wmiResSvc, wmiSvc, L".ExecQuery(%S)", szWmiQuery));

        FOR_EACH(wmiResSvc, wmiResSvc, NULL)
        {

            //для каждого логического диска из запроса ищем соответствующий ему раздел
            //на диске
            CDhStringA test;

            dhGetValue(L"%s", &test, wmiResSvc, L".Caption");

            //название логического тома
            QString logicalDriveCaption = QString::fromLocal8Bit(test);
            QString logicalDrivePartition = logicalDiskToPartition[logicalDriveCaption];
            auto it = std::find_if(result.begin(), result.end(), [logicalDrivePartition](std::shared_ptr<PartitionData> pdata)
            {return (pdata->internalPartitionName == logicalDrivePartition);});
            if (it != result.end())
            {
                //нашли раздел, соответствующий логическому диску!
                //обновляем его информацию
                std::shared_ptr<PartitionData> &pdata = *it;

                dhGetValue(L"%s", &test, wmiResSvc, L".VolumeName");
                QString volumeName = QString::fromLocal8Bit(test);

                //меняем название раздела
                pdata->partitionName = volumeName + "(" +logicalDriveCaption + ")";

                dhGetValue(L"%s", &test, wmiResSvc, L".FileSystem");
                pdata->FSType = QString::fromLocal8Bit(test);

                dhGetValue(L"%s", &test, wmiResSvc, L".FreeSpace");
                pdata->freeSpace = QString(test).toLongLong();
            }

        } NEXT_THROW(wmiResSvc);

    }
    catch (string errstr)
    {
        cerr << "Fatal error details:" << endl << errstr << endl;
    }

    //делаем запрос физических дисков
    std::map<QString, std::shared_ptr<DiskData>> disks = getDisks();

    //связь дисков и разделов
    std::map<QString, QString> diskDriveToDiskPartitions;
    try
    {
        CDispPtr wmiSvc, wmiResSvc;
        dhCheck( dhGetObject(getWmiStr(L"."), NULL, &wmiSvc) );

        WCHAR szWmiQuery[MAX_PATH+256] = L"SELECT * FROM Win32_DiskDriveToDiskPartition";
        dhCheck(dhGetValue(L"%o", &wmiResSvc, wmiSvc, L".ExecQuery(%S)", szWmiQuery));

        FOR_EACH(wmiResSvc, wmiResSvc, NULL)
        {

            CDhStringA ant, dep;

            dhGetValue(L"%s", &ant, wmiResSvc, L".Antecedent");
            QString antStr = QString::fromLocal8Bit(ant).section('=',1).remove("\"").remove(".").remove("\\");
            dhGetValue(L"%s", &dep, wmiResSvc, L".Dependent");
            QString depStr = QString::fromLocal8Bit(dep).section('=',1).remove("\"");

            diskDriveToDiskPartitions[depStr] = antStr;
        } NEXT_THROW(wmiResSvc);

    }
    catch (string errstr)
    {
        cerr << "Fatal error details:" << endl << errstr << endl;
    }

    //сопоставляем физические диски и разделы
    for (auto partition : result)
    {
        partition->parentDisk = disks[diskDriveToDiskPartitions[partition->internalPartitionName]];
    }

    //добавляем информацию о состоянии диска в записи разделов
    for (auto partition : result)
    {
        partition->state = partition->parentDisk->status +  " (" + partition->state + ")";
    }


    return result;
}
