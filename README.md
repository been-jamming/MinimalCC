# MinimalCC
Minimal C subset compiler intended to eventually run on my calculator

However, it currently compiles to MIPS assembly which runs on MARS and SPIM.

Build using `make`, which will produce an executable named `mcc`.

## Quickstart (Execution using SPIM)
There is a folder called `tests`, which allows you to test the assembler. 

We will build and execute a program which prints all of the primes less than 5000. First compile the file `tests/primes.cc` using the command `./mcc tests/primes.cc -o tests/primes.s`. This creates an assembly file named `primes.s` in the `tests` folder. Next, we need to add it's dependencies so that SPIM can execute the program. In `test` there is a file named `main.s` which serves as an entry point into the C program. Also, there are files `prints.s` and `printd.s` which are functions to print strings and integers respectively. We can combine these files into one file named `run.s` using the command `cat tests/primes.s tests/main.s tests/prints.s tests/printd.s > run.s`. Finally, if you are using SPIM, you can execute the program by running `spim -file run.s`. After a few seconds of calculation, it should print all of the prime numbers less than 5000! You can change the program in `tests/primes.cc` how you like to change the functionality. 

## Features
### List of Binary Operations
- `+` Addition
- `-` Subtraction
- `*` Multiplication
- `/` Division
- `=` Assign
- `<` Less than
- `>` Greater than
- `==` Equals
- `!=` Not equals
- `&` Bitwise AND
- `|` Bitwise OR
- `%` Modulo
- `&&` Logical AND
- `||` Logical OR
- `<<` Shift left
- `>>` Shift right (arithmetic)

### List of Unary Operations
- `*` Dereference
- `&` Reference
- `!` Logical not
- `~` Bitwise not
- `-` Negation

### List of Supported Datatypes
- The following primitive datatypes:
  - `int` which is 4 bytes big
  - `char` which is 1 byte big
  - `void`
- Pointers to another type
- Functions and function pointers
- Local and global lists

### List of Supported Keywords
- `while`
- `if`
- `else`
- `return`

Note that due to the efficiency considerations, datatypes take up constant storage inside the compiler, which means that datatypes that are complicated enough can "overflow" the type system in the compiler. Currently there is no detection for this, but this is another thing I plan to add. It is also possible to modify the compiler to be able to parse even more complex datatypes, but it becomes a trade-off between flexibility and efficiency. Currently, almost any datatype a sane programmer would use will not overflow the type system for the compiler. Don't be mad at me for this decision; remember I am trying to make this capable of running on my calculator and able to compile programs fast. 
