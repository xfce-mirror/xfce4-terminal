## What is it?

Xfce Terminal is a lightweight and easy to use terminal emulator application
with many advanced features including drop down, tabs, unlimited scrolling,
full colors, fonts, transparent backgrounds, and more.


## Installation

The file [`INSTALL`](INSTALL) contains generic installation instructions.


## Performance issues

Xfce Terminal is based on the Vte terminal widget library, just like
gnome-terminal. Vte is probably not the fastest terminal emulation library on
earth, but it's one of the best when it comes to Unicode support, and not to
forget, it's actively developed. That said, performance is still an important
issue for a terminal emulator and Vte with font-antialiasing enabled can be
very slow even on decent systems. Xfce Terminal therefore offers a possibility
to explicitly disable anti-aliasing for the terminal font. If you are
experiencing problems with the terminal rendering speed, you might want to
disable the anti-aliasing for the terminal font.


## How to report bugs?

Bugs should be reported to the [Xfce bug tracking system](https://bugzilla.xfce.org)
against the product Xfce4-terminal. You will
need to create an account for yourself.

Please read the [`HACKING`](HACKING) file for information on where to send changes
or bugfixes for this package.
