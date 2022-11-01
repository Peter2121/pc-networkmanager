#ifndef IPV4ADDRINPUT_H
#define IPV4ADDRINPUT_H

#include <QDialog>

namespace Ui {
class Ipv4addrinput;
}

class Ipv4addrinput : public QDialog
{
    Q_OBJECT

public:
    explicit Ipv4addrinput(QWidget *parent = nullptr);
    ~Ipv4addrinput();

signals:
    void saved(QString, QString);

private:
    Ui::Ipv4addrinput *ui;

private slots:
    void slotApply();
};

#endif // IPV4ADDRINPUT_H
