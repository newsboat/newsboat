extern crate libnewsboat;

use std::env;

struct EnvVariable{
    name: String,
    value: String,
    was_set: bool,
}

impl EnvVariable {

    fn new(name:String) -> EnvVariable {
        match env::var(&name) {
            Ok(val) => EnvVariable{
                name:name,
                value:val,
                was_set:true,
            },
            Err(_) => EnvVariable{
                name:name,
                value:String::new(),
                was_set:false,
            },
        }
    }

    fn set(&self,new_value:&String) {
        env::set_var(&self.name, new_value)
    }

    fn unset(&self) {
        env::remove_var(&self.name)
    }
}

impl Drop for EnvVariable {

    fn drop(&mut self){
        if self.was_set {
            env::set_var(&self.name,&self.value);
        }
    }
}

#[test]
fn t_get_default_browser() {
    use libnewsboat::utils;
    let key = String::from("BROWSER");
    let firefox = String::from("firefox");
    let opera = String::from("opera");
    
    let env_var = EnvVariable::new(key);
    env_var.unset();
    assert_eq!(utils::get_default_browser(),String::from("lynx"));
    
    env_var.set(&firefox);
    assert_eq!(utils::get_default_browser(), firefox);
    env_var.set(&opera);
    assert_eq!(utils::get_default_browser(), opera);
}