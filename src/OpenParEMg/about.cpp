////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//    OpenParEMg - A GUI for OpenParEM3D                                      //
//    Copyright (C) 2026 Brian Young                                          //
//                                                                            //
//    This program is free software: you can redistribute it and/or modify    //
//    it under the terms of the GNU General Public License as published by    //
//    the Free Software Foundation, either version 3 of the License, or       //
//    (at your option) any later version.                                     //
//                                                                            //
//    This program is distributed in the hope that it will be useful,         //
//    but WITHOUT ANY WARRANTY; without even the implied warranty of          //
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           //
//    GNU General Public License for more details.                            //
//                                                                            //
//    You should have received a copy of the GNU General Public License       //
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.   //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include "about.h"
#include "ui_about.h"
#include <qtextobject.h>

About::About(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::About)
{
    this->setWindowIcon(QApplication::windowIcon());
    ui->setupUi(this);

    // font scaling courtesy of ChatGPT

    QTextDocument *doc = ui->textEdit->document();

    const double baseSize = 11.0;
    const double newSize = QApplication::font().pointSizeF();
    const double scale = newSize / baseSize;

    QTextCursor cursor(doc);
    cursor.beginEditBlock();

    QTextBlock block = doc->begin();

    while (block.isValid()) {
        for (QTextBlock::iterator it = block.begin();
             !it.atEnd(); ++it) {

            QTextFragment fragment = it.fragment();

            if (!fragment.isValid())
                continue;

            QTextCharFormat format = fragment.charFormat();

            if (format.hasProperty(QTextFormat::FontPointSize)) {
                double oldSize = format.fontPointSize();
                format.setFontPointSize(oldSize * scale);

                QTextCursor fragmentCursor(doc);
                fragmentCursor.setPosition(fragment.position());
                fragmentCursor.setPosition(
                    fragment.position() + fragment.length(),
                    QTextCursor::KeepAnchor);

                fragmentCursor.mergeCharFormat(format);
            }
        }

        block = block.next();
    }

    cursor.endEditBlock();
}

About::~About()
{
    delete ui;
}

void About::on_actionAbout_clicked()
{
    close();
}

