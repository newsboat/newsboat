use libnewsboat::colormanager::{self, ActionHandlerStatus};

// cxx doesn't allow to share types from other crates, so we have to wrap it
// cf. https://github.com/dtolnay/cxx/issues/496
struct ColorManager(colormanager::ColorManager);

#[cxx::bridge(namespace = "newsboat::colormanager::bridged")]
mod ffi {
    enum Status {
        Ok,
        InvalidCommand,
        TooFewParameters,
        CustomErrorMessage,
    }

    struct ActionHandlerResult {
        status: Status,
        message: String,
    }

    struct KeyValue {
        key: String,
        value: String,
    }

    extern "Rust" {
        type ColorManager;

        fn create() -> Box<ColorManager>;

        fn handle_action(
            colormanager: &mut ColorManager,
            action: &str,
            params: &[&str],
        ) -> Box<ActionHandlerResult>;
        fn dump_config(colormanager: &ColorManager) -> Vec<String>;
        fn get_stfl_styles(colormanager: &ColorManager) -> Vec<KeyValue>;
    }
}

fn create() -> Box<ColorManager> {
    Box::new(ColorManager(colormanager::ColorManager::default()))
}

fn handle_action(
    colormanager: &mut ColorManager,
    action: &str,
    params: &[&str],
) -> Box<ffi::ActionHandlerResult> {
    match colormanager.0.handle_action(action, params) {
        Ok(()) => Box::new(ffi::ActionHandlerResult {
            status: ffi::Status::Ok,
            message: String::new(),
        }),
        Err(ActionHandlerStatus::InvalidCommand) => Box::new(ffi::ActionHandlerResult {
            status: ffi::Status::InvalidCommand,
            message: String::new(),
        }),
        Err(ActionHandlerStatus::TooFewParameters) => Box::new(ffi::ActionHandlerResult {
            status: ffi::Status::TooFewParameters,
            message: String::new(),
        }),
        Err(ActionHandlerStatus::CustomErrorMessage(message)) => {
            Box::new(ffi::ActionHandlerResult {
                status: ffi::Status::CustomErrorMessage,
                message,
            })
        }
    }
}

fn dump_config(colormanager: &ColorManager) -> Vec<String> {
    colormanager.0.dump_config()
}

fn get_stfl_styles(colormanager: &ColorManager) -> Vec<ffi::KeyValue> {
    colormanager
        .0
        .get_stfl_styles()
        .into_iter()
        .map(|(key, value)| ffi::KeyValue { key, value })
        .collect()
}
