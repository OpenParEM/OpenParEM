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
#include <qdir.h>
#include <qfileinfo.h>
#include <QProcess>
#include <iostream>
#include <filesystem>
#include <thread>

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
    if (tokens.size() == 3 && tokens[0].compare("newProject") == 0) {
        if (mw->projectFileLoaded) {
            if (mw->projectChanged) {
                //log.append("ERROR: newProject failed due to existing modified project.\n");
                std::cout << "ERROR: newProject failed due to existing modified project." << std::endl;
                return 2;
            }
            mw->on_actionClose_triggered();
        } else {
            mw->on_actionNew_triggered();

            // full path name
            QString filePath=QString::fromStdString(tokens[1]);
            if (!filePath.endsWith('/')) filePath.append('/');
            QString projectName=QString::fromStdString(tokens[2]);
            if (projectName.startsWith('/')) projectName.remove(0,1);
            filePath.append(projectName);

            // set the window title bar
            QString title="OpenParEMg: ";
            title.append(filePath);
            mw->setWindowTitle(title);

            QFileInfo fileInfo(filePath);

            // assign data for this project
            mw->absolutePath=fileInfo.absolutePath();
            mw->projectFile=fileInfo.fileName();
            mw->projectName=fileInfo.completeBaseName();
            set_project_name(&(mw->projData),mw->projectName.toStdString().c_str());
            QDir::setCurrent(mw->absolutePath);
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

int Macro::forceCloseProject (std::vector<std::string> &tokens)
{
    if (tokens.size() == 1 && tokens[0].compare("forceCloseProject") == 0) {
        if (mw->projectFileLoaded) {
            mw->resetProject();
        }
        //log.append("forceCloseProject\n");
        std::cout << "forceCloseProject" << std::endl;
        return 1;
    }
    return 0;
}

int Macro::setDrawingPlane (std::vector<std::string> &tokens)
{
    if (tokens.size() == 10 && tokens[0].compare("setDrawingPlane") == 0) {
        mw->setPlane(gp_Pnt(std::stod(tokens[1]),std::stod(tokens[2]),std::stod(tokens[3])),
                     gp_Pnt(std::stod(tokens[4]),std::stod(tokens[5]),std::stod(tokens[6])),
                     gp_Dir(gp_Vec(gp_Pnt(0,0,0),gp_Pnt(std::stod(tokens[7]),std::stod(tokens[8]),std::stod(tokens[9])))));

        //log.append("tokens[0]"setDrawingPlane\n");
        std::cout << "setDrawingPlane" << std::endl;
        return 1;
    }
    return 0;
}

int Macro::setDrawingPlaneToXY (std::vector<std::string> &tokens)
{
    if (tokens.size() == 1 && tokens[0].compare("setDrawingPlaneToXY") == 0) {
        mw->on_actionDrawingSetPlaneToXY_triggered();
        //log.append("tokens[0]"setDrawingPlaneToXY\n");
        std::cout << "setDrawingPlaneToXY" << std::endl;
        return 1;
    }
    return 0;
}

int Macro::setDrawingPlaneToXZ (std::vector<std::string> &tokens)
{
    if (tokens.size() == 1 && tokens[0].compare("setDrawingPlaneToXZ") == 0) {
        mw->on_actionDrawingSetPlaneToXZ_triggered();
        //log.append("tokens[0]"setDrawingPlaneToXZ\n");
        std::cout << "setDrawingPlaneToXZ" << std::endl;
        return 1;
    }
    return 0;
}

int Macro::setDrawingPlaneToYZ (std::vector<std::string> &tokens)
{
    if (tokens.size() == 1 && tokens[0].compare("setDrawingPlaneToYZ") == 0) {
        mw->on_actionDrawingSetPlaneToYZ_triggered();
        //log.append("tokens[0]"setDrawingPlaneToYZ\n");
        std::cout << "setDrawingPlaneToYZ" << std::endl;
        return 1;
    }
    return 0;
}

int Macro::drawLine (std::vector<std::string> &tokens)
{
    if (tokens.size() == 8 && tokens[0].compare("drawLine") == 0) {
        if (mw->projectFileLoaded) {
            mw->on_actionDrawLine_triggered();
            mw->getPickedVertex(gp_Pnt(std::stod(tokens[2]),std::stod(tokens[3]),std::stod(tokens[4])),false);
            mw->getPickedVertex(gp_Pnt(std::stod(tokens[5]),std::stod(tokens[6]),std::stod(tokens[7])),false);
            mw->renameSelectedDrawingItem(QString::fromStdString(tokens[1]));
        } else {
            //log.append("ERROR: drawLine failed due to no active project.");
            std::cout << "ERROR: drawLine failed due to no active project." << std::endl;
            return 2;
        }

        //log.append("tokens[0]"drawLine\n");
        std::cout << "drawLine" << std::endl;
        return 1;
    }
    return 0;
}

int Macro::drawPolyline (std::vector<std::string> &tokens)
{
    if (tokens.size() > 8 && tokens[0].compare("drawPolyline") == 0) {
        if (mw->projectFileLoaded) {
            if (std::stoi(tokens[2])*3+3 == tokens.size()) {
                mw->on_actionDrawPolyline_triggered();
                int i=0;
                while (i < std::stoi(tokens[2])) {
                    mw->getPickedVertex(gp_Pnt(std::stod(tokens[i*3+3]),std::stod(tokens[i*3+4]),std::stod(tokens[i*3+5])),false);
                    i++;
                }
                mw->finishDraw();
                mw->renameSelectedDrawingItem(QString::fromStdString(tokens[1]));
            } else {
                //log.append("ERROR: drawPolyline failed due to invalid number of points.");
                std::cout << "ERROR: drawPolyline failed due to invalid number of points." << std::endl;
                return 2;
            }
        } else {
            //log.append("ERROR: drawPolyline failed due to no active project.");
            std::cout << "ERROR: drawPolyline failed due to no active project." << std::endl;
            return 2;
        }

        //log.append("tokens[0]"drawPolyline\n");
        std::cout << "drawPolyline" << std::endl;
        return 1;
    }
    return 0;
}

int Macro::drawRectangle (std::vector<std::string> &tokens)
{
    if (tokens.size() == 8 && tokens[0].compare("drawRectangle") == 0) {
        if (mw->projectFileLoaded) {
            // mw->createRectangle(QString::fromStdString(tokens[1]),std::stod(tokens[2]),std::stod(tokens[3]),std::stod(tokens[4]),
            //                     std::stod(tokens[5]),std::stod(tokens[6]),std::stod(tokens[7]),
            //                     std::stod(tokens[8]),std::stod(tokens[9]),std::stod(tokens[10]),
            //                     std::stod(tokens[11]),std::stod(tokens[12]));
            mw->on_actionDrawRectangle_triggered();
            mw->getPickedVertex(gp_Pnt(std::stod(tokens[2]),std::stod(tokens[3]),std::stod(tokens[4])),false);
            mw->getPickedVertex(gp_Pnt(std::stod(tokens[5]),std::stod(tokens[6]),std::stod(tokens[7])),false);
            mw->renameSelectedDrawingItem(QString::fromStdString(tokens[1]));
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
    if (tokens.size() == 8 && tokens[0].compare("drawPolycircle") == 0) {
        if (mw->projectFileLoaded) {
            // mw->createPolycircle(QString::fromStdString(tokens[1]),std::stod(tokens[2]),std::stod(tokens[3]),std::stod(tokens[4]),
            //                     std::stod(tokens[5]),std::stod(tokens[6]),std::stod(tokens[7]),
            //                     std::stod(tokens[8]),std::stod(tokens[9]),std::stod(tokens[10]),
            //                     std::stoi(tokens[11]));
            mw->on_actionDrawPolycircle_triggered();
            mw->getPickedVertex(gp_Pnt(std::stod(tokens[2]),std::stod(tokens[3]),std::stod(tokens[4])),false);
            mw->getPickedVertex(gp_Pnt(std::stod(tokens[5]),std::stod(tokens[6]),std::stod(tokens[7])),false);
            mw->renameSelectedDrawingItem(QString::fromStdString(tokens[1]));
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

int Macro::deleteSubtreeSelectedObjects (std::vector<std::string> &tokens)
{
    if (tokens.size() == 1 && tokens[0].compare("deleteSubtreeSelectedObjects") == 0) {
        if (mw->projectFileLoaded) {
            mw->deletePlusDrawingItems();
        } else {
            //log.append("ERROR: deleteSubtreeSelectedObjects failed due to no active project");
            std::cout << "ERROR: deleteSubtreeSelectedObjects failed due to no active project" << std::endl;
            return 2;
        }
        //log.append("deleteSubtreeSelectedObjects\n");
        std::cout << "deleteSubtreeSelectedObjects" << std::endl; std::cout.flush();
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

int Macro::moveSelectedObjects (std::vector<std::string> &tokens)
{
    if (tokens.size() == 4 && tokens[0].compare("moveSelectedObjects") == 0) {
        if (mw->projectFileLoaded) {
            if (mw->ui->drawingWindow->get_selectedItems_size() > 0) {
                mw->moveObject();
                mw->getPickedVertex(gp_Pnt(0,0,0),false);
                mw->getPickedVertex(gp_Pnt(std::stod(tokens[1]),std::stod(tokens[2]),std::stod(tokens[3])),false);
            } else {
                //log.append("ERROR: moveSelectedObjects failed because no objects are selected.");
                std::cout << "ERROR: moveSelectedObjects failed because no objects are selected." << std::endl;
                return 2;
            }
        } else {
            //log.append("ERROR: moveSelectedObjects failed due to no active project.");
            std::cout << "ERROR: moveSelectedObjects failed due to no active project." << std::endl;
            return 2;
        }
        //log.append("moveSelectedObjects\n");
        std::cout << "moveSelectedObjects" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::rotateSelectedObjects (std::vector<std::string> &tokens)
{
    if (tokens.size() == 8 && tokens[0].compare("rotateSelectedObjects") == 0) {
        if (mw->projectFileLoaded) {
            if (mw->ui->drawingWindow->get_selectedItems_size() > 0) {
                mw->setStartPoint(std::stod(tokens[1]),std::stod(tokens[2]),std::stod(tokens[3]));
                mw->setEndPoint(std::stod(tokens[4]),std::stod(tokens[5]),std::stod(tokens[6]));
                mw->setAngle(std::stod(tokens[7]));
                mw->rotateObject();
                if (mw->rotateInputForm) {
                    mw->rotateInputForm->on_OkButton_clicked();
                } else {
                    //log.append("ERROR: rotateSelectedObjects failed on invalid form.");
                    std::cout << "ERROR: rotateSelectedObjects failed on invalid form" << std::endl;
                    return 2;
                }
            } else {
                //log.append("ERROR: rotateSelectedObjects failed because no objects are selected.");
                std::cout << "ERROR: rotateSelectedObjects failed because no objects are selected." << std::endl;
                return 2;
            }
        } else {
            //log.append("ERROR: rotateSelectedObjects failed due to no active project.");
            std::cout << "ERROR: rotateSelectedObjects failed due to no active project." << std::endl;
            return 2;
        }
        //log.append("rotateSelectedObjects\n");
        std::cout << "rotateSelectedObjects" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::copySelectedObjects (std::vector<std::string> &tokens)
{
    if (tokens.size() == 1 && tokens[0].compare("copySelectedObjects") == 0) {
        if (mw->projectFileLoaded) {
            if (mw->ui->drawingWindow->get_selectedItems_size() > 0) {
                mw->copyDrawingItems();
            } else {
                //log.append("ERROR: copySelectedObjects failed because no objects are selected.");
                std::cout << "ERROR: copySelectedObjects failed because no objects are selected." << std::endl;
                return 2;
            }
        } else {
            //log.append("ERROR: copySelectedObjects failed due to no active project.");
            std::cout << "ERROR: copySelectedObjects failed due to no active project." << std::endl;
            return 2;
        }
        //log.append("copySelectedObjects\n");
        std::cout << "copySelectedObjects" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::convertSelectedObjectsToPolylines (std::vector<std::string> &tokens)
{
    if (tokens.size() == 1 && tokens[0].compare("convertSelectedObjectsToPolylines") == 0) {
        if (mw->projectFileLoaded) {
            if (mw->ui->drawingWindow->get_selectedItems_size() > 0) {
                mw->convertToPolyline();
            } else {
                //log.append("ERROR: convertSelectedObjectsToPolylines failed because no objects are selected.");
                std::cout << "ERROR: convertSelectedObjectsToPolylines failed because no objects are selected." << std::endl;
                return 2;
            }
        } else {
            //log.append("ERROR: convertSelectedObjectsToPolylines failed due to no active project.");
            std::cout << "ERROR: convertSelectedObjectsToPolylines failed due to no active project." << std::endl;
            return 2;
        }
        //log.append("convertSelectedObjectsToPolylines\n");
        std::cout << "convertSelectedObjectsToPolylines" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::convertSelectedObjectsToPaths (std::vector<std::string> &tokens)
{
    if (tokens.size() == 1 && tokens[0].compare("convertSelectedObjectsToPaths") == 0) {
        if (mw->projectFileLoaded) {
            if (mw->ui->drawingWindow->get_selectedItems_size() > 0) {
                mw->convertDrawingToPathN(true);
            } else {
                //log.append("ERROR: convertSelectedObjectsToPaths failed because no objects are selected.");
                std::cout << "ERROR: convertSelectedObjectsToPaths failed because no objects are selected." << std::endl;
                return 2;
            }
        } else {
            //log.append("ERROR: convertSelectedObjectsToPaths failed due to no active project.");
            std::cout << "ERROR: convertSelectedObjectsToPaths failed due to no active project." << std::endl;
            return 2;
        }
        //log.append("convertSelectedObjectsToPaths\n");
        std::cout << "convertSelectedObjectsToPaths" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::convertSelectedObjectsToPorts (std::vector<std::string> &tokens)
{
    if (tokens.size() == 1 && tokens[0].compare("convertSelectedObjectsToPorts") == 0) {
        if (mw->projectFileLoaded) {
            if (mw->ui->drawingWindow->get_selectedItems_size() > 0) {
                mw->convertDrawingToPort();
            } else {
                //log.append("ERROR: convertSelectedObjectsToPorts failed because no objects are selected.");
                std::cout << "ERROR: convertSelectedObjectsToPorts failed because no objects are selected." << std::endl;
                return 2;
            }
        } else {
            //log.append("ERROR: convertSelectedObjectsToPorts failed due to no active project.");
            std::cout << "ERROR: convertSelectedObjectsToPorts failed due to no active project." << std::endl;
            return 2;
        }
        //log.append("convertSelectedObjectsToPorts\n");
        std::cout << "convertSelectedObjectsToPorts" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::convertSelectedObjectsToBoundaries (std::vector<std::string> &tokens)
{
    if (tokens.size() == 1 && tokens[0].compare("convertSelectedObjectsToBoundaries") == 0) {
        if (mw->projectFileLoaded) {
            if (mw->ui->drawingWindow->get_selectedItems_size() > 0) {
                mw->convertDrawingToBoundary();
            } else {
                //log.append("ERROR: convertSelectedObjectsToBoundaries failed because no objects are selected.");
                std::cout << "ERROR: convertSelectedObjectsToBoundaries failed because no objects are selected." << std::endl;
                return 2;
            }
        } else {
            //log.append("ERROR: convertSelectedObjectsToBoundaries failed due to no active project.");
            std::cout << "ERROR: convertSelectedObjectsToBoundaries failed due to no active project." << std::endl;
            return 2;
        }
        //log.append("convertSelectedObjectsToBoundaries\n");
        std::cout << "convertSelectedObjectsToBoundaries" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::unselectAllPaths (std::vector<std::string> &tokens)
{
    if (tokens.size() == 1 && tokens[0].compare("unselectAllPaths") == 0) {
        if (mw->projectFileLoaded) {
            mw->unselectPathItems();
        } else {
            //log.append("ERROR: unselectAllPaths failed due to no active project.");
            std::cout << "ERROR: unselectAllPaths failed due to no active project." << std::endl;
            return 2;
        }
        //log.append("unselectAllPaths\n");
        std::cout << "unselectAllPaths" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::unselectAllPorts (std::vector<std::string> &tokens)
{
    if (tokens.size() == 1 && tokens[0].compare("unselectAllPorts") == 0) {
        if (mw->projectFileLoaded) {
            mw->unselectPortItems();
        } else {
            //log.append("ERROR: unselectAllPorts failed due to no active project.");
            std::cout << "ERROR: unselectAllPorts failed due to no active project." << std::endl;
            return 2;
        }
        //log.append("unselectAllPorts\n");
        std::cout << "unselectAllPorts" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::unselectAllBoundaries (std::vector<std::string> &tokens)
{
    if (tokens.size() == 1 && tokens[0].compare("unselectAllBoundaries") == 0) {
        if (mw->projectFileLoaded) {
            mw->unselectBoundaryItems();
        } else {
            //log.append("ERROR: unselectAllBoundaries failed due to no active project.");
            std::cout << "ERROR: unselectAllBoundaries failed due to no active project." << std::endl;
            return 2;
        }
        //log.append("unselectAllBoundaries\n");
        std::cout << "unselectAllBoundaries" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::selectPath (std::vector<std::string> &tokens)
{
    if (tokens.size() == 2 && tokens[0].compare("selectPath") == 0) {
        if (mw->projectFileLoaded) {
            mw->selectPathItem(QString::fromStdString(tokens[1]));
        } else {
            //log.append("ERROR: selectPath failed due to no active project.");
            std::cout << "ERROR: selectPath failed due to no active project." << std::endl;
            return 2;
        }
        //log.append("selectPath\n");
        std::cout << "selectPath" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::selectPort (std::vector<std::string> &tokens)
{
    if (tokens.size() == 2 && tokens[0].compare("selectPort") == 0) {
        if (mw->projectFileLoaded) {
            mw->selectPortItem(QString::fromStdString(tokens[1]));
        } else {
            //log.append("ERROR: selectPort failed due to no active project.");
            std::cout << "ERROR: selectPort failed due to no active project." << std::endl;
            return 2;
        }
        //log.append("selectPort\n");
        std::cout << "selectPort" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::selectBoundary (std::vector<std::string> &tokens)
{
    if (tokens.size() == 2 && tokens[0].compare("selectBoundary") == 0) {
        if (mw->projectFileLoaded) {
            mw->selectBoundaryItem(QString::fromStdString(tokens[1]));
        } else {
            //log.append("ERROR: selectBoundary failed due to no active project.");
            std::cout << "ERROR: selectBoundary failed due to no active project." << std::endl;
            return 2;
        }
        //log.append("selectBoundary\n");
        std::cout << "selectBoundary" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::deleteSelectedPaths (std::vector<std::string> &tokens)
{
    if (tokens.size() == 1 && tokens[0].compare("deleteSelectedPaths") == 0) {
        if (mw->projectFileLoaded) {
            mw->deletePathItems();
        } else {
            //log.append("ERROR: deleteSelectedPaths failed due to no active project.");
            std::cout << "ERROR: deleteSelectedPaths failed due to no active project." << std::endl;
            return 2;
        }
        //log.append("deleteSelectedPaths\n");
        std::cout << "deleteSelectedPaths" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::deleteSelectedPorts (std::vector<std::string> &tokens)
{
    if (tokens.size() == 1 && tokens[0].compare("deleteSelectedPorts") == 0) {
        if (mw->projectFileLoaded) {
            mw->deletePortItems();
        } else {
            //log.append("ERROR: deleteSelectedPorts failed due to no active project.");
            std::cout << "ERROR: deleteSelectedPorts failed due to no active project." << std::endl;
            return 2;
        }
        //log.append("deleteSelectedPorts\n");
        std::cout << "deleteSelectedPorts" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::deleteSelectedBoundaries (std::vector<std::string> &tokens)
{
    if (tokens.size() == 1 && tokens[0].compare("deleteSelectedBoundaries") == 0) {
        if (mw->projectFileLoaded) {
            mw->deleteBoundaryItems();
        } else {
            //log.append("ERROR: deleteSelectedBoundaries failed due to no active project.");
            std::cout << "ERROR: deleteSelectedBoundaries failed due to no active project." << std::endl;
            return 2;
        }
        //log.append("deleteSelectedBoundaries\n");
        std::cout << "deleteSelectedBoundaries" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::selectPortVoltageObject (std::vector<std::string> &tokens)
{
    if (tokens.size() == 2 && tokens[0].compare("selectPortVoltageObject") == 0) {
        if (mw->projectFileLoaded) {
            mw->selectPortVoltageItem(QString::fromStdString(tokens[1]));
        } else {
            //log.append("ERROR: selectPortVoltageObject failed due to no active project.");
            std::cout << "ERROR: selectPortVoltageObject failed due to no active project." << std::endl;
            return 2;
        }
        //log.append("selectPortVoltageObject\n");
        std::cout << "selectPortVoltageObject" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::selectPortCurrentObject (std::vector<std::string> &tokens)
{
    if (tokens.size() == 2 && tokens[0].compare("selectPortCurrentObject") == 0) {
        if (mw->projectFileLoaded) {
            mw->selectPortCurrentItem(QString::fromStdString(tokens[1]));
        } else {
            //log.append("ERROR: selectPortCurrentObject failed due to no active project.");
            std::cout << "ERROR: selectPortCurrentObject failed due to no active project." << std::endl;
            return 2;
        }
        //log.append("selectPortCurrentObject\n");
        std::cout << "selectPortCurrentObject" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::drawPortIntegrationLine (std::vector<std::string> &tokens)
{
    if (tokens.size() == 7 && tokens[0].compare("drawPortIntegrationLine") == 0) {
        if (mw->projectFileLoaded) {
            mw->drawIntegrationLine();
            mw->getPickedVertex(gp_Pnt(std::stod(tokens[1]),std::stod(tokens[2]),std::stod(tokens[3])),false);
            mw->getPickedVertex(gp_Pnt(std::stod(tokens[4]),std::stod(tokens[5]),std::stod(tokens[6])),false);
        } else {
            //log.append("ERROR: drawPortIntegrationLine failed due to no active project.");
            std::cout << "ERROR: drawPortIntegrationLine failed due to no active project." << std::endl;
            return 2;
        }
        //log.append("drawPortIntegrationLine\n");
        std::cout << "drawPortIntegrationLine" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::drawPortIntegrationPolyline (std::vector<std::string> &tokens)
{
    if (tokens.size() > 7 && tokens[0].compare("drawPortIntegrationPolyline") == 0) {
        if (mw->projectFileLoaded) {
            if (std::stoi(tokens[1])*3+2 == tokens.size()) {
                mw->drawIntegrationPolyline();
                int i=0;
                while (i < std::stoi(tokens[1])) {
                    mw->getPickedVertex(gp_Pnt(std::stod(tokens[i*3+2]),std::stod(tokens[i*3+3]),std::stod(tokens[i*3+4])),false);
                    i++;
                }
                mw->finishDraw();
            }  else {
                //log.append("ERROR: drawPortIntegrationPolyline failed due to invalid number of points.");
                std::cout << "ERROR: drawPortIntegrationPolyline failed due to invalid number of points." << std::endl;
                return 2;
            }
        } else {
            //log.append("ERROR: drawPortIntegrationPolyline failed due to no active project.");
            std::cout << "ERROR: drawPortIntegrationPolyline failed due to no active project." << std::endl;
            return 2;
        }
        //log.append("drawPortIntegrationPolyline\n");
        std::cout << "drawPortIntegrationPolyline" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::setLocalMaterialDatabase (std::vector<std::string> &tokens)
{
    if (tokens.size() == 3 && tokens[0].compare("assignLocalMaterialDatabase") == 0) {
        if (mw->projectFileLoaded) {
            if (mw->projData.materials_local_path) free(mw->projData.materials_local_path);
            mw->projData.materials_local_path=allocCopyConstString(tokens[1].c_str());

            if (mw->projData.materials_local_name) free(mw->projData.materials_local_name);
            mw->projData.materials_local_name=allocCopyConstString(tokens[2].c_str());
        } else {
            //log.append("ERROR: assignLocalMaterialDatabase failed due to no active project.");
            std::cout << "ERROR: assignLocalMaterialDatabase failed due to no active project." << std::endl;
            return 2;
        }

        //log.append("assignLocalMaterialDatabase\n");
        std::cout << "assignLocalMaterialDatabase" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::setGlobalMaterialDatabase (std::vector<std::string> &tokens)
{
    if (tokens.size() == 3 && tokens[0].compare("setGlobalMaterialDatabase") == 0) {
        if (mw->projectFileLoaded) {
            if (mw->projData.materials_global_path) free(mw->projData.materials_global_path);
            mw->projData.materials_global_path=allocCopyConstString(tokens[1].c_str());

            if (mw->projData.materials_global_name) free(mw->projData.materials_global_name);
            mw->projData.materials_global_name=allocCopyConstString(tokens[2].c_str());
        } else {
            //log.append("ERROR: setGlobalMaterialDatabase failed due to no active project.");
            std::cout << "ERROR: setGlobalMaterialDatabase failed due to no active project." << std::endl;
            return 2;
        }

        //log.append("setGlobalMaterialDatabase\n");
        std::cout << "setGlobalMaterialDatabase" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::setMaterialToObject (std::vector<std::string> &tokens)
{
    if (tokens.size() == 3 && tokens[0].compare("setMaterialToObject") == 0) {
        if (mw->projectFileLoaded) {
            mw->assignMaterialToDrawingItem(QString::fromStdString(tokens[1]),QString::fromStdString(tokens[2]));
        } else {
            //log.append("ERROR: setMaterialToObject failed due to no active project.");
            std::cout << "ERROR: setMaterialToObject failed due to no active project." << std::endl;
            return 2;
        }

        //log.append("setMaterialToObject\n");
        std::cout << "setMaterialToObject" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::generateMesh (std::vector<std::string> &tokens)
{
    if (tokens.size() == 1 && tokens[0].compare("generateMesh") == 0) {
        if (mw->projectFileLoaded) {
            if (mw->mesh->childCount() > 0) mw->on_actionMeshDelete_triggered();
            if (mw->drawing->childCount() > 0) mw->on_actionMeshGenerate_triggered();
        } else {
            //log.append("ERROR: generateMesh failed due to no active project.");
            std::cout << "ERROR: generateMesh failed due to no active project." << std::endl;
            return 2;
        }

        //log.append("generateMesh\n");
        std::cout << "generateMesh" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::forceSave (std::vector<std::string> &tokens)
{
    if (tokens.size() == 1 && tokens[0].compare("forceSave") == 0) {
        if (mw->projectFileLoaded) {
            mw->forceSave();
        } else {
            //log.append("ERROR: forceSave failed due to no active project.");
            std::cout << "ERROR: forceSave failed due to no active project." << std::endl;
            return 2;
        }

        //log.append("forceSave\n");
        std::cout << "forceSave" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::setReferenceImpedance (std::vector<std::string> &tokens)
{
    if (tokens.size() == 2 && tokens[0].compare("setReferenceImpedance") == 0) {
        if (mw->projectFileLoaded) {
            mw->projData.reference_impedance=stod(tokens[1]);
        } else {
            //log.append("ERROR: setReferenceImpedance failed due to no active project.");
            std::cout << "ERROR: setReferenceImpedance failed due to no active project." << std::endl;
            return 2;
        }
        //log.append("setReferenceImpedance\n");
        std::cout << "setReferenceImpedance" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::setFEMorder (std::vector<std::string> &tokens)
{
    if (tokens.size() == 2 && tokens[0].compare("setFEMorder") == 0) {
        if (mw->projectFileLoaded) {
            mw->projData.fem_order=stoi(tokens[1]);
        } else {
            //log.append("ERROR: setFEMorder failed due to no active project.");
            std::cout << "ERROR: setFEMorder failed due to no active project." << std::endl;
            return 2;
        }
        //log.append("setFEMorder\n");
        std::cout << "setFEMorder" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::setCPUslotCount (std::vector<std::string> &tokens)
{
    if (tokens.size() == 2 && tokens[0].compare("setCPUslotCount") == 0) {
        if (mw->projectFileLoaded) {
            mw->projData.gui_slot_count=stoi(tokens[1]);
        } else {
            //log.append("ERROR: setCPUslotCount failed due to no active project.");
            std::cout << "ERROR: setCPUslotCount failed due to no active project." << std::endl;
            return 2;
        }
        //log.append("setCPUslotCount\n");
        std::cout << "setCPUslotCount" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::setRefinementFrequency (std::vector<std::string> &tokens)
{
    if (tokens.size() == 2 && tokens[0].compare("setRefinementFrequency") == 0) {
        if (mw->projectFileLoaded) {
            if (mw->projData.refinement_frequency) free(mw->projData.refinement_frequency);
            mw->projData.refinement_frequency=allocCopyConstString(tokens[1].c_str());
        } else {
            //log.append("ERROR: setRefinementFrequency failed due to no active project.");
            std::cout << "ERROR: setRefinementFrequency failed due to no active project." << std::endl;
            return 2;
        }
        //log.append("setRefinementFrequency\n");
        std::cout << "setRefinementFrequency" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::addSimulationFrequency (std::vector<std::string> &tokens)
{
    if (tokens.size() == 2 && tokens[0].compare("addSimulationFrequency") == 0) {
        if (mw->projectFileLoaded) {
            add_inputFrequencyPlan (&(mw->projData),2,stod(tokens[1]),-1,-1,-1,-1,0,0);
        } else {
            //log.append("ERROR: addSimulationFrequency failed due to no active project.");
            std::cout << "ERROR: addSimulationFrequency failed due to no active project." << std::endl;
            return 2;
        }
        //log.append("addSimulationFrequency\n");
        std::cout << "addSimulationFrequency" << std::endl; std::cout.flush();
        return 1;
    }
    return 0;
}

int Macro::runSimulation (std::vector<std::string> &tokens)
{
    if (tokens.size() == 1 && tokens[0].compare("runSimulation") == 0) {
        if (mw->projectFileLoaded) {
            mw->on_actionRun_triggered();

            // loop until the simulation has started
            while (!mw->simulationRunning) {
                QApplication::processEvents(QEventLoop::AllEvents);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        } else {
            //log.append("ERROR: runSimulation failed due to no active project.");
            std::cout << "ERROR: runSimulation failed due to no active project." << std::endl;
            return 2;
        }
        //log.append("runSimulation\n");
        std::cout << "runSimulation" << std::endl; std::cout.flush();

        // loop until the simulation is finished
        while (mw->simulationRunning) {
            QApplication::processEvents(QEventLoop::AllEvents);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        return 1;
    }
    return 0;
}

int Macro::runScript (std::vector<std::string> &tokens)
{
    if (tokens.size() == 2 && tokens[0].compare("runScript") == 0) {
        if (mw->projectFileLoaded) {


            std::filesystem::path filePath=tokens[1];

            if (!std::filesystem::exists(filePath)) {
                //log.append("ERROR: runScript failed due to script file not found.");
                std::cout << "ERROR: runScript failed due to script file not found." << std::endl;
                return 2;
            }

            if (std::filesystem::is_directory(filePath)) {
                //log.append("ERROR: runScript failed due to the script being a directory.");
                std::cout << "ERROR: runScript failed due to the script being a directory." << std::endl;
                return 2;
            }

            std::filesystem::perms prms=std::filesystem::status(filePath).permissions();

            if ((prms & (std::filesystem::perms::owner_exec | std::filesystem::perms::group_exec | std::filesystem::perms::others_exec)) == std::filesystem::perms::none) {
                //log.append("ERROR: runScript failed due to the script not being executable.");
                std::cout << "ERROR: runScript failed due to the script not being executable." << std::endl;
                return 2;
            }

            std::string command = "cd " + mw->absolutePath.toStdString() + " && ./" + tokens[1];
            int result=std::system(command.c_str());
            if (result > 0) {
                //log.append("ERROR: runScript failed due to a script error.");
                std::cout << "ERROR: runScript failed due to a script error." << std::endl;
                return 2;
            }
        } else {
            //log.append("ERROR: runScript failed due to no active project.");
            std::cout << "ERROR: runScript failed due to no active project." << std::endl;
            return 2;
        }
        //log.append("runScript\n");
        std::cout << "runScript" << std::endl; std::cout.flush();
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
        QString &line=lines[i];

        // skip comment lines
        if (!line.trimmed().startsWith('#')) {

            // remove trailing comments
            int index=line.indexOf('#');
            if (index != -1) {
                line=line.left(index).trimmed();
            }

            if (loop.active) {
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

                retVal=forceCloseProject(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=setDrawingPlane(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=setDrawingPlaneToXY(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=setDrawingPlaneToXZ(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=setDrawingPlaneToYZ(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=drawLine(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=drawPolyline(tokens);
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

                retVal=deleteSubtreeSelectedObjects(tokens);
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

                retVal=moveSelectedObjects(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=copySelectedObjects(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=rotateSelectedObjects(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=convertSelectedObjectsToPaths(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=convertSelectedObjectsToPolylines(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=convertSelectedObjectsToPorts(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=convertSelectedObjectsToBoundaries(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=unselectAllPaths(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=unselectAllPorts(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=unselectAllBoundaries(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=selectPath(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=selectPort(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=selectBoundary(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=deleteSelectedPaths(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=deleteSelectedPorts(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=deleteSelectedBoundaries(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=selectPortVoltageObject(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=selectPortCurrentObject(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=drawPortIntegrationLine(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=drawPortIntegrationPolyline(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=setLocalMaterialDatabase(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=setGlobalMaterialDatabase(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=setMaterialToObject(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=generateMesh(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=forceSave(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=setReferenceImpedance(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=setFEMorder(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=setCPUslotCount(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=setRefinementFrequency(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=addSimulationFrequency(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=runSimulation(tokens);
                if (retVal > 0) {
                    if (retVal == 2) return;
                    i++; continue;
                }

                retVal=runScript(tokens);
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

        i++;
    }

    return;
}
