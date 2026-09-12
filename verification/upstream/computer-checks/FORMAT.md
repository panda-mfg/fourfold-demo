# Formats
We give descriptions of formats of configurations, rules, combined rules, and cartwheels.

## Configuration Format
(blank line) \
$N$ $R$ \
$R+1$ $\delta(v_{R+1})$ $a_{R+1}^1$ $\ldots$ $a_{R+1}^{\delta(v_{R+1})}$ \
$\vdots$ \
$N$ $\delta(v_{N})$ $a_{N}^1$ $\ldots$ $a_{N}^{\delta(v_{N})}$

---

$N$ denotes the total number of vertices of a configuration.
$R$ denotes the ring size of a configuration.
The vertex set is $V=\{v_1, \ldots, v_{N}\}$, and vertex set in the ring is $V_R=\{v_1, \ldots, v_{R}\} \subseteq V$.
For each internal vertex $v_i$ ($i=R+1, \ldots, N$), the file lists the vertex index $i$, its degree $\delta(v_i)$, and the clockwise rotation of indices of its neighbors $a_i^1, \ldots, a_i^{\delta(v_i)}$.

Specifically, the vertices in the ring $v_1, \ldots, v_R$ are clockwisely ordered.

## Rule Format
(blank line) \
$N$ $s$ $t$ $r$ \
$1$ $\delta^-(v_1)$ $\delta^+(v_1)$ $a_1^1$ $\ldots$ $a_1^{d(v_1)}$ \
$\vdots$ \
$N$ $\delta^-(v_N)$ $\delta^+(v_N)$ $a_N^1$ $\ldots$ $a_N^{d(v_N)}$

---

$N$ denotes the total number of vertices of a rule.
The vertex set is $V=\{v_1, \ldots, v_N\}$.
$s, t$ denotes the index of the vertex that charge sends/receives respecitvely.
$r$ denotes the amount of charge.
For each vertex $v_i$ ($i=1,\ldots,N$), the file lists the vertex index $i$, its degree-range $\delta^-(v_i), \delta^+(v_i)$, and the clockwise rotation of indices of its neighbors $a_i^1, \ldots, a_i^{d(v_i)}$.
The degree $\infty$ is represented by $0$.
When $a_i^j=-1$, it representes the boundary.


## Combined Rule Format
(blank line) \
$N$ $s$ $t$ $r$ \
$1$ $\delta^-(v_1)$ $\delta^+(v_1)$ $a_1^1$ $\ldots$ $a_1^{d(v_1)}$ \
$\vdots$ \
$N$ $\delta^-(v_N)$ $\delta^+(v_N)$ $a_N^1$ $\ldots$ $a_N^{d(v_N)}$ \
$F$

---

Except for the last line, the format is identical to the Rule format.
The last line contains a $01$ sequence $F$ representing which rules are combined.
When combining rules, we order rules by their filenames, resulting $R_0, \ldots, R_{k-1}$.
The length of $F$ is $k$.
If the $i$-th character $F[i]$ is $1$, it means the rule $R_i$ is included in the combination.

## CartWheel Format
(blank line) \
$N$ $c$ \
$1$ $\delta^-(v_1)$ $\delta^+(v_1)$ $a^1_1$ $\ldots$ $a^1_{d(v_1)}$ \
$\vdots$ \
$N$ $\delta^-(v_N)$ $\delta^+(v_N)$ $a^N_1$ $\ldots$ $a^N_{d(v_1)}$

---

$N$ denotes the total number of vertices of a cartwheel.
The vertex set is $V=\{v_1, \ldots, v_N\}$.
$c$ denotes the index of the center of a cartwheel.
The other format is identical to the Rule format.
