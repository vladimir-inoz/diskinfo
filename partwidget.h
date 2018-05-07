#ifndef PARTWIDGET_H
#define PARTWIDGET_H

#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include <vector>

#include "winutils.h"

class PartWidget : public QFrame
{
    Q_OBJECT
public:
    explicit PartWidget(QWidget *parent = nullptr);
    void setData(PartitionTable _data) {m_data = _data; updateView();}
    void updateView();
private:
    QVBoxLayout *m_layout;
    PartitionTable m_data;
signals:
    void partitionSelected(QString);
};

#endif // PARTWIDGET_H
