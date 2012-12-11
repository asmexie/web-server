web-server
==========
Nayef Copty (nayefc) and K Al Najar (kar)

A multithreaded HTTP web server that provides access to web services and files. 
Compliant with HTTP/1.1, HTTP/1.0. The services provided follow the REST architecture style.

# Usage

## Make
To build, invoke:

    $ make clean all
    
## Arguments
```
-h prints help
-p port to accept HTTP client requests (required)
-R path that specifies the root directory for serving files
```

## Run
For the widgets to work proeprly, change the url in the <div> tags to match the server's IP and port that it's
running on.
To run on port 2000,

    $ ./sysstatd -p 2000

# Description
The main() method starts by parsing out the command line arguments. 
It then sets up the TCP protocol by creating a socket, binding it and listening to it. 
An accept() call is called in an infinite-loop which awaits incoming requests. 
An incoming request is duplicated into a different socket to keep the main socket available for more 
connections, and the HTTP transaction is dispatched into a new thread-pool from the previous project is 
used a static library, and is under this repository as a git submodule. 
The TCP protocol is handled in the main() function.

## System Status

The server offers a system status service, where a client can inquire about the server's status.

### /loadavg
The first service offered is a /loadavg request. The /loadavg will return a JSON string based on /proc/loadavg/.
The HTTP handler method checks if the request is a /loadavg request, and if it is, it will build the response
and send it to the client.

### /meminfo
The second service offered is a /meminfo request. The /meminfo will return a JSON string based on /proc/meminfo/.
The HTTP handler method checks if the request is a /meminfo request, and if it is, it will build the response
and send it to the client.

Both services support a callback argument for JavaScript and AJAX callback requests.

##  Serving Files
To support serving files, our server takes an -R argument which determines the path to the root directory 
of our server. The default path is /files. The server prevents requests to /files/../../ (up one directory)
to avoid serving other files on the server. If the path given is a valid path, the file is served to the 
client as a response.

## Synthetic Load Requests
For Synthetic Load Requests for the widget that is included in the server, the request is checked against
/runloop, /allocanon and /freeanon.

### /runloop
A new thread is dispatched which will run for 15 seconds and a JSON confirmation response is sent to the client.

### /allocanon
64MB are allocated using mmap(). The memory is made resident and stored in a list by the server.

### /freeanon
This will free the last allocated memory in the list.

Those can be observed in the widget.

## Multiple Client Support
To support multiple clients, each request is dispatched in a new thread while the main thread is awaiting new 
requests. Our thread pool is utilised here.

## Robustness
To pass robsutness tests, the server does not crash when an error is made while handling a request. This also
includes improper requests formats that are sent in.





















