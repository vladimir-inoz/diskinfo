#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QPushButton>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    table = getAllPartitions();

    ui->setupUi(this);
    ui->tableView->setModel(&model);
    model.setData(table);
    ui->widget->setData(table);

    ui->tableView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->tableView->verticalHeader()->setDefaultSectionSize(24);
    ui->tableView->resizeColumnsToContents();

    QObject::connect(ui->widget, &PartWidget::partitionSelected,
                     this, &MainWindow::selectRowWithPartition);
    QObject::connect(ui->tableView, &QAbstractItemView::clicked,
                     [this] (const QModelIndex &index)
    {
        QString str = table[index.row()]->partitionName;
        ui->widget->selectPartition(str);
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

//выбираем ряд в таблице по названию раздела
void MainWindow::selectRowWithPartition(const QString &partName)
{
    auto it = std::find_if(table.cbegin(), table.cend(), [&partName] (std::shared_ptr<PartitionData> x)
    {return x->partitionName == partName;});

    int row = std::distance(table.cbegin(), it);

    QModelIndex idx = ui->tableView->model()->index(row, 0);

    ui->tableView->setCurrentIndex(idx);
}
