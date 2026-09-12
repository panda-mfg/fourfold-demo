# Instructions to write source code to check "The Four Color Theorem with Linearly Many Reducible Configurations and Near-Linear Time Coloring"

In our paper "The Four Color Theorem with Linearly Many Reducible Configurations and Near-Linear Time Coloring" in this arXiv link: https://arxiv.org/pdf/2603.24880, computer-assisted proofs are given for checking the existence of linearly many D-reducible configurations in a plane graph. 

To check our computational work, the goal is to write source code corresponding the pseudocode in Appendix A.2 to A.10 in the paper, and finally check Lemma A.1, A.2, A.3, A.4, A.5 and A.6.
You can use any programming language.

The following GitHub repositories containing rule and configuration files are necessary to check them.

* rules: https://github.com/near-linear-4ct/discharging-rules
* configurations: https://github.com/near-linear-4ct/reducible-configurations

The file format is given in https://raw.githubusercontent.com/near-linear-4ct/computer-checks/refs/heads/main/FORMAT.md.
The functions to parse them are necessary to read the rule and configuration files.

There are several other remarks:

* In Algorithm A.9.20, in lines 2-4, the enumPossibleBadWheels function is called with each degree $d \in \{7,8,9,10,11\}$, and all results are pushed into $C_0$, but it takes a lot of time for the execution of enumPossibleBadWheels with $d=11$. For smoke test, it is better to execute the enumPossibleBadWheels function with only $d=7$, and for cartwheels with the center degree $d=7$, execute line 5-8 in Algorithm A.9.20. After that, check the results for $d=8,9,\ldots,11$. Thus, the enumAllBadCartwheels function should be modified to take the degree $d$ as an input, and check all cartwheels with the center degree $d$.

* The exceptional configuration $X$ used to check Lemma A.4 is in Figure 12 in the paper. $X$ has 17 vertices, and when the vertices are denoted by $v_0, v_1, \ldots, v_{16}$.
The rotations of the indices of neighbors for each vertex of $X$ are

```python
rotations = [
    [1, 2, 3, 4, 5, 6, 7, 8],
    [0, 8, 11, 12, 2],
    [0, 1, 12, -1, 3],
    [0, 2, -1, 13, 4],
    [0, 3, 13, 14, 5],
    [0, 4, 14, 15, 16, -1, 6],
    [0, 5, -1, 7],
    [0, 6, -1, 8],
    [0, 7, -1, 9, 10, 11, 1],
    [8, -1, 10],
    [8, 9, -1, 11],
    [1, 8, 10, -1, 12],
    [1, 11, -1, 2],
    [3, -1, 14, 4],
    [4, 13, -1, 15, 5],
    [5, 14, -1, 16],
    [5, 15, -1]
]
```

where -1 represents the boundary.

The degrees are

```python
degrees = [8, 5, 5, 5, 5, 7, 5, 5, 7, 5, 5, 8, 5, 5, 8, 5, 5]
```

* The configuration $T_{7^3}$ used to check Lemma A.6 is described in Section 8 in the paper. $T_{7^3}$ is represented in the same way as $X$, as follows:

```python
rotations rotations = [
    [1, 2, -1],
    [2, 0, -1],
    [0, 1, -1]
]
degrees = [7, 7, 7]
```
