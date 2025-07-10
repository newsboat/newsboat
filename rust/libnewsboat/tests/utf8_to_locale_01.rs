use section_testing::{enable_sections, section};

use libNewsboat::utils::utf8_to_locale;

mod locale_helpers;

enable_sections! {
    #[test]
    #[ignore]
    fn t_utf8_to_locale_converts_text_from_utf8_to_the_locale_encoding() {
    if section!("UTF-8") {
        locale_helpers::set_locale("en_US.UTF-8");

        assert_eq!(utf8_to_locale(""), &[]);

        // "Just testing" in Russian.
        let input = "Просто проверяю";
        let expected = &[ 0xd0, 0x9f, 0xd1, 0x80, 0xd0, 0xbe, 0xd1, 0x81, 0xd1, 0x82, 0xd0, 0xbe,
        0x20, 0xd0, 0xbf, 0xd1, 0x80, 0xd0, 0xbe, 0xd0, 0xb2, 0xd0, 0xb5, 0xd1, 0x80, 0xd1, 0x8f,
        0xd1, 0x8e];
        assert_eq!(utf8_to_locale(input), expected);
    }

    if section!("KOI8-R") {
        locale_helpers::set_locale("ru_RU.KOI8-R");

        assert_eq!(utf8_to_locale(""), &[]);

        // "another test" in Russian.
        let input =  "ещё один тест";
        let expected = &[ 0xc5, 0xdd, 0xa3, 0x20, 0xcf, 0xc4, 0xc9, 0xce, 0x20, 0xd4, 0xc5, 0xd3, 0xd4];
        assert_eq!(utf8_to_locale(input), expected);
    }

    if section!("CP1251") {
        locale_helpers::set_locale("ru_RU.CP1251");

        assert_eq!(utf8_to_locale(""), &[]);

        // "Greetings!" in Russian.
        let input =  "Приветствую!";
        let expected = &[ 0xcf, 0xf0, 0xe8, 0xe2, 0xe5, 0xf2, 0xf1, 0xf2, 0xe2, 0xf3, 0xfe, 0x21];
        assert_eq!(utf8_to_locale(input), expected);
    }
    }
}
