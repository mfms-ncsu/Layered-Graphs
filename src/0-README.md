# Running a sequence of heuristics
To run a sequence of heuristics, you can now use unix pipes. For example, the following runs barycenter (with dfs preprocessing), followed by sifting, followed by mce.
```
./minimization -P b_t -p dfs -h mod_bary -i 1000 -O north42.32_GKNV-scr.sgf
  | ./minimization -I -P b_t -h sifting -R 1 -i 10000 -O
  | ./minimization -I -P b_t -h mce -R 3 -i 10000
```
The important (new) options are
* `-O` to send output to stdout, with Pareto points in comments
* `-I` to take input from stdin
Other options in this example are
* `-P` to specify the Pareto objectives; must be the same for each heuristic
* `-p` to specify the preprocessor, used only initially here
* `-h` to specify the heuristic
* `-R` to use randomization with the given seed
* `-i` to specify number of iterations