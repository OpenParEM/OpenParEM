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

#ifndef RECTANGLESELECTOR_H
#define RECTANGLESELECTOR_H

#pragma once

#include <QEvent>
#include <QMouseEvent>
#include <QObject>
#include <QWidget>

#include <AIS_RubberBand.hxx>
#include <AIS_InteractiveContext.hxx>
#include <V3d_View.hxx>
#include <TopoDS_Shape.hxx>
#include <Graphic3d_Vec2.hxx> // for Graphic3d_Vec2i

class RectangleSelector : public QObject
{
    Q_OBJECT
public:
    RectangleSelector (const Handle(AIS_InteractiveContext)& context,
                       const Handle(V3d_View)& view,
                       QWidget* widget)
        : QObject(widget),m_context(context),m_view(view),m_widget(widget)
    {
        m_rubber=new AIS_RubberBand();
        m_rubber->SetLineColor(Quantity_NOC_WHITE);
        if (m_widget) m_widget->installEventFilter(this);
    }

    ~RectangleSelector () override {
        if (!m_rubber.IsNull() && !m_context.IsNull()) {
            m_context->Erase(m_rubber, Standard_True);
        }
        if (!m_rubber.IsNull()) m_rubber.Nullify();
        if (m_widget) m_widget->removeEventFilter(this);
    }

signals:
    void selectionFinished ();

protected:
    bool eventFilter (QObject* obj, QEvent* ev) override {
        if (obj != m_widget) return QObject::eventFilter(obj, ev);

        switch (ev->type()) {
        case QEvent::MouseButtonPress: {
            QMouseEvent* me = static_cast<QMouseEvent*>(ev);
            if (me && me->button() == Qt::LeftButton) {
                beginSelection(me->pos().x(), me->pos().y(), me->modifiers());
                //return true;
                return false;
            }
            break;
        }
        case QEvent::MouseMove: {
            if (m_isSelecting) {
                QMouseEvent* me = static_cast<QMouseEvent*>(ev);
                if (me) {
                    updateSelection(me->pos().x(), me->pos().y());
                    //return true;
                    return false;
                }
            }
            break;
        }
        case QEvent::MouseButtonRelease: {
            QMouseEvent* me = static_cast<QMouseEvent*>(ev);
            if (me && me->button() == Qt::LeftButton && m_isSelecting) {
                finishSelection(me->pos().x(), me->pos().y());
                return true;
            }
            break;
        }
        default: break;
        }
        return QObject::eventFilter(obj, ev);
    }

private:

    void beginSelection (int x, int y, Qt::KeyboardModifiers mods) {
        m_isSelecting=true;
        m_startX=x; m_startY=y;
        m_shiftPressed=(mods & Qt::ShiftModifier);

        int h = m_widget->height();

        m_rubber->SetRectangle(m_startX,h-m_startY,m_startX,h-m_startY);
        m_context->Display(m_rubber,Standard_True);
        m_context->UpdateCurrentViewer();
    }

    void updateSelection (int x, int y) {
        if (!m_isSelecting) return;

        int xmin=std::min(m_startX, x);
        int ymin=std::min(m_startY, y);
        int xmax=std::max(m_startX, x);
        int ymax=std::max(m_startY, y);

        int h = m_widget->height();

        m_rubber->SetRectangle(xmin,h-ymin,xmax,h-ymax);
        m_context->Redisplay(m_rubber, Standard_True);  // mark for redraw
        m_context->UpdateCurrentViewer();               // force immediate update
    }

    void finishSelection (int x, int y) {
        if (!m_isSelecting) return;
        m_isSelecting = false;

        int xmin=std::min(m_startX, x);
        int ymin=std::min(m_startY, y);
        int xmax=std::max(m_startX, x);
        int ymax=std::max(m_startY, y);

        m_context->Erase(m_rubber, Standard_True);

        int h = m_widget->height();

        // Graphic3d_Vec2i points for OCCT rectangle selection
        Graphic3d_Vec2i pmin(xmin, ymin);
        Graphic3d_Vec2i pmax(xmax, ymax);


        AIS_SelectionScheme scheme = m_shiftPressed
                                         ? AIS_SelectionScheme_XOR
                                         : AIS_SelectionScheme_Replace;

        m_context->SelectRectangle(pmin, pmax, m_view, scheme);

        if (!m_view.IsNull()) m_view->Redraw();

        emit selectionFinished();
    }

private:
    Handle(AIS_InteractiveContext) m_context;
    Handle(V3d_View) m_view;
    QWidget* m_widget = nullptr;

    Handle(AIS_RubberBand) m_rubber;
    bool m_isSelecting{false};
    bool m_shiftPressed{false};
    int m_startX{0}, m_startY{0};
};


#endif // RECTANGLESELECTOR_H
