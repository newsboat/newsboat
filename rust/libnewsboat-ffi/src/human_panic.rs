use abort_on_panic;
use libnewsboat::human_panic;

#[no_mangle]
pub extern "C" fn rs_setup_human_panic() {
    abort_on_panic(|| {
        human_panic::setup();
    })
}
