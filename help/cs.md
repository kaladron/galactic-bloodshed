# CS

## Name
   cs [0] -- change scope

## Syntax
   cs <-d> <path>
        where path = <#shipnumber> | < : > | < /[path] > | <..[/path]> 
		     | [name]

## Description
  cs changes from the current operative scope to the specified one.
If no scope is specified, cs will change to the default scope.
  If the -d option is specified, cs will make [path] the new default
scope.
  You cannot cs into areas that have not yet been explored by you.

## See Also
 scope, orbit
