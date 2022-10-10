Terminal based progress tracker for the [advent of code](https://adventofcode.com/)
made in C99.

### Building

```
gcc aoc.c -O3 -std=c99 -o aoc
```

### Using

By default the progress data is stored in a file called `.aoc` located in the
same directory as the executable. To set a different name or location, change
the value of the `FILENAME` macro in the source file before compiling.

This file contains binary data and must not be edited other than through the
executable (including any potential 'white space' or line feed).

Display progress:

```
aoc show
```

Add a language:

```
aoc add SomeLanguage
```

Edit a specific day:

```
aoc [clear | complete | start] 2018 04 SomeLanguage
```

Get a random non completed day + language:

```
aoc [get | random]
```

Other:

```
aoc help
```

### Other

Some 'technical' limits, all of which can be solved with minor changes to the
code:

* Language names cannot be longer than 255 characters
* Can only store up to 255 years, starting from 2015
* Can only add up to 2^31 languages (maximum value of a 32 bits signed integer)
* Displaying will only show the first 64 languages
* Generating a random day will only poll from the first 64 languages is only as
random as `rand()` can get

File encoding / how it works:

* The first byte of the file is the number of years that are stored
* The following pattern is repeated until the end of the file:
  * A byte which is the size of a language's name
  * An equal number of bytes which contain the language's name
  * A number of bytes equal to 8 times the value of the max year byte; these
  bytes store the progress of each year for this language in each group of 8 
  bytes.

For each language / year pair, there is a corresponding group of 64 bits, in
which the data for a day `d` is stored at the bits `2 * d` and `2 * d + 1`
according to the following rules:
* if the bit at index `2 * d` is 1, then the day is completed,
* else if the bit at index `2 * d + 1` is 1, then the day has been started,
* else it was not started yet (and both bits will be set to 0).

This means that only 51 of the 64 bits are used, effectively leaving 13 useless
bits at the end of each 64 bits word.
