```
   __ __         __
  / //_/__  ____/ /__ ___ ___ ___ _
 / ,< / _ \/ __/ / -_|_-<(_-</ _ `/
/_/|_|\___/_/ /_/\__/___/___/\_,_/

  -= midi singer =-

```

Korlessa lets you play that damn synth from a command line. You don't need other fancy tools just fire your synth up, plug it into the computer, and write some cool tunes. And, oh, you need a Linux computer. Sorry.

In other words; Korlessa is a command line sequencer. It can schedule midi events based on an original music notation format.

## How to compile

You need [ALSA](https://www.alsa-project.org/) powered Linux to compile Korlessa. No other dependencies are needed.

Download or clone the project:

```console
$ git clone https://github.com/geomodular/korlessa.git
```

Run `make`:

```console
$ make
```

Now run `make install`. Set up your `$PREFIX` if you don't want to install into `/usr/local`:

```console
$ PREFIX=$HOME/.local make install
```

The `korlessa` binary should be available in your system after the installation.
