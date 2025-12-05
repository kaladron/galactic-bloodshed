# ANNOUNCE

## Name
   announce [0] -- send message to other races in a star system

## Syntax
   announce <message>

## Description
    This command is used to communicate with other races in a star 
system. If your current scope is set to a star system or lower, 
and you inhabit this particular system (a ship is enaugh), a message is 
sent to all other races who inhabit this system, provided that their 
current scope also includes this system.
    If you do not inhabit the system, an error message is returned 
informing you of the situation. If your current scope is at the 
universe (or root '/') level, announce is identical to the 
broadcast  command.

## Example
announce Is there anybody out there?    (send a message to all 
                                            races inhabiting a star
                                            system whose current scope
                                            includes that system)

## See Also
	communications, broadcast, think, post
