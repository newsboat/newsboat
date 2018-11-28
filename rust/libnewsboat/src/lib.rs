#[macro_use] extern crate once_cell;


pub mod logger;

macro_rules! log {
    ( $level:expr, $message:expr ) => {
        logger::get_instance().log($level, $message);
    }
}

pub mod utils;
