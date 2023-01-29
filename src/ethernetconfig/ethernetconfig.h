/***************************************************************************
 *   Copyright (C) 2016 by Kris Moore   *
 *   kris@ixsystems.com							   *
 *                                                                         *
 *   Permission is hereby granted, free of charge, to any person obtaining *
 *   a copy of this software and associated documentation files (the       *
 *   "Software"), to deal in the Software without restriction, including   *
 *   without limitation the rights to use, copy, modify, merge, publish,   *
 *   distribute, sublicense, and/or sell copies of the Software, and to    *
 *   permit persons to whom the Software is furnished to do so, subject to *
 *   the following conditions:                                             *
 *                                                                         *
 *   The above copyright notice and this permission notice shall be        *
 *   included in all copies or substantial portions of the Software.       *
 *                                                                         *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    *
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. *
 *   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR     *
 *   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, *
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR *
 *   OTHER DEALINGS IN THE SOFTWARE.                                       *
 ***************************************************************************/
#include <qdialog.h>
#include <qwidget.h>
#include "ui_ethernetconfig.h"
#include "ui_ipv4addrinput.h"
#include "../NMDaemonClient/nmdaemoncheckstatus.h"
#include "../NMDaemonClient/nmdaemonspy.h"

#ifndef _ETHERNETCONFIG_H_
#define _ETHERNETCONFIG_H_


class ethernetconfig : public QDialog, private Ui::ethernetconfig
{
    Q_OBJECT

public:
    ethernetconfig() : QDialog()
    {
    nmd_status = new NMDaemonCheckStatus();
	setupUi(this);
    IsMain = false;
    changedUpDown = false;
    }
    ~ethernetconfig()
    {
        nmd_status->stopMonitoring();
        delete nmd_status;
    }
    virtual void setDevice(QString Device);
    virtual void showInfo();
    virtual void programInit();
    void setIsMain(bool);

public slots:
    virtual void slotIPCheckbox();
    virtual void slot_apply();
    virtual void slot_close();
    virtual void slotDisableCheck();
    virtual void slotOK();
    virtual void slotIPv6clicked();
    virtual void slotMacClicked();
    virtual void slotRemoveAlias();
    virtual void slotAddAlias();
    virtual void slotAddAliasToList(QString, QString);

private:
    void runCommand( QString command );
    /*
    QString getNetmaskForIdent( QString ident );
    QString getOutErrorsForIdent(QString ident);
    QString getOutPacketsForIdent(QString ident);
    QString getInErrorsForIdent(QString ident);
    QString getInPacketsForIdent(QString ident);
    QString getStatusForIdent( QString ident );
    QString getMacForIdent( QString ident );
    QString getIpForIdent( QString ident );
    QString getNameForIdent( QString ident );
    QString getGatewayForIdent(QString ident);
    QString getIPv6ForIdent(QString ident);
    QString getMediaForIdent( QString ident );
    */
    QString getLineFromCommandOutput( QString command );
    std::shared_ptr<QVector<QString>> getIPv4AliasesFromUi();
//    bool isWifiEnabled();
    void saveLaggLine(QString dev, QString config);
    void setupEthLagg(QString dev);
    bool useLagg;
    bool changedUpDown;
    QString DeviceName;
    NMDaemonCheckStatus* nmd_status;
    bool IsMain;

private slots:
    void loadInfo();
    void slotFinishLoad();
    void slotCheckGlobalText();
    void slotConfigWPA();
};

#endif
