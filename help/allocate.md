# ALLOCATE

## Name
   allocate [0] -- transfer global action points to star system

## Syntax
   allocate <action points>

## Description
    The allocate command transfers global action points to a star 
system. The one required parameter is the number of APs to 
transfer and this value must be a positive integer. Upon 
executing the command this amount will be transferred to the 
star system of the current scope. The scope can be set to a 
planet or lower, but allocate will not work from the universe 
(or root, '/') scope. The maximum number of APs you can have at 
the global and star system levels is a constant determined by 
the deity. The default value for this maximum is 255.
    Once executed, this command can not be reversed. Action points 
can only be transferred from the global level to a stellar 
level, not visa versa.

## Example
allocate 10     (transfers 10 APs from the global level to a star 
                    system's AP store)

## See Also
	actionpoints
