#ifndef QLISTWIDGETITEMSEC_H
#define QLISTWIDGETITEMSEC_H

#include <QListWidgetItem>
#include "../NMDaemonClient/nmdaemonclient.h"

class QListWidgetItemSec : public QListWidgetItem
{
protected:
    QString Security;
public:
    QListWidgetItemSec(QListWidget *parent = nullptr, int type = Type);
    QListWidgetItemSec(const QString &text, QListWidget *parent = nullptr, int type = Type);
    QListWidgetItemSec(const QIcon &icon, const QString &text, QListWidget *parent = nullptr, int type = Type);
    QListWidgetItemSec(const QListWidgetItem &other);
    QString GetSecurity() const;
    void SetSecurity(const QString &newSecurity);
};

#endif // QLISTWIDGETITEMSEC_H
