#ifndef WIFIEDIT_H
#define WIFIEDIT_H

#include <QDialog>
#include "ui_wifiedit.h"
#include "wpasuppnetwork.h"

namespace Ui {
class WifiEdit;
}

class WifiEdit : public QDialog, private Ui_WifiEdit
{
    Q_OBJECT

public:
    explicit WifiEdit(WpaSuppNetwork *net, QWidget *parent = nullptr);
    ~WifiEdit();
    bool IsModified();

private:
    WpaSuppNetwork *ParentNet;
    QString NewKey;
    bool modified;

private slots:
    void SlotClose();
    void SlotSaveAndClose();
    void SlotSave();
    void SlotResetKey();

signals:
    void Saved();
};

#endif // WIFIEDIT_H
