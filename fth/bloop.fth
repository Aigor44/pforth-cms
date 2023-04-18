

: BLOOP	 ( n -- n' )
	0 swap 0
	DO
		i +
		i 1 and
		IF
			dup dup	2 +
			swap - drop
		THEN
	LOOP
;


\ ." 	START" cr
\ 8000000 bloop .
\ ." END" cr


: uselocs  { aa bb -- }
	aa bb +
	aa bb -
	- drop
;

: BLOCS  (	N -- )
	0 DO i 77 uselocs LOOP
;


." 	START" cr
2000000 blocs
." END" cr
