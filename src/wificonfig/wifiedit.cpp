#include "wifiedit.h"

WifiEdit::WifiEdit(WpaSuppNetwork *net, QWidget *parent) : QDialog(parent), NewKey("")
{
    setupUi(this);
    editID->setText(QString::number(net->GetId()));
    editSSID->setText(net->GetSsid());
    editBSSID->setText(net->GetBssid());
    editPriority->setText(QString::number(net->GetPriority()));
    if(!net->IsProtected())
        pushResetKey->setEnabled(false);
    ParentNet = net;
    modified = false;
    //connect( pushApply, SIGNAL( clicked() ), this, SLOT( SlotSave() ) );
    connect( pushOk, SIGNAL( clicked() ), this, SLOT( SlotSaveAndClose() ) );
    connect( pushClose, SIGNAL( clicked() ), this, SLOT( SlotClose() ) );
    connect( pushResetKey, SIGNAL( clicked() ), this, SLOT( SlotResetKey() ) );
}

WifiEdit::~WifiEdit()
{
}

void WifiEdit::SlotClose()
{
    close();
}

void WifiEdit::SlotSave()
{
    modified = ( editSSID->isModified() || editBSSID->isModified() || editPriority->isModified() || (!NewKey.isEmpty()) );
    if(!modified)
        return;
    if( editSSID->text() != ParentNet->GetSsid() )
        ParentNet->SetSsid(editSSID->text());
    if( editBSSID->text() != ParentNet->GetBssid() )
        ParentNet->SetBssid(editBSSID->text());
    bool ok_convert = false;
    int new_priority = editPriority->text().toInt(&ok_convert);
    if( ok_convert && (new_priority>=0) )
        ParentNet->SetPriority(new_priority);
    if(!NewKey.isEmpty())
    {
        ParentNet->SetKey(NewKey);
    }
    emit Saved();
}

void WifiEdit::SlotSaveAndClose()
{
    SlotSave();
    close();
}

void WifiEdit::SlotResetKey()
{
    auto netkey_dialog = std::make_unique<netKey>();
    connect( netkey_dialog.get(), SIGNAL( saved(QString, QString, bool) ), this, SLOT( SlotKeySaved(QString, QString, bool) ) );
    netkey_dialog->setSSID(editSSID->text());
    netkey_dialog->showKeyType(true);
    netkey_dialog->setModal(true);
    netkey_dialog->exec();
}

bool WifiEdit::IsModified()
{
    return modified;
}

void WifiEdit::SlotKeySaved(QString key, QString, bool)
{
    NewKey = key;
}
