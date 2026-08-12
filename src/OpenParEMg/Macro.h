////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//    OpenParEM3g - A GUI for OpenParEM3D                                     //
//    Copyright (C) 2025 Brian Young                                          //
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


#ifndef MACRO_H
#define MACRO_H

#include <qobject.h>

std::vector<std::string> splitWhitespace(const std::string& input);

class OpenParEMg;

struct ForLoop
{
    std::string variable;
    double start;
    double end;
    double step;
    int bodyStart;
    int bodyEnd;
    bool active;
    int count;
};

class Macro
{
public:
    Macro ();
    Macro (OpenParEMg *);

    void setRunning (bool running_) {running=running_;}
    void setStopping (bool stopping_) {stopping=stopping_;}
    void setText (QString text_) {text=text_; loaded=true;}

    bool isLoaded () {return loaded;}
    bool isRunning () {return running;}
    bool isStopping () {return stopping;}

    void reset ();
    void reset_loop ();

    int forLoop (std::vector<std::string> &tokens, ForLoop *loop);
    int endForLoop (std::vector<std::string> &tokens, ForLoop *loop);
    int newProject (std::vector<std::string> &tokens);
    int closeProject (std::vector<std::string> &tokens);
    int drawRectangle (std::vector<std::string> &tokens);
    int drawPolycircle (std::vector<std::string> &tokens);
    int undo (std::vector<std::string> &tokens);
    int redo (std::vector<std::string> &tokens);
    int fitAll (std::vector<std::string> &tokens);
    int unselectAllObjects (std::vector<std::string> &tokens);
    int selectObject (std::vector<std::string> &tokens);
    int deleteSelectedObjects (std::vector<std::string> &tokens);
    int extrudeSelectedObjects (std::vector<std::string> &tokens);
    int mergeSelectedObjects (std::vector<std::string> &tokens);
    int subtractSelectedObjects (std::vector<std::string> &tokens);

    void run ();

private:
    OpenParEMg *mw;
    bool loaded;
    bool running;
    bool stopping;
    QString text;
    QString log;

    ForLoop loop;
};

#endif // MACRO_H
