# Shell
A simple shell that will run one command or a pipeline of commands. The shell has two builtin commands including "exit" and "cd" which are implemented as function pointers to allow for future expandability. All other commands are executed using execvp passing in the command and its arguments.

#Piping
The shell program supports piping an arbitrarily long list of piped commands. To demonstrate its robustness, a copy of <i>Moby Dick</i> by Herman Melville has been included in the repository. Typing the following command:

cat moby.txt |tr A-Z a-z|tr -C a-z '\n' |sed  '/^$/d' |sort|uniq -c|sort -nr|sed 10q

will pipe the following commands in a separate process for each command:

1. cat moby.txt puts the contents of the file moby.txt onto the standard output stream. 

2. tr A-Z a-z translates all uppercase letters to their corresponding lowercase ones. This ensures that mixed-case words all get converted to lower case and will therefore look the same when we compare them. 

3. tr -C a-z '\n' translates anything that is not a lowercase letter into a newline (\n). This gives us one word per line as well as a lot of extra blank lines. 

4. sed '/^$/d' runs sed, the stream editor, to delete all empty lines (i.e., lines that match the regular expression ^$). 

5. sort sorts the output. Now all of our words are sorted. 

6. uniq -c combines adjacent lines (words) that are identical and writes out a count of the number of duplicates followed by the line. 

7. sort -nr sorts the output of uniq by the count of duplicate lines. The sorting is in inverse numeric order (-nr). The output from this is a frequency-ordered list of unique words. 

8.  sed 10q  tells the stream editor to quit after reading ten lines. The user will see the top ten lines of the output of sort -nr. 



