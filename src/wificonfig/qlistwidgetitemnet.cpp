#include "qlistwidgetitemnet.h"

QListWidgetItemNet::QListWidgetItemNet(QListWidget *parent, int type) :
    QListWidgetItem(parent, type)
{
    NetId = -1;
}

QListWidgetItemNet::QListWidgetItemNet(const QString &text, QListWidget *parent, int type) :
    QListWidgetItem(text, parent, type)
{
    NetId = -1;
}

QListWidgetItemNet::QListWidgetItemNet(const QIcon &icon, const QString &text, QListWidget *parent, int type) :
    QListWidgetItem(icon, text, parent, type)
{
    NetId = -1;
}

QListWidgetItemNet::QListWidgetItemNet(const QListWidgetItem &other) :
    QListWidgetItem(other)
{
    NetId = -1;
}

int QListWidgetItemNet::GetNetId()
{
    return NetId;
}
void QListWidgetItemNet::SetNetId(int netid)
{
    NetId = netid;
}
