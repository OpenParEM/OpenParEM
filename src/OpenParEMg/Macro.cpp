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

#include "Macro.h"
#include "OPEMg.h"
#include "ui_OPEMg.h"
#include <qapplication.h>

Macro::Macro (OpenParEMg *mw_)
{
    mw=mw_;
    reset();
}

void Macro::reset ()
{
    loaded=false;
    running=false;
    stopping=false;
    text.clear();
    log.clear();
    reset_loop();
}

void Macro::reset_loop ()
{
    loop.variable="";
    loop.start=0;
    loop.end=0;
    loop.step=0;
    loop.bodyStart=0;
    loop.bodyEnd=0;
    loop.active=false;
    loop.count=0;
}

int Macro::forLoop (std::vector<std::string> &tokens, ForLoop *loop)
{
    if (tokens.size() == 8 && tokens[0].compare("For") == 0) {
        loop->variable=tokens[1];
        if (tokens[2].compare("=") != 0) {
            //log.append("ERROR: typo in For loop.\n");
            std::cout << "ERROR: typo in For loop." << std::endl; std::cout.flush();
            return 2;
        }
        loop->start=std::stoi(tokens[3]);
        if (tokens[4].compare("To") != 0) {
            //log.append("ERROR: typo in For loop.\n");
            std::cout << "ERROR: typo in For loop." << std::endl; std::cout.flush();
            return 2;
        }
        loop->end=std::stoi(tokens[5]);
        if (tokens[6].compare("Step") != 0) {
            //log.append("ERROR: typo in For loop.\n");
            std::cout << "ERROR: typo in For loop." << std::endl; std::cout.flush();
            return 2;
        }
        loop->step=std::stoi(tokens[7]);

        //log.append("for loop\n");
        std::cout << "for loop" << std::endl; std::cout.flush();

        return 1;
    }
    return 0;
}

int Macro::endForLoop (std::vector<std::string> &tokens, ForLoop *loop)
{
    if (tokens.size() == 1 && tokens[0].compare("EndFor") == 0) {
        //log.append("end for loop\n");
        std::cout << "end for loop" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::newProject (std::vector<std::string> &tokens)
{
    if (tokens.size() == 1 && tokens[0].compare("newProject") == 0) {
        if (mw->projectFileLoaded) {
            if (mw->projectChanged) {
                //log.append("ERROR: newProject failed due to existing modified project.\n");
                std::cout << "ERROR: newProject failed due to existing modified project." << std::endl;
                return 2;
            }
            mw->on_actionClose_triggered();
        } else {
            mw->on_actionNew_triggered();
        }
        //log.append("newProject\n");
        std::cout << "newProject" << std::endl;
        return 1;
    }
    return 0;
}

int Macro::closeProject (std::vector<std::string> &tokens)
{
    if (tokens.size() == 1 && tokens[0].compare("closeProject") == 0) {
        if (mw->projectFileLoaded) {
            if (mw->projectChanged) {
                //log.append("ERROR: closeProject failed due to modified project.");
                std::cout << "ERROR: closeProject failed due to modified project." << std::endl;
                return 2;
            }
            mw->on_actionClose_triggered();
        }
        //log.append("closeProject\n");
        std::cout << "closeProject" << std::endl;
        return 1;
    }
    return 0;
}

int Macro::drawRectangle (std::vector<std::string> &tokens)
{
    if (tokens.size() == 13 && tokens[0].compare("drawRectangle") == 0) {
        if (mw->projectFileLoaded) {
            mw->createRectangle(QString::fromStdString(tokens[1]),std::stod(tokens[2]),std::stod(tokens[3]),std::stod(tokens[4]),
                                std::stod(tokens[5]),std::stod(tokens[6]),std::stod(tokens[7]),
                                std::stod(tokens[8]),std::stod(tokens[9]),std::stod(tokens[10]),
                                std::stod(tokens[11]),std::stod(tokens[12]));
        } else {
            //log.append("ERROR: drawRectangle failed due to no active project.");
            std::cout << "ERROR: drawRectangle failed due to no active project." << std::endl;
            return 2;
        }

        //log.append("tokens[0]"drawRectangle\n");
        std::cout << "drawRectangle" << std::endl;
        return 1;
    }
    return 0;
}

int Macro::drawPolycircle (std::vector<std::string> &tokens)
{
    if (tokens.size() == 12 && tokens[0].compare("drawPolycircle") == 0) {
        if (mw->projectFileLoaded) {
            mw->createPolycircle(QString::fromStdString(tokens[1]),std::stod(tokens[2]),std::stod(tokens[3]),std::stod(tokens[4]),
                                std::stod(tokens[5]),std::stod(tokens[6]),std::stod(tokens[7]),
                                std::stod(tokens[8]),std::stod(tokens[9]),std::stod(tokens[10]),
                                std::stoi(tokens[11]));
        } else {
            //log.append("ERROR: drawPolycircle failed due to no active project.");
            std::cout << "ERROR: drawPolycircle failed due to no active project." << std::endl;
            return 2;
        }

        //log.append("tokens[0]"drawPolycircle\n");
        std::cout << "drawPolycircle" << std::endl;
        return 1;
    }
    return 0;
}

int Macro::undo (std::vector<std::string> &tokens)
{
    if (tokens.size() == 1 && tokens[0].compare("undo") == 0) {
        if (mw->itemChangesStack.hasUndo()) {
            mw->on_actionUndo_triggered();
        } else {
            //log.append("ERROR: undo failed with no active undo.");
            std::cout << "ERROR: undo failed with no active undo." << std::endl;
            return 2;
        }
        //log.append("undo\n");
        std::cout << "undo" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::redo (std::vector<std::string> &tokens)
{
    if (tokens.size() == 1 && tokens[0].compare("redo") == 0) {
        if (mw->itemChangesStack.hasRedo()) {
            mw->on_actionRedo_triggered();
        } else {
            //log.append("ERROR: redo failed with no active redo.");
            std::cout << "ERROR: redo failed with no active redo." << std::endl;
            return 2;
        }
        //log.append("redo\n");
        std::cout << "redo" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::fitAll (std::vector<std::string> &tokens)
{
    if (tokens.size() == 1 && tokens[0].compare("fitAll") == 0) {
        if (mw->projectFileLoaded) {
            mw->on_actionFitAll_triggered();
        } else {
            //log.append("ERROR: fitAll failed due to no active project");
            std::cout << "ERROR: fitAll failed due to no active project" << std::endl;
            return 2;
        }
        //log.append("fitAll\n");
        std::cout << "fitAll" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}


int Macro::unselectAllObjects (std::vector<std::string> &tokens)
{
    if (tokens.size() == 1 && tokens[0].compare("unselectAllObjects") == 0) {
        if (mw->projectFileLoaded) {
            mw->on_actionUnselectAll_triggered();
        } else {
            //log.append("ERROR: unselectAllObjects failed due to no active project");
            std::cout << "ERROR: unselectAllObjects failed due to no active project" << std::endl;
            return 2;
        }
        //log.append("unselectAllObjects\n");
        std::cout << "unselectAllObjects" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::selectObject (std::vector<std::string> &tokens)
{
    if (tokens.size() == 2 && tokens[0].compare("selectObject") == 0) {
        if (mw->projectFileLoaded) {
            mw->selectDrawingItem(QString::fromStdString(tokens[1]));
        } else {
            //log.append("ERROR: selectObject failed due to no active project");
            std::cout << "ERROR: selectObject failed due to no active project" << std::endl;
            return 2;
        }
        //log.append("selectObject\n");
        std::cout << "selectObject" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::deleteSelectedObjects (std::vector<std::string> &tokens)
{
    if (tokens.size() == 1 && tokens[0].compare("deleteSelectedObjects") == 0) {
        if (mw->projectFileLoaded) {
            mw->deleteDrawingItems();
        } else {
            //log.append("ERROR: deleteSelectedObjects failed due to no active project");
            std::cout << "ERROR: deleteSelectedObjects failed due to no active project" << std::endl;
            return 2;
        }
        //log.append("deleteSelectedObjects\n");
        std::cout << "deleteSelectedObjects" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::extrudeSelectedObjects (std::vector<std::string> &tokens)
{
    if (tokens.size() == 3 && tokens[0].compare("extrudeSelectedObjects") == 0) {
        if (mw->projectFileLoaded) {
            mw->setLength(std::stod(tokens[2]));
            mw->finishExtrudePolywire();
            mw->renameSelectedDrawingItem(QString::fromStdString(tokens[1]));
        } else {
            //log.append("ERROR: extrudeSelectedObjects failed due to no active project");
            std::cout << "ERROR: extrudeSelectedObjects failed due to no active project" << std::endl;
            return 2;
        }
        //log.append("extrudeSelectedObjects\n");
        std::cout << "extrudeSelectedObjects" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::mergeSelectedObjects (std::vector<std::string> &tokens)
{
    if (tokens.size() == 2 && tokens[0].compare("mergeSelectedObjects") == 0) {
        if (mw->projectFileLoaded) {
            if (mw->ui->drawingWindow->get_selectedItems_size() == 2) {
                mw->mergeSolids();
                mw->renameSelectedDrawingItem(QString::fromStdString(tokens[1]));
            } else {
                //log.append("ERROR: mergeSelectedObjects failed because two objects are not selected.");
                std::cout << "ERROR: mergeSelectedObjects failed because two objects are not selected." << std::endl;
                return 2;
            }
        } else {
            //log.append("ERROR: mergeSelectedObjects failed due to no active project.");
            std::cout << "ERROR: mergeSelectedObjects failed due to no active project." << std::endl;
            return 2;
        }
        //log.append("mergeSelectedObjects\n");
        std::cout << "mergeSelectedObjects" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::subtractSelectedObjects (std::vector<std::string> &tokens)
{
    if (tokens.size() == 2 && tokens[0].compare("subtractSelectedObjects") == 0) {
        if (mw->projectFileLoaded) {
            if (mw->ui->drawingWindow->get_selectedItems_size() == 2) {
                mw->subtractSolids();
                mw->renameSelectedDrawingItem(QString::fromStdString(tokens[1]));
            } else {
                //log.append("ERROR: subtractSelectedObjects failed because two objects are not selected.");
                std::cout << "ERROR: subtractSelectedObjects failed because two objects are not selected." << std::endl;
                return 2;
            }
        } else {
            //log.append("ERROR: subtractSelectedObjects failed due to no active project.");
            std::cout << "ERROR: subtractSelectedObjects failed due to no active project." << std::endl;
            return 2;
        }
        //log.append("subtractSelectedObjects\n");
        std::cout << "subtractSelectedObjects" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}


void Macro::run ()
{
    QStringList lines=text.split('\n');

    // parse inputs and perform actions
    int i=0;
    while (i < lines.size()) {

        //std::cout << "i=" << i << "  loopActive=" << loop.active << std::endl; std::cout.flush();

        QString &line=lines[i];

        // skip comment lines
        if (!line.trimmed().startsWith('#')) {

            // remove trailing comments
            int index=line.indexOf('#');
            if (index != -1) {
                line=line.left(index).trimmed();
            }

            if (loop.active) {
                //std::cout << "  loop.bodyStart=" << loop.bodyStart << "loop.bodyEnd=" << loop.bodyEnd << "  loop.count=" << loop.count << std::endl; std::cout.flush();
                if (loop.bodyEnd == 0) {
                    // keep going
                } else if (i > loop.bodyEnd) {
                    loop.count+=loop.step;
                    std::cout << "  loop count=" << loop.count << std::endl; std::cout.flush();
                    if (loop.count >= loop.end) {
                        i=loop.bodyEnd;
                        i++;
                        reset_loop();
                        continue;
                    }
                    i=loop.bodyStart;
                    continue;
                }
            }

            std::vector<std::string> tokens=splitWhitespace(line.toStdString());

            if (tokens.size() > 0) {
                int retVal;

                retVal=forLoop(tokens,&loop);
                if (retVal > 0) {
                    loop.active=true;
                    loop.bodyStart=i+1;
                    loop.bodyEnd=0;
                    i++; continue;
                }

                retVal=endForLoop(tokens,&loop);
                if (retVal > 0) {
                    loop.bodyEnd=i-1;
                    i++; continue;
                }

                retVal=newProject(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=closeProject(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=drawRectangle(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=drawPolycircle(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=undo(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=fitAll(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=redo(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=unselectAllObjects(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=selectObject(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=deleteSelectedObjects(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=extrudeSelectedObjects(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=mergeSelectedObjects(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=subtractSelectedObjects(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                std::string message="ERROR: Unrecognized macro command \"";
                message.append(line.toStdString());
                message.append("\".");
                //log.append(message);
                std::cout << message << std::endl; std::cout.flush();
            }

        }

        if (stopping) {
            //log.append("Macro execution stopped.\n");
            std::cout << "Macro execution stopped." << std::endl; std::cout.flush();
            return;
        }

        //mw->ui->drawingWindow->updateViewer();
        //QApplication::processEvents(QEventLoop::AllEvents,10);

        i++;
    }

    return;
}
