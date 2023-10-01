#ifndef NETKEY_H
#define NETKEY_H

#include <qfile.h>
#include <QMessageBox>
#include <QDialog>
#include "ui_netKey.h"

class netKey : public QDialog, private Ui::netKey
{
        Q_OBJECT

public:
        netKey() : QDialog()
        {
          setupUi(this);
        }

   void setSSID( QString SSID );
   void showKeyType(bool show);
   QString GetSecurity() const;
   void SetSecurity(const QString &newSecurity);

public slots:

private slots:
   void slotCancel();
   void slotApply();
   void slotShowKey();

private:
    QString Ssid;
    QString Security;
signals:
    void saved(QString, QString, bool, QString);
    void cancel();

} ;
#endif // NETKEY_H

