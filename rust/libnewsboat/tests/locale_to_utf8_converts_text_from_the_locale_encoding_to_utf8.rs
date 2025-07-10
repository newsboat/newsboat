use section_testing::{enable_sections, section};

use libNewsboat::utils::locale_to_utf8;

mod locale_helpers;

enable_sections! {
    #[test]
    #[ignore]
    fn t_locale_to_utf8_converts_text_from_the_locale_encoding_to_utf8() {
    if section!("UTF-8") {
        locale_helpers::set_locale("en_US.UTF-8");

        assert_eq!(locale_to_utf8(&[]), "");

        //, "I like Newsboat" in Russian.
        let expected = "Newsboat мне нравится";
        let input = &[ 0x4e, 0x65, 0x77, 0x73, 0x62, 0x6f, 0x61, 0x74, 0x20, 0xd0, 0xbc, 0xd0,
        0xbd, 0xd0, 0xb5, 0x20, 0xd0, 0xbd, 0xd1, 0x80, 0xd0, 0xb0, 0xd0, 0xb2, 0xd0, 0xb8, 0xd1,
        0x82, 0xd1, 0x81, 0xd1, 0x8f ];
        assert_eq!(locale_to_utf8(input), expected);
    }

    if section!("KOI8-R") {
        locale_helpers::set_locale("ru_RU.KOI8-R");

        assert_eq!(locale_to_utf8(&[]), "");

        // "excellent check" in Russian.
        let expected = "великолепная проверка";
        let input = &[ 0xd7, 0xc5, 0xcc, 0xc9, 0xcb, 0xcf, 0xcc, 0xc5, 0xd0, 0xce, 0xc1, 0xd1,
        0x20, 0xd0, 0xd2, 0xcf, 0xd7, 0xc5, 0xd2, 0xcb, 0xc1];
        assert_eq!(locale_to_utf8(input), expected);
    }

    if section!("CP1251") {
        locale_helpers::set_locale("ru_RU.CP1251");

        assert_eq!(locale_to_utf8(&[]), "");

        // "All tests green!" in Russian.
        let expected = "Все тесты зелёные!";
        let input = &[ 0xc2, 0xf1, 0xe5, 0x20, 0xf2, 0xe5, 0xf1, 0xf2, 0xfb, 0x20, 0xe7, 0xe5,
        0xeb, 0xb8, 0xed, 0xfb, 0xe5, 0x21];
        assert_eq!(locale_to_utf8(input), expected);
    }
    }
}
