/////////////////////////////////////////////////////////////////////////////
// Name:            score.cpp
// Purpose:         Stores and renders a score
// Author:          Brad Larsen
// Modified by:
// Created:         Dec 16, 2004
// RCS-ID:
// Copyright:       (c) Brad Larsen
// License:         wxWindows license
/////////////////////////////////////////////////////////////////////////////

#include "score.h"

#include "staff.h"
#include "barline.h"
#include "powertabfileheader.h"
#include "system.h"
#include "guitar.h"
#include "chorddiagram.h"
#include "floatingtext.h"
#include "guitarin.h"
#include "tempomarker.h"
#include "alternateending.h"
#include "position.h"
#include "powertabinputstream.h"
#include "powertaboutputstream.h"
#include "direction.h"
#include "dynamic.h"
#include "systemlocation.h"
#include "common.h"
#include "tuning.h"

#include <map>
#include <bitset>
#include <algorithm>
#include <boost/foreach.hpp>
#include <boost/bind.hpp>

namespace PowerTabDocument {

/// Default Constructor
Score::Score(const char* name)
    : m_scoreName(name)
{
}

/// Copy Constructor
Score::Score(const Score& score) :
    PowerTabObject()
{
    *this = score;
}

// Operators
/// Assignment Operator
const Score& Score::operator=(const Score& score)
{
    // Check for assignment to self
    if (this != &score)
    {
        deepCopy(score.m_guitarArray, m_guitarArray);
        deepCopy(score.m_chordDiagramArray, m_chordDiagramArray);
        deepCopy(score.m_floatingTextArray, m_floatingTextArray);
        deepCopy(score.m_guitarInArray, m_guitarInArray);
        deepCopy(score.m_tempoMarkerArray, m_tempoMarkerArray);
        deepCopy(score.m_dynamicArray, m_dynamicArray);
        deepCopy(score.m_alternateEndingArray, m_alternateEndingArray);
        deepCopy(score.m_systemArray, m_systemArray);
        m_scoreName = score.m_scoreName;
    }
    return *this;
}

/// Equality Operator
bool Score::operator==(const Score& score) const
{
    return (isDeepEqual(m_guitarArray, score.m_guitarArray) &&
            isDeepEqual(m_chordDiagramArray, score.m_chordDiagramArray) &&
            isDeepEqual(m_floatingTextArray, score.m_floatingTextArray) &&
            isDeepEqual(m_guitarInArray, score.m_guitarInArray) &&
            isDeepEqual(m_tempoMarkerArray, score.m_tempoMarkerArray) &&
            isDeepEqual(m_dynamicArray, score.m_dynamicArray) &&
            isDeepEqual(m_alternateEndingArray, score.m_alternateEndingArray) &&
            isDeepEqual(m_systemArray, score.m_systemArray) &&
            m_scoreName == score.m_scoreName);
}

/// Inequality Operator
bool Score::operator!=(const Score& score) const
{
    return (!operator==(score));
}

// Serialization Functions
/// Performs serialization for the class
/// @param stream Power Tab output stream to serialize to
/// @return True if the object was serialized, false if not
bool Score::Serialize(PowerTabOutputStream& stream) const
{
    //------Last Checked------//
    // - Jan 5, 2005
    stream.WriteVector(m_guitarArray);
    PTB_CHECK_THAT(stream.CheckState(), false);

    stream.WriteVector(m_chordDiagramArray);
    PTB_CHECK_THAT(stream.CheckState(), false);

    stream.WriteVector(m_floatingTextArray);
    PTB_CHECK_THAT(stream.CheckState(), false);

    stream.WriteVector(m_guitarInArray);
    PTB_CHECK_THAT(stream.CheckState(), false);

    stream.WriteVector(m_tempoMarkerArray);
    PTB_CHECK_THAT(stream.CheckState(), false);

    stream.WriteVector(m_dynamicArray);
    PTB_CHECK_THAT(stream.CheckState(), false);

    stream.WriteVector(m_alternateEndingArray);
    PTB_CHECK_THAT(stream.CheckState(), false);

    stream.WriteVector(m_systemArray);
    PTB_CHECK_THAT(stream.CheckState(), false);

    return (stream.CheckState());
}

/// Performs deserialization for the class
/// @param stream Power Tab input stream to load from
/// @param version File version
/// @return True if the object was deserialized, false if not
bool Score::Deserialize(PowerTabInputStream& stream, uint16_t version)
{
    stream.ReadVector(m_guitarArray, version);
    stream.ReadVector(m_chordDiagramArray, version);
    stream.ReadVector(m_floatingTextArray, version);
    stream.ReadVector(m_guitarInArray, version);
    stream.ReadVector(m_tempoMarkerArray, version);
    stream.ReadVector(m_dynamicArray, version);
    stream.ReadVector(m_alternateEndingArray, version);
    stream.ReadVector(m_systemArray, version);

    return true;
}

namespace
{

template <class Symbol>
struct IsSymbolInSystem : std::unary_function<const Symbol&, bool>
{
    IsSymbolInSystem(uint32_t systemIndex) :
        systemIndex(systemIndex)
    {
    }

    bool operator()(const Symbol& symbol) const
    {
        return symbol->GetSystem() == systemIndex;
    }

    const uint32_t systemIndex;
};

/// Helper for GetAlternateEndingsInSystem, GetTempoMarkersInSystem, etc.
template <class Symbol>
void GetSymbolsInSystem(std::vector<Symbol>& output, const std::vector<Symbol>& symbolList, const uint32_t systemIndex)
{
    output.clear();

    // remove_copy_if did not compile on OSX, maybe I could have worked out what
    // was wrong but this is a lot easier to understand and works
    typename std::vector<Symbol>::const_iterator it;
    for (it = symbolList.begin() ; it != symbolList.end(); ++it)
    {
        if ((*it)->GetSystem() == systemIndex)
        {
            output.push_back(*it);
        }
    }
}

/// Removes the symbols in the specified system, and moves lower symbols up by
/// one system.
template <class Symbol>
void RemoveSymbolsInSystem(std::vector<Symbol>& symbolList, const uint32_t systemIndex)
{
    symbolList.erase(std::remove_if(symbolList.begin(), symbolList.end(),
                                    IsSymbolInSystem<Symbol>(systemIndex)),
                     symbolList.end());

    // Shift following symbols up by one system.
    BOOST_FOREACH(const Symbol& symbol, symbolList)
    {
        if (symbol->GetSystem() > systemIndex)
        {
            symbol->SetSystem(symbol->GetSystem() - 1);
        }
    }
}

/// Shifts symbols down by one system if they are either in or are below the
/// specified system.
template <class Symbol>
void ShiftFollowingSymbols(std::vector<Symbol>& symbolList, const uint32_t systemIndex)
{
    BOOST_FOREACH(const Symbol& symbol, symbolList)
    {
        if (symbol->GetSystem() >= systemIndex)
        {
            symbol->SetSystem(symbol->GetSystem() + 1);
        }
    }
}
}

// Finds the index of a system within the score
int Score::FindSystemIndex(const SystemConstPtr& system) const
{
    std::vector<SystemPtr>::const_iterator result = std::find(m_systemArray.begin(),
                                                                   m_systemArray.end(), system);
    return std::distance(m_systemArray.begin(), result);
}


/// Finds all of the tempo markers that are in the given system
void Score::GetTempoMarkersInSystem(std::vector<TempoMarkerPtr>& tempoMarkers, SystemConstPtr system) const
{
    GetSymbolsInSystem(tempoMarkers, m_tempoMarkerArray, FindSystemIndex(system));
}

void Score::GetAlternateEndingsInSystem(std::vector<AlternateEndingPtr>& endings, SystemConstPtr system) const
{
    GetSymbolsInSystem(endings, m_alternateEndingArray, FindSystemIndex(system));
}

/// Returns all of the dynamics located in the given system
void Score::GetDynamicsInSystem(std::vector<Score::DynamicPtr> &dynamics, Score::SystemConstPtr system) const
{
    GetSymbolsInSystem(dynamics, m_dynamicArray, FindSystemIndex(system));
}

/// Shifts all positions forward/backward starting from the given index.
void Score::PerformPositionShift(Score::SystemConstPtr system,
                                 uint32_t positionIndex, int offset)
{
    const boost::function<bool (uint32_t, uint32_t)> comparison =
            std::greater_equal<uint32_t>();

    // Shift tempo markers, dynamics, and alternate endings.
    // TODO - handle guitar in symbols?
    std::vector<TempoMarkerPtr> tempoMarkers;
    std::vector<DynamicPtr> dynamics;
    std::vector<AlternateEndingPtr> altEndings;

    GetTempoMarkersInSystem(tempoMarkers, system);
    GetDynamicsInSystem(dynamics, system);
    GetAlternateEndingsInSystem(altEndings, system);

    ShiftPosition<TempoMarkerPtr> shiftTempoMarkers(comparison, positionIndex,
                                                    offset);
    ShiftPosition<DynamicPtr> shiftDynamics(comparison, positionIndex, offset);
    ShiftPosition<AlternateEndingPtr> shiftAltEndings(comparison, positionIndex,
                                                      offset);

    std::for_each(tempoMarkers.begin(), tempoMarkers.end(), shiftTempoMarkers);
    std::for_each(dynamics.begin(), dynamics.end(), shiftDynamics);
    std::for_each(altEndings.begin(), altEndings.end(), shiftAltEndings);
}

/// Shift all positions forward in a system, starting from a given location.
void Score::ShiftForward(Score::SystemPtr system, uint32_t positionIndex)
{
    PerformPositionShift(system, positionIndex, 1);
    system->ShiftForward(positionIndex);
}

/// Shift all positions backwards in a system, starting from a given location.
void Score::ShiftBackward(Score::SystemPtr system, uint32_t positionIndex)
{
    PerformPositionShift(system, positionIndex, -1);
    system->ShiftBackward(positionIndex);
}

/// Determines if a alternate ending index is valid
/// @param index alternate ending index to validate
/// @return True if the alternate ending index is valid, false if not
bool Score::IsValidAlternateEndingIndex(uint32_t index) const
{
    return index < GetAlternateEndingCount();
}

/// Gets the number of alternate endings in the score
/// @return The number of alternate endings in the score
size_t Score::GetAlternateEndingCount() const
{
    return m_alternateEndingArray.size();
}

/// Gets the nth alternate ending in the score
/// @param index Index of the alternate ending to get
/// @return The nth alternate ending in the score
Score::AlternateEndingPtr Score::GetAlternateEnding(uint32_t index) const
{
    PTB_CHECK_THAT(IsValidAlternateEndingIndex(index), AlternateEndingPtr());
    return m_alternateEndingArray[index];
}

/// Determines if a chord diagram index is valid
/// @param index ChordDiagram index to validate
/// @return True if the chord diagram index is valid, false if not
bool Score::IsValidChordDiagramIndex(uint32_t index) const
{
    return index < GetChordDiagramCount();
}

/// Gets the number of chord diagrams in the score
/// @return The number of chord diagrams in the score
size_t Score::GetChordDiagramCount() const
{
    return m_chordDiagramArray.size();
}

/// Gets the nth chord diagram in the score
/// @param index Index of the chord diagram to get
/// @return The nth chord diagram in the score
Score::ChordDiagramPtr Score::GetChordDiagram(uint32_t index) const
{
    PTB_CHECK_THAT(IsValidChordDiagramIndex(index), ChordDiagramPtr());
    return m_chordDiagramArray[index];
}

// Guitar Functions
/// Determines if a guitar index is valid
/// @param index guitar index to validate
/// @return True if the guitar index is valid, false if not
bool Score::IsValidGuitarIndex(size_t index) const
{
    return index < GetGuitarCount();
}

/// Gets the number of guitars in the score
/// @return The number of guitars in the score
size_t Score::GetGuitarCount() const
{
    return m_guitarArray.size();
}

/// Gets the nth guitar in the score
/// @param index Index of the guitar to get
/// @return The nth guitar in the score
Score::GuitarPtr Score::GetGuitar(size_t index) const
{
    PTB_CHECK_THAT(IsValidGuitarIndex(index), GuitarPtr());
    return m_guitarArray[index];
}

// System Functions
/// Determines if a system index is valid
/// @param index system index to validate
/// @return True if the system index is valid, false if not
bool Score::IsValidSystemIndex(uint32_t index) const
{
    return index < GetSystemCount();
}

/// Gets the number of systems in the score
/// @return The number of systems in the score
size_t Score::GetSystemCount() const
{
    return m_systemArray.size();
}

/// Gets the nth system in the score
/// @param index Index of the system to get
/// @return The nth system in the score
Score::SystemPtr Score::GetSystem(uint32_t index) const
{
    PTB_CHECK_THAT(IsValidSystemIndex(index), SystemPtr());
    return m_systemArray[index];
}

// Dynamic Functions
/// Determines if a dynamic index is valid
/// @param index dynamic index to validate
/// @return True if the dynamic index is valid, false if not
bool Score::IsValidDynamicIndex(uint32_t index) const
{
    return index < GetDynamicCount();
}

/// Gets the number of dynamics in the score
/// @return The number of dynamics in the score
size_t Score::GetDynamicCount() const
{
    return m_dynamicArray.size();
}

/// Gets the nth dynamic in the score
/// @param index Index of the dynamic to get
/// @return The nth dynamic in the score
Score::DynamicPtr Score::GetDynamic(uint32_t index) const
{
    PTB_CHECK_THAT(IsValidDynamicIndex(index), DynamicPtr());
    return m_dynamicArray[index];
}

// Floating Text Functions
/// Determines if a floating text index is valid
/// @param index floating text index to validate
/// @return True if the floating text index is valid, false if not
bool Score::IsValidFloatingTextIndex(uint32_t index) const
{
    return index < GetFloatingTextCount();
}

/// Gets the number of floating text items in the score
/// @return The number of floating text items in the score
size_t Score::GetFloatingTextCount() const
{
    return m_floatingTextArray.size();
}

/// Gets the nth floating text in the score
/// @param index Index of the floating text to get
/// @return The nth floating text in the score
Score::FloatingTextPtr Score::GetFloatingText(uint32_t index) const
{
    PTB_CHECK_THAT(IsValidFloatingTextIndex(index), FloatingTextPtr());
    return m_floatingTextArray[index];
}

// Guitar In Functions
/// Determines if a guitar in index is valid
/// @param index guitar in index to validate
/// @return True if the guitar in index is valid, false if not
bool Score::IsValidGuitarInIndex(uint32_t index) const
{
    return index < GetGuitarInCount();
}

/// Gets the number of guitar ins in the score
/// @return The number of guitar ins in the score
size_t Score::GetGuitarInCount() const
{
    return m_guitarInArray.size();
}

/// Gets the nth guitar in in the score
/// @param index Index of the guitar in to get
/// @return The nth guitar in in the score
Score::GuitarInPtr Score::GetGuitarIn(uint32_t index) const
{
    PTB_CHECK_THAT(IsValidGuitarInIndex(index), GuitarInPtr());
    return m_guitarInArray[index];
}

void Score::GetGuitarInsInSystem(std::vector<Score::GuitarInPtr> &guitarIns,
                                 Score::SystemConstPtr system) const
{
    GetSymbolsInSystem(guitarIns, m_guitarInArray, FindSystemIndex(system));
}

// Tempo Marker Functions
/// Determines if a tempo marker index is valid
/// @param index tempo marker index to validate
/// @return True if the tempo marker index is valid, false if not
bool Score::IsValidTempoMarkerIndex(uint32_t index) const
{
    return index < GetTempoMarkerCount();
}

/// Gets the number of tempo markers in the score
/// @return The number of tempo markers in the score
size_t Score::GetTempoMarkerCount() const
{
    return m_tempoMarkerArray.size();
}

/// Gets the nth tempo marker in the score
/// @param index Index of the tempo marker to get
/// @return The nth tempo marker in the score
Score::TempoMarkerPtr Score::GetTempoMarker(uint32_t index) const
{
    PTB_CHECK_THAT(IsValidTempoMarkerIndex(index), TempoMarkerPtr());
    return m_tempoMarkerArray[index];
}

/// @return The name of the score
std::string Score::GetScoreName() const
{
    return m_scoreName;
}

}
