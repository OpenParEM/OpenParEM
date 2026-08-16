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
extern "C" char* allocCopyString (char *);
extern "C" char* allocCopyConstString (const char *);
extern "C" void add_inputFrequencyPlan (struct projectData *, int, double, double, double, double, int, int, int);

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

    int openProject (std::vector<std::string> &tokens);
    int newProject (std::vector<std::string> &tokens);
    int closeProject (std::vector<std::string> &tokens);
    int forceCloseProject (std::vector<std::string> &tokens);

    int setDrawingPlane (std::vector<std::string> &tokens);
    int setDrawingPlaneToXY (std::vector<std::string> &tokens);
    int setDrawingPlaneToXZ (std::vector<std::string> &tokens);
    int setDrawingPlaneToYZ (std::vector<std::string> &tokens);
    int drawLine (std::vector<std::string> &tokens);
    int drawPolyline (std::vector<std::string> &tokens);
    int drawRectangle (std::vector<std::string> &tokens);
    int drawPolycircle (std::vector<std::string> &tokens);
    int undo (std::vector<std::string> &tokens);
    int redo (std::vector<std::string> &tokens);
    int fitAll (std::vector<std::string> &tokens);
    int unselectAllObjects (std::vector<std::string> &tokens);
    int selectObject (std::vector<std::string> &tokens);
    int deleteSelectedObjects (std::vector<std::string> &tokens);
    int deleteSubtreeSelectedObjects (std::vector<std::string> &tokens);
    int extrudeSelectedObjects (std::vector<std::string> &tokens);
    int mergeSelectedObjects (std::vector<std::string> &tokens);
    int subtractSelectedObjects (std::vector<std::string> &tokens);
    int moveSelectedObjects (std::vector<std::string> &tokens);
    int rotateSelectedObjects (std::vector<std::string> &tokens);
    int copySelectedObjects (std::vector<std::string> &tokens);

    int convertSelectedObjectsToPolylines (std::vector<std::string> &tokens);
    int convertSelectedObjectsToPaths (std::vector<std::string> &tokens);
    int convertSelectedObjectsToPorts (std::vector<std::string> &tokens);
    int convertSelectedObjectsToBoundaries (std::vector<std::string> &tokens);

    int unselectAllPaths (std::vector<std::string> &tokens);
    int unselectAllPorts (std::vector<std::string> &tokens);
    int unselectAllBoundaries (std::vector<std::string> &tokens);

    int selectPath (std::vector<std::string> &tokens);
    int selectPort (std::vector<std::string> &tokens);
    int selectBoundary (std::vector<std::string> &tokens);

    int deleteSelectedPaths (std::vector<std::string> &tokens);
    int deleteSelectedPorts (std::vector<std::string> &tokens);
    int deleteSelectedBoundaries (std::vector<std::string> &tokens);

    int selectPortVoltageObject (std::vector<std::string> &tokens);
    int selectPortCurrentObject (std::vector<std::string> &tokens);

    int drawPortIntegrationLine (std::vector<std::string> &tokens);
    int drawPortIntegrationPolyline (std::vector<std::string> &tokens);

    int setLocalMaterialDatabase (std::vector<std::string> &tokens);
    int setGlobalMaterialDatabase (std::vector<std::string> &tokens);
    int setMaterialToObject (std::vector<std::string> &tokens);

    int generateMesh (std::vector<std::string> &tokens);
    int forceSave (std::vector<std::string> &tokens);

    int setReferenceImpedance (std::vector<std::string> &tokens);
    int setFEMorder (std::vector<std::string> &tokens);
    int setCPUslotCount (std::vector<std::string> &tokens);
    int setRefinementFrequency (std::vector<std::string> &tokens);
    int addSimulationFrequency (std::vector<std::string> &tokens);

    int runSimulation (std::vector<std::string> &tokens);
    int runScript (std::vector<std::string> &tokens);

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
