//Copyright 2017 Ryan Wick

//This file is part of Bandage

//Bandage is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//Bandage is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

//You should have received a copy of the GNU General Public License
//along with Bandage.  If not, see <http://www.gnu.org/licenses/>.


#ifndef GRAPHICSITEMEDGE_H
#define GRAPHICSITEMEDGE_H

#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QPointF>

class DeBruijnEdge;

class GraphicsItemEdge : public QGraphicsPathItem
{
public:
    explicit GraphicsItemEdge(DeBruijnEdge * deBruijnEdge, QGraphicsItem * parent = nullptr);

    DeBruijnEdge * m_deBruijnEdge;
    QPointF m_startingLocation;
    QPointF m_endingLocation;
    QPointF m_beforeStartingLocation;
    QPointF m_afterEndingLocation;
    QPointF m_controlPoint1;
    QPointF m_controlPoint2;
    QColor m_edgeColor;
    Qt::PenStyle m_penStyle;

    void paint(QPainter * painter, const QStyleOptionGraphicsItem *, QWidget *) override;
    QPainterPath shape() const override;
    void calculateAndSetPath();
    void setControlPointLocations();
    void setStartingPoints(QPointF startingLocation, QPointF beforeStartingLocation) {m_startingLocation = startingLocation; m_beforeStartingLocation = beforeStartingLocation;}
    void setEndingPoints(QPointF endingLocation, QPointF afterEndingLocation) {m_endingLocation = endingLocation; m_afterEndingLocation = afterEndingLocation;}
    void makeSpecialPathConnectingNodeToSelf();
    void makeSpecialPathConnectingNodeToReverseComplement();
};

#endif // GRAPHICSITEMEDGE_H
