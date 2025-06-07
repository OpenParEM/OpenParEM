#ifndef CUSTOMLINEEDIT_H
#define CUSTOMLINEEDIT_H

#include <QLineEdit>
#include <QFocusEvent>
#include <QDebug>
#include <iostream>

using namespace std;

class CustomLineEdit : public QLineEdit {
    Q_OBJECT

public:
    CustomLineEdit(QWidget *parent = nullptr) : QLineEdit(parent) {}

protected:
    void focusOutEvent(QFocusEvent *event) override {
        QLineEdit::returnPressed();
        QLineEdit::focusOutEvent(event);
    }
};

#endif // CUSTOMLINEEDIT_H
