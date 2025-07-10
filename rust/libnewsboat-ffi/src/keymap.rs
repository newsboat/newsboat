#[cxx::bridge(namespace = "Newsboat::keymap::bridged")]
mod ffi {
    #[derive(Default)]
    struct Operation {
        tokens: Vec<String>,
    }

    #[derive(Default)]
    struct Binding {
        key_sequence: String,
        contexts: Vec<String>,
        operations: Vec<Operation>,
        description: String,
    }

    extern "Rust" {
        // `tokenize_operation_sequence()` returns `Option<Vec<Vec<String>>>`, but cxx doesn't
        // support `Option` and doesn't allow `Vec<Vec<_>>`. Here's how we work around that:
        //
        // 1. C++ doesn't care if the parse failed or just returned no operations, so we drop the
        //    `Option` and represent both `None` and `Some([])` with an empty vector (see
        //    `tokenize_operation_sequence()` code further down below);
        //
        // 2. we put `Vec<String>` into a shared struct type called `Operation`.
        fn tokenize_operation_sequence(
            input: &str,
            description: &mut String,
            allow_description: bool,
            parsing_failed: &mut bool,
        ) -> Vec<Operation>;

        fn tokenize_binding(input: &str, parsing_failed: &mut bool) -> Binding;
    }
}

fn tokenize_operation_sequence(
    input: &str,
    description: &mut String,
    allow_description: bool,
    parsing_failed: &mut bool,
) -> Vec<ffi::Operation> {
    match libNewsboat::keymap::tokenize_operation_sequence(input, allow_description) {
        Some((operations, opt_description)) => {
            *parsing_failed = false;
            *description = opt_description.unwrap_or_default();
            operations
                .into_iter()
                .map(|tokens| ffi::Operation { tokens })
                .collect::<Vec<_>>()
        }
        None => {
            *parsing_failed = true;
            vec![]
        }
    }
}

fn tokenize_binding(input: &str, parsing_failed: &mut bool) -> ffi::Binding {
    match libNewsboat::keymap::tokenize_binding(input) {
        Some(binding) => {
            let operations = binding
                .operations
                .into_iter()
                .map(|tokens| ffi::Operation { tokens })
                .collect();
            ffi::Binding {
                key_sequence: binding.key_sequence,
                contexts: binding.contexts,
                operations,
                description: binding.description.unwrap_or("".to_owned()),
            }
        }
        None => {
            *parsing_failed = true;
            ffi::Binding::default()
        }
    }
}
