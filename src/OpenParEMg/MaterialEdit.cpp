#include "MaterialEdit.h"
#include "ui_MaterialEdit.h"

MaterialEdit::MaterialEdit(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::MaterialEdit)
{
    ui->setupUi(this);
}

MaterialEdit::~MaterialEdit()
{
    delete ui;
}
