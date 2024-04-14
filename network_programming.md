# Lecture 1: Intro to C++
## The 3 Perspectives
In this course, we discuss the paradign of OOP from 3 perspectives:
1. the programmers' perspective
    - how to structure programs correctly
    - how to lower risk of bugs
2. the compiler's perspective
    - what do our constructions mean?
    - what must a compier do to actually support them?
3. the designer's perspective
    - how can we use the toolds that OOP provides to build systems
    - basic SE

## Code Comparison: C vs. C++
```C
// C
#include <stdio.h>
int main() {
    printf("Hello World");
    return 0;
}
```
```C++
import <iostream>;
using namespace std;
int main() {
    cout << "Hello World" << endl;
    return 0;
}
```
**Notes:**
- `main` must return int (`void` main is illegal)
- `return` statement returns status code to OS which can be omitted from main (0 is assumed)
- `stdio`, `printf` is still available in C++
    - prefer C++ I/O header: `<iostream>`
- `std::endl` = end of line and flush output buffer 
    - any data that has been buffered (i.e., temporarily held in memory) for output will be immediately written to the  ostream
> - **Buffer:** char doesn't go immediately, waits until there is enough char to print at once
- `using namespace std;` lets you omit `std::` prefix
    - C++ putting a name in front of functions so that functions you write do not clash

## I/O
### iostreams
1. `std::cout`/`std::cerr` for printing
2. `std::cin` for reading from stdin
### I/O operators
- `<<` "put to" (output)
- `>>` "get from" (input)

**Examples:**
- `std::cer << x;` takes from x to cerr stream
- `std::cin >> x` takes from keyboard into `x`
### Problem: (Ex) Adding 2 numbers
```C++
import <iostream>;
using namespace std;
int main(){
	int x,y;
	cin >> x >> y;
	cout << x + y << endl;
}

// Compile:
// g++20 plus.cc -o plus
// ./plus
```
- what if input doesn't contain an integer next?
- what if the input is too large/ small?

### Solution: (Ex) Reading Ints from stdin
If the read fails, `cin.fail()` will be true. If it's the EOF, then both `cin.fail()` `cin.eof()` will be true. But not until the attempted read fails!
- terminology from `cin.fail()`: `cin` is the *field selector*, `fail()` is the *function call*
```C++
// reading all ints from stdin and echo them, one per line, to stdout
// stop on bad input or eof
import <iostream>;
using namespace std;

int main() {
    int i;
    while (true) {
        cin >> i;
        if (cin.fail()) break; // also can write (!cin)
        cout << i << endl;
    }
}
```
**Note:**
- there is an implicit conversion from cin's type (istream) to bool
- `>>` is C & C++ bit shift operator
    - let a and b be ints
    - `a >> b` shifts a's bits to the right by b spots
    - ex: `21 >> 3` = 2 (21 = 10101 &rarr; shift 3 bits &rarr; 10)
- but when LHS is an istream (ex: `cin`)
    - `>>` is the "get from" operator
    - first example of **overloading**
    
