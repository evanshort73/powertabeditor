#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

#include "serialization_test.h"
#include <powertabdocument/note.h>

BOOST_AUTO_TEST_SUITE(NoteTest)

    BOOST_AUTO_TEST_CASE(ConstructionAndAssignment)
    {
        Note note1(4, 5);
        note1.SetTrill(7); // add some complex symbols
        note1.SetSlideInto(Note::slideIntoFromBelow);

        Note note2;

        BOOST_CHECK(note1 != note2);

        note2 = note1;

        BOOST_CHECK(note1 == note2);

        Note note3(4, 5);
        // add complex symbols in a different order
        note3.SetSlideInto(Note::slideIntoFromBelow);
        note3.SetTrill(7);

        BOOST_CHECK(note1 == note3);
    }

    BOOST_AUTO_TEST_CASE(Serialization)
    {
        Note note(4, 5);
        note.SetTrill(7);
        note.SetSlideOutOf(Note::slideOutOfDownwards, 3);

        testSerialization(note);
    }

    BOOST_AUTO_TEST_CASE(Trill)
    {
        Note note;
        note.SetTrill(3);

        BOOST_CHECK(note.HasTrill());

        uint8_t trill;
        note.GetTrill(trill);
        BOOST_CHECK_EQUAL(trill, 3);

        note.ClearTrill();
        BOOST_CHECK(!note.HasTrill());
    }

BOOST_AUTO_TEST_SUITE_END()

