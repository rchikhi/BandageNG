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


#pragma once

#include "small_vector/small_pod_vector.hpp"

#include <QPointF>
#include <QColor>
#include <QFont>
#include <QString>
#include <QPainterPath>
#include <QStringList>
#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>

#include <vector>

class DeBruijnNode;
class Path;

class GraphicsItemNode : public QGraphicsItem
{
public:
    GraphicsItemNode(DeBruijnNode * deBruijnNode,
                     GraphicsItemNode * toCopy,
                     QGraphicsItem * parent = nullptr);
    GraphicsItemNode(DeBruijnNode * deBruijnNode,
                     const std::vector<QPointF> &linePoints,
                     QGraphicsItem * parent = nullptr);
    GraphicsItemNode(DeBruijnNode * deBruijnNode,
                     const adt::SmallPODVector<QPointF> &linePoints,
                     QGraphicsItem * parent = nullptr);

    DeBruijnNode * m_deBruijnNode;
    adt::SmallPODVector<QPointF> m_linePoints;
    QColor m_colour;
    QPainterPath m_path;
    float m_width;
    size_t m_grabIndex : 31;
    bool m_hasArrow : 1;

    static QSize getNodeTextSize(const QString& text);
    static double getNodeWidth(double depthRelativeToMeanDrawnDepth,
                               double depthPower,
                               double depthEffectOnWidth,
                               double averageNodeWidth);
    static void drawTextPathAtLocation(QPainter *painter, const QPainterPath& textPath, QPointF centre);

    void mousePressEvent(QGraphicsSceneMouseEvent * event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent * event) override;
    void paint(QPainter * painter, const QStyleOptionGraphicsItem *, QWidget *) override;
    QPainterPath shape() const override;
    void shiftPoints(QPointF difference);
    void remakePath();
    bool usePositiveNodeColour() const;
    QPointF getFirst() const {return m_linePoints[0];}
    QPointF getSecond() const {return m_linePoints[1];}
    QPointF getLast() const {return m_linePoints[m_linePoints.size()-1];}
    QPointF getSecondLast() const {return m_linePoints[m_linePoints.size()-2];}
    std::vector<QPointF> getCentres() const;
    void setNodeColour(QColor color) { m_colour = color; }
    QStringList getNodeText() const;
    void setWidth();
    QPainterPath makePartialPath(double startFraction, double endFraction);
    double getNodePathLength();
    QPointF findLocationOnPath(double fraction);
    QRectF boundingRect() const override;
    void shiftPointsLeft();
    void shiftPointsRight();
    void fixEdgePaths(std::vector<GraphicsItemNode *> * nodes = nullptr) const;
    double indexToFraction(int64_t pos) const;

private:
    static void pathHighlightNode3(QPainter * painter, QPainterPath highlightPath);
    static bool anyNodeDisplayText();

    void exactPathHighlightNode(QPainter * painter);
    void queryPathHighlightNode(QPainter * painter);
    void pathHighlightNode2(QPainter * painter, DeBruijnNode * node, bool reverse, Path * path);
    QPainterPath buildPartialHighlightPath(double startFraction, double endFraction, bool reverse);
    void shiftPointSideways(bool left);
};