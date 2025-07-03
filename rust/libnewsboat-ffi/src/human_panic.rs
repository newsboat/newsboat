use crate::abort_on_panic;
use libnewsboat::human_panic;

#[unsafe(no_mangle)]
pub extern "C" fn rs_setup_human_panic() {
    abort_on_panic(|| {
        human_panic::setup();
    })
}
