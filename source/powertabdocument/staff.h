/////////////////////////////////////////////////////////////////////////////
// Name:            staff.h
// Purpose:         Stores and renders a staff
// Author:          Brad Larsen
// Modified by:
// Created:         Dec 16, 2004
// RCS-ID:
// Copyright:       (c) Brad Larsen
// License:         wxWindows license
/////////////////////////////////////////////////////////////////////////////

#ifndef __STAFF_H__
#define __STAFF_H__

#include "position.h"

#define NUM_STAFF_VOICES 2    ///< Number of voices in a staff

/// Stores and renders a staff
class Staff : public PowerTabObject
{
    // Constants
public:
    // Default Constants
    static const uint8_t DEFAULT_DATA;                                   ///< Default value for the data member variable
    static const uint8_t DEFAULT_CLEF;                                   ///< Default clef type
    static const uint8_t DEFAULT_TABLATURE_STAFF_TYPE;                   ///< Default tablature staff type
    static const uint8_t DEFAULT_STANDARD_NOTATION_STAFF_ABOVE_SPACING;  ///< Default value for the standard notation staff above spacing member variable
    static const uint8_t DEFAULT_STANDARD_NOTATION_STAFF_BELOW_SPACING;  ///< Default value for the standard notation staff below spacing member variable
    static const uint8_t DEFAULT_SYMBOL_SPACING;                         ///< Default value for the symbol spacing member variable
    static const uint8_t DEFAULT_TABLATURE_STAFF_BELOW_SPACING;          ///< Default value for the tablature staff below spacing member variable
    static const uint8_t STD_NOTATION_LINE_SPACING;
    static const uint8_t STD_NOTATION_STAFF_TYPE;
    static const uint8_t STAFF_BORDER_SPACING; // padding around top and bottom of staves

    // Clef Constants
    static const uint8_t TREBLE_CLEF;                                ///< Treble clef
    static const uint8_t BASS_CLEF;                                  ///< Bass clef

    // Tablature Staff Type Constants
    static const uint8_t MIN_TABLATURE_STAFF_TYPE;                   ///< Minimum allowed value for tablature staff type
    static const uint8_t MAX_TABLATURE_STAFF_TYPE;                   ///< Maximum allowed value for tablature staff type

    enum flags
    {
        clefMask                    = (uint8_t)0xf0,                 ///< Mask used to retrieve the clef type
        tablatureStaffTypeMask      = (uint8_t)0xf                   ///< Mask used to retrieve the tablature type (3 - 7 string)
    };

    // Member Variables
protected:
    uint8_t m_data;											        ///< Top 4 bits = clef type, bottom 4 bits = tablature type
    uint8_t m_standardNotationStaffAboveSpacing;                     ///< Amount of space alloted from the top line of the standard notation staff
    uint8_t m_standardNotationStaffBelowSpacing;                     ///< Amount of space alloted from the last line of the standard notation staff
    uint8_t m_symbolSpacing;                                         ///< Amount of space alloted for symbols located between the standard notation and tablature staff
    uint8_t m_tablatureStaffBelowSpacing;                            ///< Amount of space alloted from the last line of the tablature staff

public:
    vector<Position*> highMelodyPositionArray;
    vector<Position*> lowMelodyPositionArray;

    // Constructor/Destructor
public:
    Staff();
    Staff(uint8_t tablatureStaffType, uint8_t clef);
    Staff(const Staff& staff);
    ~Staff();

    // Operators
    const Staff& operator=(const Staff& staff);
    bool operator==(const Staff& staff) const;
    bool operator!=(const Staff& staff) const;

    // Serialize Functions
    bool Serialize(PowerTabOutputStream& stream);
    bool Deserialize(PowerTabInputStream& stream, uint16_t version);

    Staff* CloneObject() const;

    // MFC Class Functions
public:
    /// Gets the MFC Class Name for the object
    /// @return The MFC Class Name
    string GetMFCClassName() const
    {return "CStaff";}
    /// Gets the MFC Class Schema for the object
    /// @return The MFC Class Schema
    uint16_t GetMFCClassSchema() const
    {return ((uint16_t)1);}

    // Clef Functions
    /// Determines if a Clef is valid
    /// @param clef Clef to validate
    /// @return True if the clef is valid, false if not
    static bool IsValidClef(uint8_t clef)
    {return ((clef >= TREBLE_CLEF) && (clef <= BASS_CLEF));}
    bool SetClef(uint8_t type);
    /// Gets the clef used on the standard notation staff
    /// @return The clef used on the standard notation staff
    uint8_t GetClef() const
    {return ((uint8_t)((m_data & clefMask) >> 4));}

    // Tablature Staff Type Functions
    /// Determines if a Tablature Staff Type is valid
    /// @param type Tablature staff type to validate
    /// @return True if the tablature staff type is valid, false if not
    static bool IsValidTablatureStaffType(uint8_t type)
    {
        return ((type >= MIN_TABLATURE_STAFF_TYPE) &&
                ((type <= MAX_TABLATURE_STAFF_TYPE)));
    }
    bool SetTablatureStaffType(uint8_t type);
    /// Gets the tablature staff type (3-7 strings)
    /// @return The tablature staff type
    uint8_t GetTablatureStaffType() const
    {return ((uint8_t)(m_data & tablatureStaffTypeMask));}

    // Standard Notation Staff Above Spacing Functions
    /// Sets the amount of spacing above the standard notation staff
    /// @param spacing Spacing to set
    void SetStandardNotationStaffAboveSpacing(uint8_t spacing)
    {m_standardNotationStaffAboveSpacing = spacing;}
    /// Gets the amount of spacing above the standard notation staff
    /// @return The amount of spacing above the standard notation staff
    uint8_t GetStandardNotationStaffAboveSpacing() const
    {return (m_standardNotationStaffAboveSpacing);}

    // Standard Notation Staff Below Spacing Functions
    /// Sets the amount of spacing below the standard notation staff
    /// @param spacing Spacing to set
    void SetStandardNotationStaffBelowSpacing(uint8_t spacing)
    {m_standardNotationStaffBelowSpacing = spacing;}
    /// Gets the amount of spacing below the standard notation staff
    /// @return The amount of spacing below the standard notation staff
    uint8_t GetStandardNotationStaffBelowSpacing() const
    {return (m_standardNotationStaffBelowSpacing);}

    // Symbol Spacing Functions
    /// Sets the amount of spacing used by symbols in between the standard
    /// notation and tablature staves
    /// @param spacing Spacing to set
    void SetSymbolSpacing(uint8_t spacing)
    {m_symbolSpacing = spacing;}
    /// Gets the amount of spacing used by symbols in between the standard
    /// notation and tablature staves
    /// @return The amount of spacing used by symbols in between the standard
    /// notation and tablature staves
    uint8_t GetSymbolSpacing() const
    {return (m_symbolSpacing);}

    // Tablature Staff Below Spacing Functions
    /// Sets the amount of spacing below the tablature staff
    /// @param spacing Spacing to set
    void SetTablatureStaffBelowSpacing(uint8_t spacing)
    {m_tablatureStaffBelowSpacing = spacing;}
    /// Gets the amount of spacing below the tablature staff
    /// @return The amount of spacing below the tablature staff
    uint8_t GetTablatureStaffBelowSpacing() const
    {return (m_tablatureStaffBelowSpacing);}

    // Voice Functions
    /// Determines if a voice is valid
    /// @param voice Voice to validate
    /// @return True if the voice is valid, false if not
    static bool IsValidVoice(uint32_t voice)
    {return (voice < NUM_STAFF_VOICES);}

    // Position Functions
    /// Determines if a position index is valid
    /// @param voice Voice the position belongs to
    /// @param index position index to validate
    /// @return True if the position index is valid, false if not
    bool IsValidPositionIndex(uint32_t voice, uint32_t index) const
    {
        CHECK_THAT(IsValidVoice(voice), false);
        return (index < GetPositionCount(voice));
    }
    /// Gets the number of positions in the staff
    /// @param voice Voice of the positions to get the count of
    /// @return The number of positions in the staff
    size_t GetPositionCount(uint32_t voice) const
    {
        CHECK_THAT(IsValidVoice(voice), 0);
        if (voice == 0)
        {
            return highMelodyPositionArray.size();
        }
        else
        {
            return lowMelodyPositionArray.size();
        }
    }
    /// Gets the nth position in the staff
    /// @param voice Voice the position belongs to
    /// @param index Index of the position to get
    /// @return The nth position in the staff
    Position* GetPosition(uint32_t voice, uint32_t index) const
    {
        CHECK_THAT(IsValidPositionIndex(voice, index), NULL);
        if (voice == 0)
        {
            return highMelodyPositionArray[index];
        }
        else
        {
            return lowMelodyPositionArray[index];
        }
    }

    Position* GetPositionByPosition(uint32_t index) const;

    int GetHeight() const;
};

#endif
