use section_testing::{enable_sections, section};

use libNewsboat::utils::utf8_to_locale;

mod locale_helpers;

enable_sections! {
    #[test]
    #[ignore]
    fn t_utf8_to_locale_transliterates_characters_unsupported_by_the_locale_encoding() {
    // Tests below don't spell out the expected result because different
    // platforms might follow different transliteration rules. The best we can
    // do is check that 1) the output is non-empty, i.e. there was no error in
    // conversion; 2) the output is not the same as the input, i.e. the
    // conversion did something.

    if section!("KOI8-R doesn't support Ukrainian characters") {
        locale_helpers::set_locale("ru_RU.KOI8-R");

        // "Song" in Ukrainian.
        let input = "Пісня";

        let result = utf8_to_locale(input);
        assert_ne!(result, &[]);
        assert_ne!(result, input.as_bytes());
    }

    if section!("CP1251") {
        locale_helpers::set_locale("ru_RU.CP1251");

        // "Japan" in Japanese
        let input = "日本";

        let result = utf8_to_locale(input);
        assert_ne!(result, &[]);
        assert_ne!(result, input.as_bytes());
    }
    }
}
