#include "winutils.h"

#include "disphelper.h"

#include <iostream>

using namespace std;

//размер диска в читаемом формате
QString humanSize(double size) {
    int i = 0;
    QString units[] = {"B", "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
    while (size >= 1024) {
        size /= 1024;
        i++;
    }
    QString result = QString::number(size) + " " + units[i];
    return result;
}

QString humanSize(char *text)
{
    double sz = QString(text).toDouble();
    return humanSize(sz);
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

            CDhStringA test;

            dhGetValue(L"%s", &test, wmiResSvc, L".Name");
            cout << "Name: "     << test << endl;
            pdata->partitionName = QString::fromLocal8Bit(test);

            //объем
            dhGetValue(L"%s", &test, wmiResSvc, L".Size");
            cout << "Size: "     << test << endl;
            pdata->capacity = QString(test).toDouble();
            //пока не нашли примонтированный диск,
            //считаем, что свободно 100% раздела
            pdata->free_space = pdata->capacity;

            dhGetValue(L"%s", &test, wmiResSvc, L".SystemName");
            cout << "SystemName: "     << test << endl;

            dhGetValue(L"%s", &test, wmiResSvc, L".Caption");
            cout << "Caption: "     << test << endl;

            dhGetValue(L"%s", &test, wmiResSvc, L".DeviceID");
            cout << "DeviceID: "     << test << endl;
            pdata->internalPartitionName = QString::fromLocal8Bit(test);

            dhGetValue(L"%s", &test, wmiResSvc, L".Description");
            cout << "Description: "     << test << endl;
            pdata->state = QString::fromLocal8Bit(test);

            dhGetValue(L"%s", &test, wmiResSvc, L".Type");
            cout << "Type: "     << test << endl;

            dhGetValue(L"%s", &test, wmiResSvc, L".Bootable");
            cout << "Bootable: "     << test << endl;

            dhGetValue(L"%s", &test, wmiResSvc, L".BootPartition");
            cout << "BootPartition: "     << test << endl;

            dhGetValue(L"%s", &test, wmiResSvc, L".DiskIndex");
            cout << "DiskIndex: "     << test << endl;
            pdata->disk_number = QString(test).toInt();

            dhGetValue(L"%s", &test, wmiResSvc, L".Index");
            cout << "Index: "     << test << endl;

            dhGetValue(L"%s", &test, wmiResSvc, L".StartingOffset");
            cout << "StartingOffset: "     << test << endl;
            pdata->offset.QuadPart = QString(test).toDouble();

            //узнаем Health status
            //узнаем, к какому физическому диску относится раздел
            CDispPtr tmp;
            dhCheck(dhGetValue(L"%o", &tmp, wmiSvc, L".ExecQuery(%S)",
                               L"SELECT * FROM Win32_DiskDriveToDiskPartition"));
            FOR_EACH(tmp, tmp, NULL)
            {
                dhGetValue(L"%s", &test, wmiResSvc, L".StartingOffset");
                cout << "StartingOffset: "     << test << endl;
            } NEXT_THROW(tmp);

            result.push_back(pdata);


            cout << endl;
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

            CDhStringA test;

            dhGetValue(L"%s", &test, wmiResSvc, L".Caption");
            cout << "Caption: " << test << endl;
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
                pdata->fs_type = QString::fromLocal8Bit(test);

                dhGetValue(L"%s", &test, wmiResSvc, L".FreeSpace");
                pdata->free_space = QString(test).toDouble();
            }

        } NEXT_THROW(wmiResSvc);

    }
    catch (string errstr)
    {
        cerr << "Fatal error details:" << endl << errstr << endl;
    }

    //делаем запрос состояния S.M.A.R.T физических дисков
    std::map<QString,QString> SMARTStatus;
    try
    {
        CDispPtr wmiSvc, wmiResSvc;
        dhCheck( dhGetObject(getWmiStr(L"."), NULL, &wmiSvc) );

        WCHAR szWmiQuery[MAX_PATH+256] = L"SELECT * FROM Win32_DiskDrive";
        dhCheck(dhGetValue(L"%o", &wmiResSvc, wmiSvc, L".ExecQuery(%S)", szWmiQuery));

        FOR_EACH(wmiResSvc, wmiResSvc, NULL)
        {

            CDhStringA status, name;

            dhGetValue(L"%s", &status, wmiResSvc, L".Status");
            QString statusStr = QString::fromLocal8Bit(status);
            dhGetValue(L"%s", &name, wmiResSvc, L".Name");
            QString nameStr = QString::fromLocal8Bit(name).remove("\\").remove(".");

            SMARTStatus[nameStr] = statusStr;
        } NEXT_THROW(wmiResSvc);

    }
    catch (string errstr)
    {
        cerr << "Fatal error details:" << endl << errstr << endl;
    }

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

    //добавляем информацию о состоянии диска в записи разделов
    for (auto dd : diskDriveToDiskPartitions)
    {
        auto it = std::find_if(result.begin(), result.end(),
                               [dd](std::shared_ptr<PartitionData> pdata)
        {return (pdata->internalPartitionName == dd.first);});
        if (it != result.end())
        {
            //нашли раздел, соответствующий логическому диску!
            //обновляем его информацию
            std::shared_ptr<PartitionData> &pdata = *it;

            pdata->state = SMARTStatus[dd.second] + " (" + pdata->state + ")";
        }
    }


    return result;
}
