IPP Project 1

author: xhanus19

Description:
A simple implementation of HTTP server.
App supports three basic commands, which are:
 - curl "address:port"/load
 - curl "address:port"/hostname
 - curl "address:port"/cpu-name

 Also command "GET" or "wget" can be used instead of "curl".

 Running the app:
  1) type "make"
  2) run "./hinfosvc 'port_number'" to start a HTTP server
  3) call one of the commands above from a different command line
  4) kill server using CTR+C

Sample usage:
 1) ./hinfosvc 12345
 2) curl "http://localhost:12345"/hostname
