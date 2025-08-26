# Run Profanity from the image

**Without external customizations**

Sometimes you don't care, and just want a quick one-off for testing purposes:

$CMD run -it <NAME>:latest

**With customizations read from existing environment**

$HOME == full path to your home directory (Ex: /home/alice)

**Example**

$CMD run -v $HOME/.config/profanity:/root/.config/profanity -it <NAME>:latest

The "run.sh" script implements a wrapper around just such a call. To run: 

1. Edit the script for your environment. Look for section labeled "User editable section".
2. Edit the variables with "##" in the lead, within this section. 
3. Stop editing when you see a line beginning with " ### END...".
4. Run the following chmod command on the shell script: 
   <user@computer> chmod +x run.sh
5. Run the script to start profanity
   ./run.sh
6. Profanity should pop-up automatically in the tab/terminal as expected.

## Notes

Some high level notes on limitations 

### Notifications

External OS notifications will not work. While this was not tested, it's almost 100% 
certain this is the case, running profanity from a Docker container. You may not care
about this lack of functionality, but it will be a problem for those who do care such
as the author who relies on this functionality (as well as "psychic mode"). 

### Theme support issues (Remote systems, and Docker containers)

You'll see on remote systems sometimes that certain themes simply will not load (As in, SSH
into remote, then run profanity from there). 

That's due to limitations of its terminal implementaton, and its ability to draw color. It's
not a reflection of Profanity being "broken". For example the author defaults to 
gruvbox_transparent, which does not work on RHEL 9 server consoles whereas batman or
hacker themes work just fine.

This holds true as well within Docker containers. The default terminal does not support more
comprehensive color schemes. It may be possible to patch them to support better colors though
this is currently an exercise for the reader.


