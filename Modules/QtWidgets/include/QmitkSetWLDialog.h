#ifndef SetWLDialog_h
#define SetWLDialog_h

#include <QObject>

#include <QDialog>
#include <QSpinBox>
#include <QComboBox>

#include "MitkQtWidgetsExports.h"

class MITKQTWIDGETS_EXPORT SetWLDialog : public QDialog
{
    Q_OBJECT
public:
    SetWLDialog();
    //
    int getX() { return spinX->value(); }
    int getY() { return spinY->value(); }
private slots:
    void itemActiveated(int i);
    void ChangeWL(int i);
private:
    QSpinBox *spinX, *spinY;
    QComboBox *cb;
signals:
    void signalWLChanged(int w, int l);
};

#endif //SetWLDialog_h