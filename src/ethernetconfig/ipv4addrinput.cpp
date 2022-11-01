#include "ipv4addrinput.h"
#include "ui_ipv4addrinput.h"

Ipv4addrinput::Ipv4addrinput(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Ipv4addrinput)
{
    ui->setupUi(this);
    connect( this, SIGNAL( accepted() ), this, SLOT( slotApply() ) );
}

Ipv4addrinput::~Ipv4addrinput()
{
    delete ui;
}

void Ipv4addrinput::slotApply()
{
    emit saved(ui->IPv4addr->text(), ui->IPv4mask->text());
}
