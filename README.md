# SO Project Chat Server
SO project by Marco Calamo and Ghenadie Artic
## Setup

### Client
Our Chat Client and Server are fully compatible with any Linux flavour and MacOs.
To make client work you need gtk and pkg-config libraries to be installed.

To install on Debian based distros:
```
sudo apt install libgtk-3-dev pkgconf
```
To install on Arch based distros:
```
sudo pacman -S gtk3 pkgconf
```
To install on MacOs(with brew): (Note: on MacOs you may need further configurations)
```
brew install gtk+3 pkg-config
```
Note: If you use other distros (i.e. OpenSuse, RedHat...) with different packet managers you probably already know the procedure

### Server
Our server is currently running on our remote RasperryPi 4B machine and accepting any request.
If you want to compile server on your machine you may need libpq for postgresql. It will connect to our remote DB on Raspberry.
To make client connect to local server you need to change AWS flag to 0 in _Client/client.h

To install on Debian based distros (You may need to change the include header to <postgres/libpq-fe.h>):
```
sudo apt install libpq-dev
```
To install on Arch based distros:
```
sudo pacman -S postgresql-libs
```
To install on MacOs(with brew): (Note: on MacOs you may need further configurations)
```
brew install libpq
``` 
Note: If you use other distros (i.e. OpenSuse, RedHat...) with different packet managers you probably already know the procedure

## Compiling
To compile both client and server type
```
make
```
Else type after make the target you choose (client/server)
## Execution
To start client or server type only 
```
./client
```
or
```
./server
```

## Database Structure

### Users
```
  Column  |         Type          | Collation | Nullable | Default | Storage  | Stats target | Description 
----------+-----------------------+-----------+----------+---------+----------+--------------+-------------
 username | character varying(32) |           | not null |         | extended |              | 
 password | character varying(32) |           |          |         | extended |              | 
```


### Messaggi
```
 Column |          Type           | Collation | Nullable | Default | Storage  | Stats target | Description 
--------+-------------------------+-----------+----------+---------+----------+--------------+-------------
 _from  | character varying(32)   |           | not null |         | extended |              | 
 _to    | character varying(32)   |           | not null |         | extended |              | 
 mes    | character varying(1024) |           | not null |         | extended |              | 
 data   | character varying(64)   |           | not null |         | extended |              | 
 ```
 
