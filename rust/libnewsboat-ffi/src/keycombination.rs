use libNewsboat::keycombination;

// cxx doesn't allow to share types from other crates, so we have to wrap it
// cf. https://github.com/dtolnay/cxx/issues/496
struct KeyCombination(keycombination::KeyCombination);

#[cxx::bridge(namespace = "Newsboat::keycombination::bridged")]
mod ffi {
    extern "Rust" {
        type KeyCombination;

        fn from_bindkey(input: &str) -> Box<KeyCombination>;
        fn from_bind(input: &str) -> Vec<KeyCombination>;

        fn get_key(key_combination: &KeyCombination) -> &str;
        fn has_shift(key_combination: &KeyCombination) -> bool;
        fn has_control(key_combination: &KeyCombination) -> bool;
        fn has_alt(key_combination: &KeyCombination) -> bool;
    }
}

fn from_bindkey(input: &str) -> Box<KeyCombination> {
    Box::new(KeyCombination(keycombination::bindkey(input)))
}

fn from_bind(input: &str) -> Vec<KeyCombination> {
    keycombination::bind(input)
        .into_iter()
        .map(KeyCombination)
        .collect()
}

fn get_key(key_combination: &KeyCombination) -> &str {
    key_combination.0.get_key()
}

fn has_shift(key_combination: &KeyCombination) -> bool {
    key_combination.0.has_shift()
}

fn has_control(key_combination: &KeyCombination) -> bool {
    key_combination.0.has_control()
}

fn has_alt(key_combination: &KeyCombination) -> bool {
    key_combination.0.has_alt()
}
