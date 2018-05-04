#include "winutils.h"

#include <QRegularExpression>

#include <iostream>

using namespace std;

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

//парсинг структуры с информацией о разделе диска
void parseDriveLayout(const DRIVE_LAYOUT_INFORMATION_EX &pdg, QString devName, PartitionTable &table)
{
    for (DWORD i = 0; i < pdg.PartitionCount; i++)
    {
        PartitionData data;
        data.partitionName = devName + " partition " + QString::number(pdg.PartitionEntry[i].PartitionNumber);

        data.capacity = pdg.PartitionEntry[i].PartitionLength.QuadPart;
        data.free_space = data.capacity;

        table.push_back(data);
    }
}

BOOL GetDriveGeometry(LPWSTR wszPath, DISK_GEOMETRY *pdg)
{
    HANDLE hDevice = INVALID_HANDLE_VALUE;  // handle to the drive to be examined
    BOOL bResult   = FALSE;                 // results flag
    DWORD junk     = 0;                     // discard results

    hDevice = CreateFileW(wszPath,          // drive to open
                          0,                // no access to the drive
                          FILE_SHARE_READ | // share mode
                          FILE_SHARE_WRITE,
                          NULL,             // default security attributes
                          OPEN_EXISTING,    // disposition
                          0,                // file attributes
                          NULL);            // do not copy file attributes

    if (hDevice == INVALID_HANDLE_VALUE)    // cannot open the drive
    {
        return (FALSE);
    }

    bResult = DeviceIoControl(hDevice,                       // device to be queried
                              IOCTL_DISK_GET_DRIVE_GEOMETRY, // operation to perform
                              NULL, 0,                       // no input buffer
                              pdg, sizeof(*pdg),            // output buffer
                              &junk,                         // # bytes returned
                              (LPOVERLAPPED) NULL);          // synchronous I/O

    CloseHandle(hDevice);

    return (bResult);
}

QStringList getPhysicalDisks(const QStringList &drivesList)
{
    QStringList result;
    char physical[65536];
    char *str = physical;
    //получаем список всех устройств в системе
    QueryDosDeviceA(NULL, physical, sizeof(physical));

    do
    {
        //считываем название диска
        QString physName = QString::fromLocal8Bit(str);
        str+= physName.size() + 1;
        //проверяем, что оно содержит название PhysicalDrive
        if (physName.contains(QRegularExpression("PhysicalDrive")))
            result.append("\\\\.\\"+physName);
    } while (str < &physical[65535]);

    return result;
}

QStringList getDrivesList()
{
    //считываем все физические диски в системе
    LPWSTR str = (LPWSTR)malloc(1024);
    LPWSTR str_end = str + 1024;
    GetLogicalDriveStringsW(1024, str);
    //из строки str получаем все физические диски
    QStringList drivesList;
    do
    {
        //считываем название диска
        QString driveName = QString::fromWCharArray(str);
        //проверяем, что оно содержит букву
        if (driveName.contains(QRegularExpression("[A-Z]")))
        {
            str+= driveName.size() + 1;
            driveName.remove('\\');
            drivesList.append("\\\\.\\"+driveName);
        }
        else
            //если нет, то это просто мусор в конце массива
            //завершаем цикл
            break;
    } while (str < str_end);

    return drivesList;
}

PartitionTable getAllPartitions()
{
    PartitionTable result;

    auto drivesList = getDrivesList();
    auto physDisks = getPhysicalDisks(drivesList);
    //для каждого диска получаем информацию о разделах

    for (QString x : physDisks)
    {
        HANDLE hDevice;               // handle to the drive to be examined

        DRIVE_LAYOUT_INFORMATION_EX *pdg;
        char tmpbuf[2048];
        DWORD lpBytesReturned;
        BOOL bResult = 1;

        auto str = x.toStdWString();
        str += L'\0';

        hDevice = CreateFileW(str.c_str(),          // drive to open
                              0,                // no access to the drive
                              FILE_SHARE_READ | // share mode
                              FILE_SHARE_WRITE,
                              NULL,             // default security attributes
                              OPEN_EXISTING,    // disposition
                              0,                // file attributes
                              NULL);

        //даем команду на получение информации о разделах данного диска
        bResult = DeviceIoControl(
                    (HANDLE) hDevice,                        // device to be queried
                    IOCTL_DISK_GET_DRIVE_LAYOUT_EX,  // operation to perform
                    NULL,
                    0,
                    &tmpbuf,
                    sizeof(tmpbuf),
                    &lpBytesReturned,
                    NULL
                    );

        if (!bResult)
        {
            //проверяем, какая возникла ошибка
            DWORD error = GetLastError();
            cout << "error occured while getting partition data: " << error << endl;
            //завершаем программу, поскольку не смогли получить информацию о разделах
            //return 0;
        }

        //выделяем память под структуру
        pdg = (DRIVE_LAYOUT_INFORMATION_EX*)malloc(lpBytesReturned);
        //копируем содержимое буфера в структуру
        memcpy(pdg, tmpbuf, lpBytesReturned);

        //парсим структуру
        //добавляем новые записи в результирующий массив
        parseDriveLayout(*pdg, x, result);

        //удаляем структуру
        free(pdg);

        //закрываем устройство
        CloseHandle(hDevice);

    }

    return result;
}
