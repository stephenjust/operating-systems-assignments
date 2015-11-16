CMPUT 397 Assignment 2 - Stephen Just
=====================================

This assignment was to build two web server programs that
either use forked processes or threads for each request.

The common parts of the two web-servers are separated into
server_common.{c,h}. f_server.c contains the forked-
process server, whereas p_server.c contains the pthread
server.
