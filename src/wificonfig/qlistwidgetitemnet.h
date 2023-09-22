#ifndef QLISTWIDGETITEMNET_H
#define QLISTWIDGETITEMNET_H

#include <QListWidgetItem>

class QListWidgetItemNet : public QListWidgetItem
{
protected:
    int NetId;
public:
    QListWidgetItemNet(QListWidget *parent = nullptr, int type = Type);
    QListWidgetItemNet(const QString &text, QListWidget *parent = nullptr, int type = Type);
    QListWidgetItemNet(const QIcon &icon, const QString &text, QListWidget *parent = nullptr, int type = Type);
    QListWidgetItemNet(const QListWidgetItem &other);

    int GetNetId();
    void SetNetId(int);
};

#endif // QLISTWIDGETITEMNET_H
