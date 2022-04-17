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


#ifndef DEBRUIJNNODE_H
#define DEBRUIJNNODE_H

#include "program/globals.h"
#include "program/settings.h"
#include "blast/blasthitpart.h"

#include "ogdf/basic/Graph.h"
#include "ogdf/basic/GraphAttributes.h"

#include "llvm/ADT/iterator_range.h"
#include "seq/sequence.hpp"
#include "small_vector/small_pod_vector.hpp"

#include <QColor>
#include <QByteArray>
#include <vector>

class OgdfNode;
class DeBruijnEdge;
class GraphicsItemNode;
class BlastHit;

class DeBruijnNode
{
public:
    //CREATORS
    DeBruijnNode(QString name, double depth, const Sequence &sequence, int length = 0);
    ~DeBruijnNode();

    //ACCESSORS
    QString getName() const {return m_name;}
    QString getNameWithoutSign() const {return m_name.left(m_name.length() - 1);}
    QString getSign() const {if (m_name.length() > 0) return m_name.right(1); else return "+";}

    double getDepth() const {return m_depth;}
    double getDepthRelativeToMeanDrawnDepth() const {return m_depthRelativeToMeanDrawnDepth;}

    const Sequence &getSequence() const;
    Sequence &getSequence();

    int getLength() const {return m_length;}
    QByteArray getSequenceForGfa() const;
    int getFullLength() const;
    int getLengthWithoutTrailingOverlap() const;
    QByteArray getFasta(bool sign, bool newLines = true, bool evenIfEmpty = true) const;
    QByteArray getGfaSegmentLine(const QString& depthTag) const;
    char getBaseAt(int i) const {if (i >= 0 && i < m_sequence.size()) return m_sequence[i]; else return '\0';} // NOTE
    ContiguityStatus getContiguityStatus() const {return m_contiguityStatus;}
    DeBruijnNode * getReverseComplement() const {return m_reverseComplement;}
    OgdfNode * getOgdfNode() const {return m_ogdfNode;}
    GraphicsItemNode * getGraphicsItemNode() const {return m_graphicsItemNode;}
    bool thisOrReverseComplementHasGraphicsItemNode() const {return (m_graphicsItemNode != 0 || getReverseComplement()->m_graphicsItemNode != 0);}
    bool hasGraphicsItem() const {return m_graphicsItemNode != 0;}

    auto edgeBegin() { return m_edges.begin(); }
    const auto edgeBegin() const { return m_edges.begin(); }
    auto edgeEnd() { return m_edges.end(); }
    const auto edgeEnd() const { return m_edges.end(); }
    auto edges() { return llvm::make_range(edgeBegin(), edgeEnd()); }
    const auto edges() const { return llvm::make_range(edgeBegin(), edgeEnd()); }
    
    std::vector<DeBruijnEdge *> getEnteringEdges() const;
    std::vector<DeBruijnEdge *> getLeavingEdges() const;
    std::vector<DeBruijnNode *> getDownstreamNodes() const;
    std::vector<DeBruijnNode *> getUpstreamNodes() const;
    std::vector<DeBruijnNode *> getAllConnectedPositiveNodes() const;
    bool isSpecialNode() const {return m_specialNode;}
    bool isDrawn() const {return m_drawn;}
    bool thisNodeOrReverseComplementIsDrawn() const {return isDrawn() || getReverseComplement()->isDrawn();}
    bool isNotDrawn() const {return !m_drawn;}
    QColor getCustomColour() const {return m_customColour;}
    QColor getCustomColourForDisplay() const;
    QString getCustomLabel() const {return m_customLabel;}
    QStringList getCustomLabelForDisplay() const;
    bool hasCustomColour() const {return m_customColour.isValid();}
    bool isPositiveNode() const;
    bool isNegativeNode() const;
    bool inOgdf() const {return m_ogdfNode != 0;}
    bool notInOgdf() const {return m_ogdfNode == 0;}
    bool thisOrReverseComplementInOgdf() const {return (inOgdf() || getReverseComplement()->inOgdf());}
    bool thisOrReverseComplementNotInOgdf() const {return !thisOrReverseComplementInOgdf();}
    bool isNodeConnected(DeBruijnNode * node) const;
    DeBruijnEdge * doesNodeLeadIn(DeBruijnNode * node) const;
    DeBruijnEdge * doesNodeLeadAway(DeBruijnNode * node) const;
    std::vector<BlastHitPart> getBlastHitPartsForThisNode(double scaledNodeLength) const;
    std::vector<BlastHitPart> getBlastHitPartsForThisNodeOrReverseComplement(double scaledNodeLength) const;
    bool hasCsvData() const {return !m_csvData.isEmpty();}
    QStringList getAllCsvData() const {return m_csvData;}
    QString getCsvLine(int i) const {if (i < m_csvData.length()) return m_csvData[i]; else return "";}
    bool isInDepthRange(double min, double max) const;
    bool sequenceIsMissing() const;
    DeBruijnEdge *getSelfLoopingEdge() const;
    int getDeadEndCount() const;
    static int getNumberOfOgdfGraphEdges(double drawnNodeLength) ;
    double getDrawnNodeLength() const;

    //MODIFERS
    void setDepthRelativeToMeanDrawnDepth(double newVal) {m_depthRelativeToMeanDrawnDepth = newVal;}
    void setSequence(const QByteArray &newSeq) {m_sequence = Sequence(newSeq); m_length = m_sequence.size();}
    void setSequence(const Sequence &newSeq) {m_sequence = newSeq; m_length = m_sequence.size();}
    void upgradeContiguityStatus(ContiguityStatus newStatus);
    void resetContiguityStatus() {m_contiguityStatus = NOT_CONTIGUOUS;}
    void setReverseComplement(DeBruijnNode * rc) {m_reverseComplement = rc;}
    void setGraphicsItemNode(GraphicsItemNode * gin) {m_graphicsItemNode = gin;}
    void setAsSpecial() {m_specialNode = true;}
    void setAsNotSpecial() {m_specialNode = false;}
    void setAsDrawn() {m_drawn = true;}
    void setAsNotDrawn() {m_drawn = false;}
    void setCustomColour(QColor newColour) {m_customColour = newColour;}
    void setCustomLabel(QString newLabel);
    void resetNode();
    void addEdge(DeBruijnEdge * edge);
    void removeEdge(DeBruijnEdge * edge);
    void addToOgdfGraph(ogdf::Graph * ogdfGraph, ogdf::GraphAttributes * graphAttributes,
                        ogdf::EdgeArray<double> * edgeArray, double xPos, double yPos);
    void determineContiguity();
    void labelNeighbouringNodesAsDrawn(int nodeDistance, DeBruijnNode * callingNode);
    void setCsvData(QStringList csvData) {m_csvData = std::move(csvData);}
    void clearCsvData() {m_csvData.clear();}
    void setDepth(double newDepth) {m_depth = newDepth;}
    void setName(QString newName) {m_name = std::move(newName);}

private:
    QString m_name;
    double m_depth;
    double m_depthRelativeToMeanDrawnDepth;
    Sequence m_sequence;
    DeBruijnNode * m_reverseComplement;
    adt::SmallPODVector<DeBruijnEdge *> m_edges;

    OgdfNode * m_ogdfNode;
    GraphicsItemNode * m_graphicsItemNode;

    int m_length;
    int m_highestDistanceInNeighbourSearch : 27;
    ContiguityStatus m_contiguityStatus : 3;
    bool m_specialNode : 1;
    bool m_drawn : 1;

    QColor m_customColour;
    QString m_customLabel;
    QStringList m_csvData;

    QString getNodeNameForFasta(bool sign) const;
    QByteArray getUpstreamSequence(int upstreamSequenceLength) const;

    static double getNodeLengthPerMegabase() ;
    static bool isOnlyPathInItsDirection(DeBruijnNode * connectedNode,
                                  std::vector<DeBruijnNode *> * incomingNodes,
                                  std::vector<DeBruijnNode *> * outgoingNodes) ;
    static bool isNotOnlyPathInItsDirection(DeBruijnNode * connectedNode,
                                     std::vector<DeBruijnNode *> * incomingNodes,
                                     std::vector<DeBruijnNode *> * outgoingNodes) ;
    static std::vector<DeBruijnNode *> getNodesCommonToAllPaths(std::vector< std::vector <DeBruijnNode *> > * paths,
                                                         bool includeReverseComplements) ;
    bool doesPathLeadOnlyToNode(DeBruijnNode * node, bool includeReverseComplement);
};

#endif // DEBRUIJNNODE_H
