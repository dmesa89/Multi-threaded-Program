This one one of the final assignments for Operating Systems 1

This program applies the principles of multi-threaded programming and uses the Producer-Consumer model to implement character processing.
The program will read in lines of characters from the standard input and write them as 80 character long lines to 
standard output with the following changes. 
1. Every line separator in the input will be replaced by a space.
2. Every adjacent pair of plus signs ++ is replaced by ^^


1. In order to compile 
"gcc -o line_processor line_processor.c -pthread"

2. to execute:
"./line_processor"
OR, if text file present:
"./line_processor < input.txt"

3. type input