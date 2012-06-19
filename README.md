Profanity
=========

Profanity is a console based jabber client inspired by [Irssi](http://www.irssi.org/),

Installation
------------

Dependencies: ncurses, libstrophe, glib, expat, xml2 and openssl.

Libstrophe can be found at: https://github.com/metajack/libstrophe

To run unit tests requires head-unit: https://github.com/boothj5/head-unit

All other dependencies should have packages for your distribution.

Once depdendencies have been installed, run:

    make

To build and install in the current directory.

Running
-------

    ./profanity

Some older jabber servers advertise SSL/TLS support but don't respond to the handshake,
if you have trouble connecting, run with the `-notls` option:

    ./profanity -notls

Preferences
-----------

User preferences are stored in

    ~/.profanity

The following example is described below:

    [ui]
    beep=false
    flash=true
    showsplash=true

    [connections]
    logins=mylogin@jabber.org;otherlogin@gmail.com

    [colours]
    bkgnd=default
    text=white
    online=green
    offline=red
    err=red
    inc=yellow
    bar=green
    bar_draw=black
    bar_text=black

The `[ui]` section contains preferences for user interface behaviour:

    beep:       Try to sound beep on incoming messages if the terminal supports it
    flash:      Try to make the terminal flash on incoming messages if the terminal supports it
    showsplash: Show the ascii logo on startup

The `[connections]` section contains a history of logins you've used already, so profanity can autocomplete them for you.
This section is populated automatically when you login with a new username.

The `[colours]` sections allows you to theme profanity.  Available colours are

    black, white, red, green, blue, yellow, cyan, magenta

Setting a colour to `default`, lets the terminal use whatever default it would use for foreground/background depending on the setting.

Using
-----

Commands in profanity all start with `/`.

To get a list of possible commands, and find out how to navigate around try:

    /help
