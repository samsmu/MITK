#include "QmitkSetWLDialog.h"

#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>
#include <QString>
#include <QStringList>

SetWLDialog::SetWLDialog()
    : QDialog()
{
    spinX = new QSpinBox;
    spinY = new QSpinBox;
    spinX->setMinimum(-9999);
    spinX->setMaximum(9999);
    spinX->setSingleStep(5);
    spinY->setMinimum(1);
    spinY->setMaximum(9999);
    spinY->setSingleStep(5);
    //connect( spinX,     SIGNAL(pressed()), SLOT(accept()));
    QPushButton *ok = new QPushButton("Да");
    QPushButton *cancel = new QPushButton("Отмена");
    connect(spinX, SIGNAL(valueChanged(int)), SLOT(ChangeWL(int)));
    connect(spinY, SIGNAL(valueChanged(int)), SLOT(ChangeWL(int)));
    connect(ok, SIGNAL(pressed()), SLOT(accept()));
    connect(cancel, SIGNAL(pressed()), SLOT(reject()));
    QHBoxLayout *hl1 = new QHBoxLayout;
    hl1->addWidget(new QLabel("W:"));
    hl1->addWidget(spinY);
    hl1->addWidget(new QLabel("L:"));
    hl1->addWidget(spinX);
    cb = new QComboBox;
    connect(cb, SIGNAL(activated(int)), SLOT(itemActiveated(int)));
    QHBoxLayout *hl2 = new QHBoxLayout;
    hl2->addWidget(new QLabel("Типичные:"));
    hl2->addWidget(cb);
    QHBoxLayout *hl3 = new QHBoxLayout;
    hl3->addWidget(ok);
    hl3->addWidget(cancel);
    QVBoxLayout *vl = new QVBoxLayout;
    vl->addLayout(hl1);
    vl->addLayout(hl2);
    vl->addLayout(hl3);
    setLayout(vl);
    setWindowTitle("Задайте W L:");
    cb->addItem("Abdomen 400 60");
    cb->addItem("Angio 600 300");
    cb->addItem("Bones 1500 300");
    cb->addItem("Brain 80 40");
    cb->addItem("Chest 400 40");
    cb->addItem("Lung 1500 -400");
    cb->addItem("Liver 400 30");

    QString text = cb->itemText(0);
    QStringList splitList = text.split(" ");
    spinY->setValue(splitList[1].toInt());
    spinX->setValue(splitList[2].toInt());

    /*
      <preset NAME="Heart" LEVEL="200" WINDOW="600" />
       <preset NAME="Blood" LEVEL="100" WINDOW="600" />
       <preset NAME="Intestine" LEVEL="30" WINDOW="400" />
       <preset NAME="Fat" LEVEL="-130" WINDOW="200" />
       <preset NAME="Urinary Bladder" LEVEL="30" WINDOW="50" />
       <preset NAME="Air" LEVEL="-700" WINDOW="600" />
       <preset NAME="Mamma" LEVEL="-90" WINDOW="80" />
       <preset NAME="Spleen" LEVEL="40" WINDOW="50" />
       <preset NAME="Suprarenal Gland" LEVEL="12" WINDOW="50" />
       <preset NAME="Kidney" LEVEL="40" WINDOW="300" />
       <preset NAME="Pancreas" LEVEL="40" WINDOW="400" />
       <preset NAME="Tumor" LEVEL="35" WINDOW="60" />
       <preset NAME="Water" LEVEL="0" WINDOW="10" />
       */

}

//
void SetWLDialog::itemActiveated(int i)
{
    QString text = cb->itemText(i);
    QStringList splitList = text.split(" ");
    spinY->setValue(splitList[1].toInt());
    spinX->setValue(splitList[2].toInt());
}

void SetWLDialog::ChangeWL(int i) {
    emit signalWLChanged(spinY->value(), spinX->value());
}

