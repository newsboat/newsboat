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
        fn tokenize_operation_sequence(input: &str) -> Vec<Operation>;
        fn operation_tokens(operation: &Operation) -> &Vec<String>;
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

fn tokenize_operation_sequence(input: &str) -> Vec<Operation> {
    match libnewsboat::keymap::tokenize_operation_sequence(input) {
        Some(operations) => operations
            .into_iter()
            .map(|tokens| Operation { tokens })
            .collect::<Vec<_>>(),
        None => vec![],
    }
}

fn operation_tokens(input: &Operation) -> &Vec<String> {
    &input.tokens
}
