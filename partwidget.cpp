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
    setStyleSheet(tr("QPushButton:checked {background:red};"));

    //считаем количество дисков
    int ndisks = 0;
    std::for_each(m_data.cbegin(), m_data.cend(), [&ndisks](const std::shared_ptr<PartitionData> &pdata)
    {if (pdata->disk_number > ndisks) ndisks = pdata->disk_number;});
    ndisks++;

    //динамически добавляем виджеты с дисками

    QButtonGroup *partGroup = new QButtonGroup(this);
    partGroup->setExclusive(true);

    for (int i = 0; i < ndisks; i++)
    {
        QHBoxLayout *hLayout = new QHBoxLayout(this);
        hLayout->setMargin(0);

        //добавляем начальную кнопку
        QPushButton *startButton = new QPushButton("Диск "+QString::number(i));
        QSizePolicy sp(QSizePolicy::Preferred, QSizePolicy::Fixed);
        startButton->setMinimumWidth(20);
        startButton->setMaximumHeight(50);
        startButton->setSizePolicy(sp);
        startButton->setCheckable(true);
        hLayout->addWidget(startButton);

        //добавляем виджеты разделов диска
        for (auto partition : m_data)
        {
            if (partition->disk_number == i)
            {
                QPushButton *button = new QPushButton(partition->partitionName+"\n"+humanSize(partition->capacity)+"\n"+partition->state);
                button->setCheckable(true);
                button->setMinimumWidth(20);
                button->setMaximumHeight(50);
                QSizePolicy sp(QSizePolicy::Minimum, QSizePolicy::Fixed);
                if (partition->capacity > 1024*1024*512)
                    sp.setHorizontalStretch(2);
                else
                    sp.setHorizontalStretch(1);
                button->setSizePolicy(sp);
                hLayout->addWidget(button);
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

        m_layout->addLayout(hLayout);
    }
}

void PartWidget::selectPartition(QString partName)
{
    emit selectPartitionButton(partName);
}
