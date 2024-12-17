# CATFACE
VM-based programming language ecosystem created for educational purposes.

# Why CATFACE?
Idk, really. This project is created entirely to study how compiler & VM works (and earn some experience writing them).

# Applications

### Assembler
### Disassembler
### Executor
### Linker
### Compiler

# Language example
```catface
// Quadratic equation solver program

// declare intrisicts

fn _cfvm_f32_sqrt(n: f32) f32;
fn _cfvm_f32_read() f32;
fn _cfvm_f32_write(n: f32);

fn discriminant(a: f32, b: f32, c: f32) f32 {
    return b * b - 4.0 as f32 * a * c;
}

fn f32_abs(a: f32) f32 {
    if a < 0.0 as f32 {
        return 0.0 as f32 - a;
    } else {
        return a;
    }
}

fn f32_is_same(a: f32, b: f32) u32 {
    return f32_abs(a - b) < 0.001 as f32;
}

fn main() {
    let a: f32 = _cfvm_f32_read();
    let b: f32 = _cfvm_f32_read();
    let c: f32 = _cfvm_f32_read();

    let d: f32 = discriminant(a, b, c);

    if f32_is_same(d, 0) {
        _cfvm_f32_write(0.0 as f32 - b / (2.0 as f32 * a));
        return;
    }

    if d > 0 {
        let sqrt_d: f32 = _cfvm_f32_sqrt(d);
        let inv_double_a: f32 = 0.5 as f32 / a;
        let neg_b: f32 = 0.0 as f32 - b;

        _cfvm_write_f32((neg_b - sqrt_d) * inv_double_a);
        _cfvm_write_f32((neg_b + sqrt_d) * inv_double_a);
    }
}
```

# Installation

1. Install dependencies (project depends on SDL2 library only, so this is a bit system-dependent)
2. Clone project repo
    ```bash
    git clone https://github.com/tiot2/catface.git
    ```
3. Build the project
    ```bash
    cd <project clone directory>
    mkdir build
    cd build
    cmake ..
    make
    ```
4. Have fun!

# License
Project is distributed under MIT License (see 'LICENSE' for more information).

