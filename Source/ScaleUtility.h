#pragma once

#include <JuceHeader.h>
#include "ParameterManager.h"

/**
 * Utility class for working with musical scales and pitch mapping
 */
class ScaleUtility
{
public:
    /**
     * Get the pitch shift in semitones for a given position in a scale
     * 
     * @param scale The musical scale to use
     * @param position The position within the scale (0-15 for Game of Life grid)
     * @param range The range of notes to map to (e.g., 16 for a full Game of Life grid)
     * @return The pitch shift in semitones
     */
    static int getPitchShiftForPosition(MusicalScale scale, int position, int range)
    {
        // Map position to a scale degree
        int octave = position / 7; // Rough approximation for most scales
        int degree = position % 7;
        
        // For scales with fewer than 7 notes, adjust the calculation
        if (scale == MusicalScale::Pentatonic)
        {
            octave = position / 5;
            degree = position % 5;
        }
        else if (scale == MusicalScale::Blues)
        {
            octave = position / 6;
            degree = position % 6;
        }
        else if (scale == MusicalScale::Chromatic)
        {
            // Chromatic scale is just semitones
            return position - (range / 2);
        }
        
        // Get the scale pattern
        const int* scalePattern = getScalePattern(scale);
        int numNotesInScale = getNumNotesInScale(scale);
        
        // Calculate the pitch shift
        int pitchShift = 0;
        
        // Add octave shift
        pitchShift += octave * 12;
        
        // Add scale degree shift
        for (int i = 0; i < degree && i < numNotesInScale; ++i)
        {
            pitchShift += scalePattern[i];
        }
        
        // Center the pitch range around the middle
        pitchShift -= (range / 2);
        
        return pitchShift;
    }
    
private:
    /**
     * Get the number of notes in a scale
     */
    static int getNumNotesInScale(MusicalScale scale)
    {
        switch (scale)
        {
            case MusicalScale::Major:
            case MusicalScale::NaturalMinor:
            case MusicalScale::HarmonicMinor:
                return 7;
            case MusicalScale::Pentatonic:
                return 5;
            case MusicalScale::Blues:
                return 6;
            case MusicalScale::Chromatic:
                return 12;
            default:
                return 7;
        }
    }
    
    /**
     * Get the pattern of semitone intervals for a scale
     * Each value represents the number of semitones to the next note
     */
    static const int* getScalePattern(MusicalScale scale)
    {
        // Major scale: W-W-H-W-W-W-H (where W=whole step, H=half step)
        static const int majorPattern[] = { 2, 2, 1, 2, 2, 2, 1 };
        
        // Natural minor scale: W-H-W-W-H-W-W
        static const int naturalMinorPattern[] = { 2, 1, 2, 2, 1, 2, 2 };
        
        // Harmonic minor scale: W-H-W-W-H-WH-H (WH = whole + half step)
        static const int harmonicMinorPattern[] = { 2, 1, 2, 2, 1, 3, 1 };
        
        // Pentatonic scale: W-W-W+H-W-W+H
        static const int pentatonicPattern[] = { 2, 2, 3, 2, 3 };
        
        // Blues scale: W+H-W-H-H-W+H-W
        static const int bluesPattern[] = { 3, 2, 1, 1, 3, 2 };
        
        // Chromatic scale: all half steps
        static const int chromaticPattern[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
        
        switch (scale)
        {
            case MusicalScale::Major:
                return majorPattern;
            case MusicalScale::NaturalMinor:
                return naturalMinorPattern;
            case MusicalScale::HarmonicMinor:
                return harmonicMinorPattern;
            case MusicalScale::Pentatonic:
                return pentatonicPattern;
            case MusicalScale::Blues:
                return bluesPattern;
            case MusicalScale::Chromatic:
                return chromaticPattern;
            default:
                return majorPattern;
        }
    }
};
