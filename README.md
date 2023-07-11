# T-Machine
 G-like programming language for controlling machines.

T stands for TABLE, and also TOOL. This is a T-Machine code example:
| T0     | T1     |
|--------|--------|
| S15    | S15    |
| E10    | AR360  |
| E10    | A      |
| E10    | A      |


 # Keywords
| Keyword | Description                       |
|---------|-----------------------------------|
| E       | Extrude millimeters               |
| R       | Rotate degrees                    |
| T       | Temporize milliseconds            |
| P       | Power up/down tool                |
| S       | Tool speed in rpm                 |
| J       | Jump anchor for flow control      |
| L       | Land anchor for flow control      |
| A       | Declare as asynchronous           |
| F       | Define a function (NotImplemented)|
| C       | Call a function (NotImplemented)  |
| *       | Idle, can be used to synchronize tool |



# Flow Control
T-Machine has control flow in the shape of a for-loop. **L** sets the pointer from which the program will start looping, **Jx** Starts the loop where **x** is the counter.

The following program is for a wire bender that creates a square by feeding 10mm, bending 90° and back, repeats 3 more times.
| T0     | T1     |
|--------|--------|
| S15    | S15    |
| L      | L      |
| E10    | *      |
| *      | R90    |
| *      | R-90   |
| J3     | J3     |



# Synchrony
It is *extremely* important for T-Machine to have asynchrony so you can give instructions to any tool regardless if another tool is currently running. For example, a feeder that is always running while the rest of the tools are bending, clawing, sawing, etc..

| T0      | T1     | T2     |
|---------|--------|--------|
| S15     | S15    | S15    |
| AE1000  | R90    | R-90   |
| A       | R-90   | R90    |
| A       | R90    | R-90   |
| A       | R-90   | R90    |
| *       | *      | *      |

Note that the asynchronus tool must keep beeing declared as 'A' because, by default, is declared synchronus.

Also note that the las line of asterisks, although not instructing anything at all, it will synchronize all tools because by default all aruments are synchronus.



# Fun-Facts
Since T stands for TABLE and TOOL, T-Machine was gonna be called TT-Machine

