\ compare dictionaries

anew comp
hex

: checksum  { start end -- sum }
	0
	end start
	DO
		i @ +
	4 +LOOP
;

: findword { target start end -- }
	end start
	DO
		i @  target =
		IF
			." found at " i u. cr
			i 16 dump
		THEN
	4 +LOOP
;

echo on
hex
$ 01500fc4 codebase here findword
codebase here cr .s checksum u. cr
namebase context @ cr .s checksum u. cr
decimal

echo off
