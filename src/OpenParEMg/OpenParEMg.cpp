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

#include "OPEMg.h"

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QIcon>
#include <QtGui/qguiapplication_platform.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "commandLine.hpp"

int version_major=2;
int version_minor=2;
int version_patch=2;   

void help () {   
   printf("usage: OpenParEMg  [--help] filename|macroname [logfile]\n");
   printf("       --help      : Print this help text; can also use -h, --h, and -help\n");
   printf("       filename    : Filename of an OpenParEM3D setup file or an OpenParEMg macro file.\n");
   printf("       logfile     : Output is redirected to the given file.  Note that the file is appended.\n");
   printf("\nOpenParEMg is a front-end graphical user interface for the OpenParEM3D electromagnetic solver.\n");
   printf("Version %d.%d.%d\n",version_major,version_minor,version_patch);
}

int main(int argc, char *argv[])
{
    // command line

    CommandLine commandLine;
    std::vector<std::string> arguments;
    int i=0;
    while (i < argc) {
       arguments.push_back(std::string(argv[i]));
       i++; 
    }
    commandLine.set_arguments(&arguments);
    commandLine.parse();
    //commandLine.print();

    // help
    if (commandLine.help()) {
       help();
       exit(0);
    }


    qputenv("QT_QPA_PLATFORM","xcb");
    qputenv("QT_QPA_PLATFORMTHEME","generic");

    QApplication a(argc, argv);

    OpenParEMg w;
    w.setWindowIcon(QIcon(":/images/images/logo2.svg"));
    a.setDesktopFileName("OpenParEMg");

    // use X11 functions to change the color from black to white
    WId id=w.winId();
    Display *display=qApp->nativeInterface<QNativeInterface::QX11Application>()->display();
    XSetWindowBackground(display,id,WhitePixel(display,DefaultScreen(display)));
    XFlush(display);

    // valid input file
    if (commandLine.is_OpenParEM3D_project()) {
       w.set_project_file(commandLine.get_filename());
       w.set_directory(commandLine.get_directory());
    }

    if (commandLine.is_macro()) {
       w.set_macro_file(commandLine.get_filename());
       w.set_directory(commandLine.get_directory());
    }

    // change working directory
    if (!commandLine.get_directory().empty()) {
       std::error_code ec;
       std::filesystem::current_path(commandLine.get_directory(),ec);
       if (ec) {
          printf("ERROR: Failed to switch to the project directory.\n");
          exit(1);
       }
    }

    // log file - redirect stdout if a log name is provided
    if (commandLine.has_logFilename()) {
       std::ios_base::sync_with_stdio(true);

       std::ofstream clearLog(argv[2],std::ios::trunc);
       std::FILE* stream=std::freopen(commandLine.get_logFilename().c_str(),"a",stdout);  // append
       if (stream == nullptr) {
          std::cout << "ERROR: Failed to redirect stdout to log file \"" << commandLine.get_logFilename().c_str() << "\"." << std::endl;
          exit(1);
       }
    }

    w.show();

    // finish initialization
    w.initialize();

    // execute if command line files are provided
    w.execute();

    return a.exec();
}

