# bsdiff
An experimental tool to diff bitstream files

I'm trying to reverse engineer some proprietary binary files, and I
noticed that these files are bit streams (as opposed to byte stream).

My approach is to compare two files that have minimal diferrence and
see how the bitstream behave, I need to be able to detect bit flips as
well as bit insertion/deletion between two files.

This is a very beta version of such a tool, it attempts to mimic the
unix `diff` tool, but display bits instead of text.

For now, the algorithm is very stupid and simple, it doesn't yet
understand the concept of context as used in GNU diff and patch.

I might improve this tool to fit my needs.

# Example output

In the example below, you can see that at position 0, the first file
contains `1100` where as the second file contains `0010`.

Then the files are identical up to the 144975'th bit. At this bit
position, the first file contains `110` whereas the second one
contains `01100`, that is the second file as 2 more bits that the
first one.

The two files have then roughly 10k identical bits, after which there
a new difference `110` vs `011010110`, ...

As you might have noticed, this tool is able to track bit assertion
and removal.

    $ bsdiff Si8605AC-B-IS1.bxl Si8606AC-B-IS1.bxl
    --- Si8605AC-B-IS1.bxl
    +++ Si8606AC-B-IS1.bxl
    @@ [4,0] -4,-4 +4,4 @@
    -1100
    +0010
     ^^^
    @@ [3,2] -144975,-3 +144975,5 @@
    -  110
    +01100
     ^^ ^
    @@ [3,6] -152419,-3 +152421,9 @@
    -      110
    +011010110
     ^^^^^^
    @@ [2,0] -154292,-2 +154300,2 @@
    -11
    +01
     ^
    ...


# Sample files

https://www.silabs.com/products/power/isolators/Pages/Si86xx-Digital-Isolators.aspx

