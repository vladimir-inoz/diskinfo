#include "partwidget.h"

#include <QPushButton>
#include <QButtonGroup>
#include <QDebug>
#include <QStyleFactory>
PartWidget::PartWidget(QWidget *parent) : QFrame(parent)
{
    m_layout = new QVBoxLayout(this);
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

        //добавляем куски в диск
        for (auto partition : m_data)
        {
            if (partition->disk_number == i)
            {
                QPushButton *button = new QPushButton(partition->partitionName+"\n"+humanSize(partition->capacity)+"\n"+partition->state);
                button->setCheckable(true);
                /*QSizePolicy sp(QSizePolicy::Preferred, QSizePolicy::Preferred);
                if (partition->capacity > 1024*1024*512)
                    sp.setHorizontalStretch(2);
                else
                    sp.setHorizontalStretch(1);
                button->setSizePolicy(sp);*/
                hLayout->addWidget(button);
                partGroup->addButton(button);

                //добавляем обработчик клика
                QObject::connect(button, &QPushButton::pressed, [this, partition]()
                {
                   emit partitionSelected(partition->partitionName);
                });
            }
        }

        m_layout->addLayout(hLayout);
    }
}
