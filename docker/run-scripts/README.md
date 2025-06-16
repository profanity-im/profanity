# Run Profanity from the image

**Without external customizations**

Sometimes you don't care, and just want a quick one-off for testing purposes:

$CMD run -it <NAME>:latest

**With customizations read from existing environment**

$HOME == full path to your home directory (Ex: /home/alice)

**Example**

$CMD run -v $HOME/.config/profanity:/root/.config/profanity -it <NAME>:latest

The "run.sh" implements a wrapper around just such a call. Edit the script for your
environment. 

## Theme support issues (Remote systems, and Docker containers)

You'll see on remote systems sometimes that certain themes simply will not load (As in, SSH
into remote, then run profanity from there). 

That's due to limitations of its terminal implementaton, and its ability to draw color. It's
not a reflection of Profanity being "broken". For example the author defaults to 
gruvbox_transparent, which does not work on RHEL 9 server consoles whereas batman or
hacker themes work just fine.

This holds true as well within Docker containers. The default terminal does not support more
comprehensive color schemes. It may be possible to patch them to support better colors though
this is currently an exercise for the reader.


