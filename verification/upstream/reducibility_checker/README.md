# Reducibility checker for planar/apex/projective/toroidal graphs

## Requirements

- g++ (version 10 or more), CMake, Python (3.8+)
- boost, spdlog

## Quick Start

1. Install the requirements above.
2. Run `./preload.sh` to preload necessary files.
3. `./build/a.out -i ./projective_configurations/reducible/dconf/proj0000.dconf` to check the reducibility of `proj0000`.
- Check that the standard output matches the content of `./projective_configurations/reducible/log/proj0000.log`. It should look something like the following:
```
[info] Kempe type: Projective
[info] Vertex size: 12, Edge size: 21, Ring size: 6
[info] Started D-reducibility check
[info] #1: Feasible / Total: 16 / 31
[info] #2: Feasible / Total: 17 / 31
[info] #2: Feasible / Total: 17 / 31
[info] Graph is not D-reducible.
[info] Started C-reducibility check
[info] Trying 849 possible contractions
[info] [0/849] Starting contraction of size 1
[info] [15/849] Starting contraction of size 2
[info] [96/849] Starting contraction of size 3
[info] [295/849] Starting contraction of size 4
[info] [536/849] Starting contraction of size 5
[info] [706/849] Starting contraction of size 6
[info] All colors passed! Contracted: 7, 8, 11, 13, 16, 19
[info] Graph is C-reducible!
```
4. If you wish, you can run `./testall.sh` to generate all outputs for the graphs in `./projective_configurations/reducible/`. 
Folder `log/` will be created, and a log file containing output for each graph will be generated there.
- Note: This will take a VERY long time! Consider using the `-s` option explained below to verify a particular contraction edge set.

## Build

Building can be done with CMake.

```
cmake -S . -B build
cmake --build build
```

## Usage

First, you must preload the Kempe chain information and Coloring information into files.
Run the preload command as follows: 

```
./build/a.out -k 9 -c 18
```

(Note: The numbers feeded to `-k` and `-c` are important to the maximum ring size of the configurations that you want to check. You need `-k <N/2> -c <N>` if you want to check configurations with ring size N.)

Now, prepare a `.dconf` file corresponding to the graph you want to check the reducibility of. 
(The syntax of `.dconf` files are stated at below. )
You may grab something from `./projective_configurations/reducible/dconf/`.

Now, feed the `.dconf` file to the program via `-i` command. The logs will emit via standard output.

```
./build/a.out -i path/to/file.dconf
```

The default Kempe chain is for Kempe chains on projective planes.
If you want to check the graph with other types of Kempe chains, you can use the following options:
- `-l` for planar
- `-a` for apex
- `-t` for toroidal

```
./build/a.out -i path/to/file.dconf -l 
./build/a.out -i path/to/file.dconf -a
./build/a.out -i path/to/file.dconf -t
```

Other options:
- `-v ?` output verbosity (0=info, 1=debug, 2=trace)
- `-h ?` terminating condition when searching for contraction edges (0=terminate after one successful contraction, 1=terminate after searching all possible contractions of successful size, 2=do not terminate until all possible contractions are searched)
- `--cmin ?` designate minimum size of contraction edge set
- `-m ?` designate maximum size of contraction edge set
- `-d` converts the input `.conf` file to `.dconf`

You can use the `-d` option like below:
```
./build/a.out -i path/to/file.conf -d
```
The contents of `.dconf` will be outputted to STDOUT.
If you are curious, we have generated all `./projective_configurations/reducible/conf/*.dconf` by using this program and feeding files from `./projective_configurations/reducible/dconf/*.conf`.

- `-s ?` (example：`-s 2+16+18`, delimit by `+` signs) Check some specific edge set for reducibility. (Good for verifying the log files)
```
./build/a.out -i ./projective_configurations/reducible/dconf/proj0000.dconf -s 7+8+11+13+16+19
```

## .dconf file syntax

`.dconf` files contain the information for islands (the dual of some configuration).

Let $I$ be an island. Let `N` be the number of vertices in $I$ and let `R` be the ring size of $I$.
Let the vertices be labeled `0` to `N-1`. Let the ring edges be labeled `0` to `R-1`, and let the inner edges (edges in $I$) be labeled `R` to `R*2+N*(3/2)-1`. Let vertex `i` be adjacent to edges `a_i`, `b_i`, and `c_i`.

Then, the `.dconf` file of $I$ should be written as follows:

```
N R
a_0 b_0 c_0
:
a_{N-1} b_{N-1} c_{N-1}
```