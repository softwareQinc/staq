# Installation instructions

[pystaq](https://github.com/softwareQinc/staq/blob/main/pystaq/) is a Python 3
wrapper for **staq**. pystaq can be installed using `pip`

```
pip install git+https://github.com/softwareQinc/staq
```

## Creating python stubs for IDE autocompletion and static type checking

In case autocompletion (or static type checking via [mypy](https://www.mypy-lang.org/))
does not work properly in your editor/IDE, you may need to create python stubs
for the package. To do this, execute

```shell
mkdir ~/python_stubs
export MYPATH=$MYPATH:~/python_subs # put this in your .profile or .bashrc
. ~/venv/bin/activate
stubgen -p pystaq -o ~/python_stubs
ln -s ~/python_stubs/pystaq.pyi ~/venv/lib/python3.11/site-packages
```

In the above, we assumed that your platform is UNIX/UNIX-like and that you have
pystaq installed in a virtual environment under `~/venv`. Please modify
accordingly on your system.

## Overview

To parse a circuit, use the function `pystaq.parse_file`, which takes a file path as input, or `pystaq.parse_str`, which accepts an OpenQASM 2.0 program string.

The library provides the following tools:

```
desugar
inline
synthesize_oracles
simplify
rotation_fold
cnot_resynth
map
get_resources
output_cirq
output_ionq
output_projectq
output_qsharp
output_quil
grid_synth
qasm_synth
lattice_surgery
```

Each function takes as input a parsed program, followed by any options supported by the corresponding staq tool.

---

Example:

```
>>> import pystaq

>>> p = pystaq.parse_str('''
OPENQASM 2.0;
include "qelib1.inc";

qreg q[2];
gate cphase(theta) a,b {
  rz(theta/2) a;
  rz(theta/2) b;
  cx a,b;
  rz(-theta/2) b;
  cx a,b;
}
cphase(pi/4) q[0],q[1];
''')

>>> pystaq.inline(p, clear_decls=True)

>>> p
OPENQASM 2.0;
include "qelib1.inc";

qreg q[2];
rz((pi/4)/2) q[0];
rz((pi/4)/2) q[1];
cx q[0],q[1];
rz(-(pi/4)/2) q[1];
cx q[0],q[1];
```

## Device generator

The `pystaq.Device` class can be used to create custom devices for mapping. It has the methods `add_edge` and `set_fidelity`.

---

Example:

```python3
import pystaq
dev = pystaq.Device(4)           # 4 qubits
dev.add_edge(0, 1)               # default is undirected, with fidelity = FIDELITY_1 from staq
dev.add_edge(2, 3, fidelity=0.8, directed=True)
dev.set_fidelity(0, 0.9)         # single qubit fidelity
with open('device.json', 'w') as f:
    f.write(str(dev))
```

This produces the following `device.json` file, which can then be used by `pystaq.map`:

```js
{
  "couplings": [
    {
      "control": 0,
      "target": 1
    },
    {
      "control": 1,
      "target": 0
    },
    {
      "control": 2,
      "fidelity": 0.8,
      "target": 3
    }
  ],
  "name": "Custom Device",
  "qubits": [
    {
      "fidelity": 0.9,
      "id": 0
    },
    {
      "id": 1
    },
    {
      "id": 2
    },
    {
      "id": 3
    }
  ]
}
```

## Custom Bindings

pystaq was created using pybind11. See [`pystaq/staq_wrapper.cpp`](https://github.com/softwareQinc/staq/blob/main/pystaq/staq_wrapper.cpp) for many examples of how to wrap a circuit transformation.

For more details, see also our Quantum++ wrapper [pyqpp](https://github.com/softwareQinc/qpp/wiki/8.-pyqpp#custom-bindings).
