##Yuneta Course

Skills
======

- Operative System: Linux.
- Languages: C, Python, Javascript.
- Version Control System: Git.
- Documentation: Sphinx (reStructuredText language).
- IDE's: Kdevelop

Index
=====

- 03-print-hello-name

Compilation and intall
=======

1. Create build directory and move to it::
    
    mkdir build
    cd build/

2. Compile::

    cmake -DCMAKE_BUILD_TYPE=Debug ..

3. Install::
    
    make
    make install

4. Run program::

    ../bin/03HelloName ARG1 [STRING]
