# -*- coding: utf-8 -*-
# <nbformat>3.0</nbformat>

# <codecell>

#!/usr/bin/python           # This is client.py file

import socket               # Import socket module

s = socket.socket()         # Create a socket object
host = socket.gethostname() # Get local machine name
port = 4000                # Reserve a port for your service.

s.connect((host, port))
print s.recv(12)
print s.recv(12)
print s.recv(12)
s.send('hello big boy')
s.close                    # Close the socket when done

# <codecell>

s.close

# <codecell>


