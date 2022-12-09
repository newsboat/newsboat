#[cxx::bridge(namespace = "newsboat::keymap::bridged")]
mod ffi {
    extern "Rust" {
        // `tokenize_operation_sequence()` returns `Option<Vec<Vec<String>>>`, but cxx doesn't
        // support `Option` and doesn't allow `Vec<Vec<_>>`. Here's how we work around that:
        //
        // 1. C++ doesn't care if the parse failed or just returned no operations, so we drop the
        //    `Option` and represent both `None` and `Some([])` with an empty vector (see
        //    `tokenize_operation_sequence()` code further down below);
        //
        // 2. we put `Vec<String>` into an opaque type `Operation`, and return `Vec<Operation>`.
        //    The opaque type is, in fact, a struct holding the `Vec<String>`. C++ extracts the
        //    vector using a helper function `operation_tokens()`.
        //
        // This is not very elegant, but doing the same by hand using `extern "C"` is prohibitively
        // complex.
        type Operation;
        fn tokenize_operation_sequence(
            input: &str,
            description: &mut String,
            allow_description: bool,
            parsing_failed: &mut bool,
        ) -> Vec<Operation>;

        type Binding;
        fn tokenize_binding(input: &str, parsing_failed: &mut bool) -> Box<Binding>;

        fn operation_tokens(operation: &Operation) -> &Vec<String>;

        fn binding_key_sequence(binding: &Binding) -> &String;
        fn binding_contexts(binding: &Binding) -> &Vec<String>;
        fn binding_operations(binding: &Binding) -> &Vec<Operation>;
        fn binding_description(binding: &Binding) -> &String;
    }

    extern "C++" {
        // cxx uses `std::out_of_range`, but doesn't include the header that defines that
        // exception. So we do it for them.
        include!("stdexcept");
        // Also inject a header that defines ptrdiff_t. Note this is *not* a C++ header, because
        // cxx uses a non-C++ name of the type.
        include!("stddef.h");
    }
}

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

fn tokenize_operation_sequence(
    input: &str,
    description: &mut String,
    allow_description: bool,
    parsing_failed: &mut bool,
) -> Vec<Operation> {
    match libnewsboat::keymap::tokenize_operation_sequence(input, allow_description) {
        Some((operations, opt_description)) => {
            *parsing_failed = false;
            *description = opt_description.unwrap_or_default();
            operations
                .into_iter()
                .map(|tokens| Operation { tokens })
                .collect::<Vec<_>>()
        }
        None => {
            *parsing_failed = true;
            vec![]
        }
    }
}

fn tokenize_binding(input: &str, parsing_failed: &mut bool) -> Box<Binding> {
    match libnewsboat::keymap::tokenize_binding(input) {
        Some((keys, contexts, operations, opt_description)) => {
            *parsing_failed = false;
            let operations = operations
                .into_iter()
                .map(|tokens| Operation { tokens })
                .collect::<Vec<_>>();
            Box::new(Binding {
                key_sequence: keys,
                contexts: contexts.into_iter().map(|s| s.to_string()).collect(),
                operations,
                description: opt_description.unwrap_or_default(),
            })
        }
        None => {
            *parsing_failed = true;
            Box::new(Binding::default())
        }
    }
}

fn operation_tokens(input: &Operation) -> &Vec<String> {
    &input.tokens
}

fn binding_key_sequence(binding: &Binding) -> &String {
    &binding.key_sequence
}

fn binding_contexts(binding: &Binding) -> &Vec<String> {
    &binding.contexts
}

fn binding_operations(binding: &Binding) -> &Vec<Operation> {
    &binding.operations
}

fn binding_description(binding: &Binding) -> &String {
    &binding.description
}
