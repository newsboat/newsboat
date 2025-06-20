use std::{env::args, fs::read_to_string, process::exit};

fn main() {
    let Some(filename) = args().nth(1) else {
        let program = args().next().unwrap_or("<executable>".to_owned());
        eprintln!("usage: {program} <dsv-file> [<link-prefix>]");
        exit(1);
    };
    let link_prefix = args().nth(2).unwrap_or("".to_owned());

    let Ok(content) = read_to_string(&filename) else {
        eprintln!("couldn't open {filename}");
        exit(1);
    };

    for (i, line) in content.lines().enumerate() {
        let parts: Vec<_> = line.split("||").collect();
        if parts.len() != 5 {
            let num_cells = parts.len();
            eprintln!("expected exactly 5 cells in {filename}:{i}, but got {num_cells} instead");
            exit(1);
        }

        let option = parts[0];
        let syntax = parts[1];
        let default_param = parts[2];
        let desc = parts[3];
        let example = parts[4];

        println!("[[{link_prefix}{option}]]_{option}_ (parameters: {syntax}; default value: _{default_param}_)::");
        println!("         {desc} (example: {example})");
        println!();
    }
}
