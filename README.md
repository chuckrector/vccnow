# vccnow

![Mid-debuggifications](/data/mid-debuggifications.png)

This is very chaotic right now.

At first, I was playing with just writing a tokenizer for V1 VCC. That grew into my rewriting the preprocessor to use it. That grew into writing a decompiler. After troubles, that grew into writing a disassembler.

I've been watching [Handmade Hero](https://handmadehero.org/), so everything was converted to Windows batch files and `cl.exe`. The whole thing is now a single translation unit. I've renamed and formatted a ton of stuff in Casey's style, with the help of `clang-format` and I have `editor.formatOnSave` enabled in [VSCode](https://code.visualstudio.com/).

I've renamed a bunch of globals from the old compiler with an eye toward removing them from the global scope eventually.

There are no extremely good reasons for most of this, other than I'm having fun relearning C/C++ and trying random things just to see if I can do them.

## setup for building

Similar to HH, I created a `shell.bat` which I launch from a `cmd.exe` shell shortcut on my desktop. That configures `cl.exe` with the right environment and automatically launches VSCode and [RemedyBG](https://remedybg.itch.io/remedybg). The `build` and `s` folders are added to the `PATH` so I can easily run scripts or binaries.

I have VSCode and the `cmd.exe` window opening at specific sizes and location, which was mostly a response to how Windows 10 constantly reboots my machine overnight and causes me to lose my context on various projects. I use virtual desktops, so I'd then have to slowly reopen and arrange everything after a reboot. Now I just click various shortcuts while on given desktops and everything is mostly reconstituted and ready to go.

I have Visual Studio 2019 installed, but I don't use it anymore and mostly do everything from the command-line now.

Assuming that whole setup, I just run `build` and then `vccnow` from `C:\dev\vccnow`.

## minified output

This is a byproduct of my tokenizer and I didn't really intend to create it. `clang-format` can more or less format this output just fine as if it were C, so I've been piping decompiled output into `.vc` files and then pressing `CTRL+S` in my editor and letting the auto-formatting take care of making things readable.

## memory

I allocate all memory up front with a single `malloc()` and split it into pools. Everything is bump allocated within each pool after that and nothing is ever freed. I'm sure it's using way too much memory. Especially for tokens, which I am probably redundantly creating too many of and for which I made the pool much larger than the other pools. I have some basic tests that exercise most of the things I care about, like not breaking compilation and decompilation.

## strings

I use good old `char` buffers everywhere, mostly as an exercise to reacquaint myself with C. I'll convert it to something else later, either as an exercise or if it becomes too much of a pain in the butt. So far it's only mildly annoying.

## command-line

I put everything into a single `vccnow` command-line with a bunch of options. Currently:

```
Usage:
  vccnow <mode> <file>

Modes:
  a - disassemble
  c - compile (writes to file.compiled if no file.map found)
  d - decompile (can be file.map or file.compiled)
  p - preprocess
  t - tests (no file argument)
  v - validate script offset table
  x - extract compiled script from map

Mode modifiers:
  q - quiet (legacy)
  v - verbose (legacy)
  l - low debug logging
  m - medium debug logging
  h - high debug logging

Examples:
  vccnow c test.vc
  vccnow al test.compiled
  vccnow th
```

The preprocessor is token-based and may or may not be faithful to the original V1 VCC version. It's split out mostly for debugging purposes. Most flags exist for debugging purposes, honestly.

## logging

The old quiet/verbose stuff is there because that's what all the old code used. I tacked on logging level stuff after that when debugging some decompilation issues. It's not particularly consistently used. But since, for example, decompilation has to completely correct parse everything before outputting the final "minified" result of all the tokens, if it fails it won't output anything. That's when I use `dl` to emit tokens as I go, so I can see the ragged bloody edge of what it was trying to do up until it exploded.

## disassembler

I lazily inlined it with the decompiler and do one or the other based on flag checks everywhere. The disassembler doesn't stop on parse failures and instead tries to recover and will continue outputting as much dissasembled output as possible.
