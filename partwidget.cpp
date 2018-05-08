#include "partwidget.h"

#include <QPushButton>
#include <QButtonGroup>
#include <QDebug>
#include <QStyleFactory>
PartWidget::PartWidget(QWidget *parent) : QFrame(parent)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setMargin(1);
    setLayout(m_layout);
}

void PartWidget::updateView()
{
    setStyleSheet(tr("QPushButton:checked {background:red}"));
    //делаем запрос дисков
    auto disks = getDisks();

    //динамически добавляем виджеты с дисками
    QButtonGroup *partGroup = new QButtonGroup(this);
    partGroup->setExclusive(true);

    //для каждого диска строим строку с разделами из виджетов
    for (auto diskit : disks)
    {
        auto disk = diskit.second;

        QHBoxLayout *hLayout = new QHBoxLayout();
        hLayout->setSpacing(1);

        //добавляем начальную кнопку
        QPushButton *startButton = new QPushButton("Диск "+QString::number(disk->index)
                                                   + "\n" + disk->mediaType + "\n" + humanSize(disk->size));
        QSizePolicy sp(QSizePolicy::Fixed, QSizePolicy::Fixed);
        startButton->setMaximumWidth(100);
        startButton->setMinimumWidth(100);
        startButton->setMaximumHeight(63);
        startButton->setSizePolicy(sp);
        startButton->setCheckable(true);
        hLayout->addWidget(startButton);

        //суммарный размер разделов
        long long sumPartSize = 0;

        //добавляем виджеты разделов диска
        for (auto partition : m_data)
        {
            if (partition->parentDisk->name == disk->name)
            {
                QVBoxLayout *partLayout = new QVBoxLayout();
                partLayout->setSpacing(0);

                sumPartSize += partition->size;

                QSizePolicy sp(QSizePolicy::Minimum, QSizePolicy::Fixed);

                //синяя шапка сверху
                QWidget *widget = new QWidget();
                widget->setStyleSheet("background: blue");
                widget->setMinimumHeight(10);
                widget->setSizePolicy(sp);
                partLayout->addWidget(widget);

                //снизу - кнопка с названием раздела
                QPushButton *button = new QPushButton(
                            partition->partitionName+"\n"+humanSize(partition->size)+"\n"+
                            partition->parentDisk->status);
                button->setStyleSheet("Text-align:left");
                button->setCheckable(true);
                button->setMinimumWidth(20);
                button->setMaximumHeight(50);
                if (partition->size > 1024*1024*512)
                    sp.setHorizontalStretch(2);
                else
                    sp.setHorizontalStretch(1);
                button->setSizePolicy(sp);
                partLayout->addWidget(button);


                hLayout->addLayout(partLayout);
                partGroup->addButton(button);

                //добавляем обработчик клика
                QObject::connect(button, &QPushButton::pressed, [this, partition]()
                {
                   emit partitionSelected(partition->partitionName);
                });
                QObject::connect(this, &PartWidget::selectPartitionButton, [this, button](QString name)
                {
                    if (button->text().contains(name))
                        button->setChecked(true);
                });
            }
        }
        //добавляем неразмеченную область (больше 256 МБ)
        if (disk->size - sumPartSize > 1024*1024*256)
        {
            QVBoxLayout *partLayout = new QVBoxLayout();
            partLayout->setSpacing(0);

            QSizePolicy sp(QSizePolicy::Minimum, QSizePolicy::Fixed);

            QWidget *widget = new QWidget();
            widget->setStyleSheet("background: black");
            widget->setMinimumHeight(10);
            widget->setSizePolicy(sp);
            partLayout->addWidget(widget);

            QPushButton *button = new QPushButton("Не распределена\n"+humanSize(disk->size - sumPartSize));
            button->setStyleSheet("Text-align:left");
            button->setCheckable(true);
            button->setMinimumWidth(20);
            button->setMaximumHeight(50);
            button->setSizePolicy(sp);
            partLayout->addWidget(button);
            partGroup->addButton(button);

            hLayout->addLayout(partLayout);
        }


        //добавляем spacer
        hLayout->addStretch();


        m_layout->addLayout(hLayout);
    }
}

void PartWidget::selectPartition(QString partName)
{
    emit selectPartitionButton(partName);
}
