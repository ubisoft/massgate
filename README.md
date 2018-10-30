# World in Conflict Massgate

# Introduction

Massgate is the central server for the Massive Entertainment game World in 
Conflict that manages the online functionality such as keeping track 
of dedicated game servers, user accounts, clans, ladders etc. The original game 
was released in 2007, and the official Massgate server was shutdown in 2016. 
To make it possible to continue to play World in Conflict online the 
source code of Massgate is now open source, making it possible for anyone 
to host their own Massgate server.

The code itself is more or less the same as how the code looked like back
when the game is released. Only minor tweaks have been made to make it build 
on a relative modern compiler and to remove the necessity to manage CD-keys.
Not much of the code has survived into later releases done by Massive 
Entertainment and does not really reflect to code of the company today, apart
from the code standard and the general look and feel of the code. As a piece
of game development history, and for anyone interested in how online servers
were written at the time, it can definitely be a point of interest.

## Dependencies

Massgate depends on _MySQL_, and it was built with MySQL version 4.2.1 which is
an ancient version today and is 32bit. Massgate has been briefly tested with a 
newer version of MySQL, but there are no guarantees that it will work 
flawlessly.

The game also depends on a web server running to get information about the 
latest patches for the game. Any web server will do, more details below.

## Building Massgate

To build Massgate you need _CMake_ and some version of Visual Studio. At the
moment only Visual Studio 2015 has been tested.

Point CMake to the root folder of the source to configure and generate a 
solution. The solution will end up in the build folder.

## Running Massgate

### MySQL

In order to run Massgate, you need a MySQL server it can connect to and a 
database prepared with data and tables. The commands to create them can be
found in the SQL file under the share/sql folder. 

To set up MySQL on localhost using the command line tool would look 
something like this:

```
mysql> create database live;
mysql> use live;
Database changed
mysql> grant all on live.* to 'massgateadmin'@'localhost' identified by 'adminpassword';
Query OK, 0 rows affected (0.00 sec)

mysql> grant all on live.* to 'massgateclient'@'localhost' identified by 'clientpassword';
Query OK, 0 rows affected (0.00 sec)

mysql> source share/sql/databasestructure.sql
```

In order for Massgate to be able to connect to the database, the hosts
`writedb.massgate.local` and `readdb.massgate.local` need to point to the
database. An easy way of achiving that locally in Windows is to edit the 
Windows hosts file under `C:\Windows\System32\drivers\etc\hosts` by adding the
following lines:

```
127.0.0.1 writedb.massgate.local
127.0.0.1 readdb.massgate.local
```

### Start Massgate

To start Massgate, you need to run the MMassgateServers.exe found in the 
build/bin folder with the following arguments:

```
MMassgateServers.exe live -noboom -all -dbname live -massgateport 3001 -logsql
```

But before you run the executable, make sure config.ini is present int the 
working directory of the executable.

If everything is okay you should see something like this in the a console
window:

```
2015-09-08 12:24:13 [2532] [INFO  ]  : Creating 16 handler threads (8 per core,
max 16).
2015-09-08 12:24:13 [2532] [INFO  ]  : Kickstarting server.
2015-09-08 12:24:13 [2532] [INFO  ]  : Server startup sequence OK.
```

### Running a Web Server

World in Conflict needs to connect to a web server in order to get information
about patches. Any simple web server that points to the `share/www-root`
folder should work. 

An easy way to run a web server is to simply use Python 
and start a server like this:

```
cd share\www-root
python -m SimpleHTTPServer 80
```

### Running a Dedicated Game Server

The dedicated game server is called `Wic_ds.exe` and is included in the 
distribution of the game. It is installed by Uplay upon download. 

To redirect the executable to a locally hosted Massgate server 
(running Massagate and the database) host names need to be redirected. 

In the Windows hosts file under `C:\Windows\System32\drivers\etc\hosts` 
add the following lines:

```
127.0.0.1 liveaccount.massgate.net
127.0.0.1 liveaccountbackup.massgate.net
127.0.0.1 stats.massgate.net
127.0.0.1 www.massgate.net  
```

In order for the dedicated game server to connect to Massgate, make sure the
Wic_ds.ini is set to report to Massgate.

```
[ReportToMassgate] 
1
```

### Connect With the Game

To make the game connect to Massgate you need to redirect host names on the
machine the game is running on as well. In the Windows hosts file 
under `C:\Windows\System32\drivers\etc\hosts` add the following lines:

```
127.0.0.1 liveaccount.massgate.net
127.0.0.1 liveaccountbackup.massgate.net
127.0.0.1 stats.massgate.net
127.0.0.1 www.massgate.net  
```

## Contributing 

The code is shared as is. Anyone is free to fork the code and do whathever
they want as long as the licences are respected. We will generally not accept 
pull requests as the source is shared to let the community 
take over and continue with Massgate.

If there is a major issue with the codebase that should apply to any version
of it we will consider such a request.

## Suggestions For Possible Improvements

Adding support for more databases, such as Sqllite or PostgreSQL, could ease
setup and hosting of Massgate. If you are interested take a look at the
MDatabase library.

There is also the possibility to recreate, or make a new modern improved 
Massgate web portal by using the data found in the database. The original
portal (not included in this repo) did just that.
