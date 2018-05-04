#include "mainwindow.h"
#include <QApplication>

#include <windows.h>
#include <iostream>

#include <QDebug>
#include <QStorageInfo>

#include <QStringList>
#include <QRegularExpression>

#define wszDrive L"\\\\.\\PhysicalDrive0"
#define cDrive L"\\\\.\\C:"

using namespace std;

/*int main(int argc, char *argv[])
{

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}*/

char* readable_fs(double size/*in bytes*/, char *buf) {
    int i = 0;
    const char* units[] = {"B", "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
    while (size >= 1024) {
        size /= 1024;
        i++;
    }
    sprintf(buf, "%.*f %s", i, size, units[i]);
    return buf;
}

void parsePartitionEntry(const PARTITION_INFORMATION_EX &pdg)
{
    cout << endl;
    cout << "offset: " << pdg.StartingOffset.QuadPart << endl;
    char buf[256];
    cout << "length: " << readable_fs(pdg.PartitionLength.QuadPart, buf) << endl;
    cout << "number: " << pdg.PartitionNumber << endl;
    cout << "rewritable: " << (bool)pdg.RewritePartition << endl;
}

//парсинг структуры с информацией о разделе диска
void parseDriveLayout(const DRIVE_LAYOUT_INFORMATION_EX &pdg)
{
    switch (pdg.PartitionStyle)
    {
    case PARTITION_STYLE_MBR:
        cout << "mbr" << endl;
        break;
    case PARTITION_STYLE_GPT:
        cout << "gpt" << endl;
        break;
    case PARTITION_STYLE_RAW:
        cout << "raw" << endl;
        break;
    };

    cout << "count of partitions: " << pdg.PartitionCount;

    for (auto i = 0; i < pdg.PartitionCount; i++)
        parsePartitionEntry(pdg.PartitionEntry[i]);
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

int main(int args, char** argv)
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

    //программа получает инфомацию о разделах диска PhysicalDrive0

    HANDLE hDevice;               // handle to the drive to be examined

    DRIVE_LAYOUT_INFORMATION_EX *pdg;
    char tmpbuf[2048];
    DWORD lpBytesReturned;
    BOOL bResult = 1;

    //для каждого диска получаем информацию о разделах

    for (QString x : drivesList)
    {
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
            return 0;
        }

        //выделяем память под структуру
        pdg = (DRIVE_LAYOUT_INFORMATION_EX*)malloc(lpBytesReturned);
        //копируем содержимое буфера в структуру
        memcpy(pdg, tmpbuf, lpBytesReturned);

        //парсим структуру
        parseDriveLayout(*pdg);

        //удаляем структуру
        free(pdg);

        //закрываем устройство
        CloseHandle(hDevice);

    }


    return 0;
}
