#include "qlistwidgetitemsec.h"

QListWidgetItemSec::QListWidgetItemSec(QListWidget *parent, int type) :
    QListWidgetItem(parent, type)
{
    Security=NMDaemonClient::WIFI_SECURITY_ERROR;
}

QListWidgetItemSec::QListWidgetItemSec(const QString &text, QListWidget *parent, int type) :
    QListWidgetItem(text, parent, type)
{
    Security=NMDaemonClient::WIFI_SECURITY_ERROR;
}

QListWidgetItemSec::QListWidgetItemSec(const QIcon &icon, const QString &text, QListWidget *parent, int type) :
    QListWidgetItem(icon, text, parent, type)
{
    Security=NMDaemonClient::WIFI_SECURITY_ERROR;
}

QListWidgetItemSec::QListWidgetItemSec(const QListWidgetItem &other) :
    QListWidgetItem(other)
{
    Security=NMDaemonClient::WIFI_SECURITY_ERROR;
}

QString QListWidgetItemSec::GetSecurity() const
{
    return Security;
}

void QListWidgetItemSec::SetSecurity(const QString &security)
{
    Security = security;
}
