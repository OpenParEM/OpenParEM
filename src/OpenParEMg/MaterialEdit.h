#ifndef MATERIALEDIT_H
#define MATERIALEDIT_H

#include <QDialog>

namespace Ui {
class MaterialEdit;
}

class MaterialEdit : public QDialog
{
    Q_OBJECT

public:
    explicit MaterialEdit(QWidget *parent = nullptr);
    ~MaterialEdit();

private:
    Ui::MaterialEdit *ui;
};

#endif // MATERIALEDIT_H
