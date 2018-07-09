## Kodra

Kodra is an engine for [CheckerBoard](http://www.fierz.ch/checkers.htm) which plays the Russian version of checkers (Shashki).

This engine was supposed to a fun project but turned out play pretty pretty well.

It uses an improved alphaBeta method with several other optimizations, to make the engine competitive i added some of the well known techniques used to speedup the search process, like TT tables, move ordering, late move reduction and more.


### Build

This repo contains the **dll** which you can use with the CheckerBoard program, you can also build manually by running,

```bash
make dll
```
