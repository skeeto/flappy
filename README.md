# Ncurses Flappy Bird

Play online via telnet:

 * `telnet flappy.nullprogram.com`

![](http://i.imgur.com/BEzBFl8.gif)

## Run Your Own Server

Build the game (`make`), install an inetd daemon and telnetd and put
this in `/etc/inetd.conf`.

    telnet stream tcp nowait telnetd /usr/sbin/tcpd /usr/sbin/in.telnetd -L /path/to/flappy

By default the high scores database will be kept in a SQLite database
in `/tmp`. Use the `-d` option to change it.
