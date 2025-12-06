#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""
Test survey parser functionality
"""

from gb_client.protocol import SurveyParser
from gb_client.models import SurveyData, SurveySector

def test_survey_intro():
    """Test parsing survey intro message"""
    args = ['10', '10', 'Sol', 'Earth', '1000', '500', '0', '5000', '10000', '10', '75.5', '0']
    survey = SurveyParser.parse_survey_intro(args)
    
    assert survey is not None, "Survey should be parsed"
    assert survey.maxx == 10, "Width should be 10"
    assert survey.maxy == 10, "Height should be 10"
    assert survey.star == 'Sol', "Star should be Sol"
    assert survey.planet == 'Earth', "Planet should be Earth"
    assert survey.res == 1000, "Resources should be 1000"
    assert survey.fuel == 500, "Fuel should be 500"
    assert survey.popn == 5000, "Population should be 5000"
    assert survey.compat == 75.5, "Compatibility should be 75.5"
    print("✓ Survey intro parsing works!")

def test_survey_sector():
    """Test parsing survey sector message"""
    args = ['5', '3', '-', '.', '0', '1', '100', '50', '127', '0', '25', '100', '50', '150']
    sector = SurveyParser.parse_survey_sector(args)
    
    assert sector is not None, "Sector should be parsed"
    assert sector.x == 5, "X should be 5"
    assert sector.y == 3, "Y should be 3"
    assert sector.sect_char == '-', "Terrain should be land"
    assert sector.owner == 1, "Owner should be 1"
    assert sector.eff == 100, "Efficiency should be 100"
    assert sector.frt == 50, "Fertility should be 50"
    assert sector.civ == 100, "Civilians should be 100"
    assert sector.mil == 50, "Military should be 50"
    print("✓ Survey sector parsing works!")

def test_survey_sector_with_ships():
    """Test parsing survey sector with landed ships"""
    args = ['2', '1', '^', '.', '0', '1', '100', '30', '100', '0', '50', '0', '25', '100', ';', '123', 'D', '1', ';', '456', 'c', '1', ';']
    sector = SurveyParser.parse_survey_sector(args)
    
    assert sector is not None, "Sector should be parsed"
    assert sector.x == 2, "X should be 2"
    assert len(sector.ships) == 2, "Should have 2 ships"
    assert sector.ships[0].shipno == 123, "First ship number should be 123"
    assert sector.ships[0].letter == 'D', "First ship letter should be D"
    assert sector.ships[1].shipno == 456, "Second ship number should be 456"
    assert sector.ships[1].letter == 'c', "Second ship letter should be c"
    print("✓ Survey sector with ships parsing works!")

def test_survey_display():
    """Test survey display formatting"""
    survey = SurveyData(
        maxx=5,
        maxy=5,
        star='TestStar',
        planet='TestPlanet',
        res=500,
        fuel=300,
        des=0,
        popn=1000,
        mpopn=5000,
        tox=5,
        compat=85.5,
        enslaved=0
    )
    
    sector = SurveySector(
        x=0,
        y=0,
        sect_char='-',
        des='.',
        wasted=False,
        owner=1,
        eff=100,
        frt=50,
        mob=127,
        xtal=True,
        res=25,
        civ=100,
        mil=50,
        mpopn=150
    )
    survey.sectors.append(sector)
    
    display = SurveyParser.format_survey_display(survey)
    assert 'TestStar/TestPlanet' in display, "Display should include planet name"
    assert '85.5' in display or '85.50' in display, "Display should include compatibility"
    assert 'yes' in display, "Display should show crystal"
    print("✓ Survey display formatting works!")
    print("\nSample output:")
    print(display)

if __name__ == '__main__':
    print("Testing Survey Parser...")
    print()
    test_survey_intro()
    test_survey_sector()
    test_survey_sector_with_ships()
    test_survey_display()
    print()
    print("All tests passed! ✓")
