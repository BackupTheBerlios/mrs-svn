#
# Convert "genemap" into "mimmap.txt"
#
# Version 1.0, 08-Mar-1996, JackL.
#
BEGIN {
	FS = "|"
}

{
	printf "Code    : %s\n", $1
	printf "MIM#    : %s\n", $10
	printf "Date    : %s.%s.%s\n", $4, $2, $3
	printf "Location: %s\n", $5
	printf "Symbol  : %s\n", $6
	if ( $7 == "C" ) status = "confirmed"
	if ( $7 == "P" ) status = "provisional"
	if ( $7 == "I" ) status = "inconsistent"
	if ( $7 == "L" ) status = "tentative"
	printf "Status  : %s (%s)\n", $7, status
	printf "Title   : %s %s\n", $8, $9
	printf "Method  : %s\n", $11
	printf "Comment : %s %s\n", $12, $13
	printf "Disorder: %s %s %s\n", $14, $15, $16
	mim = ""
	n = split(sprintf("%s %s %s", $14, $15, $16), a, " ")
	for ( i = 1; i <= n; i++ ) {
		if ( a[i] ~ /[0-9][0-9][0-9][0-9][0-9][0-9]/ ) {
			if ( mim == "" ) mim = a[i]
			else mim = sprintf("%s, %s", mim, a[i])
		}
	}
	printf "MIM_Dis : %s\n", mim
	printf "Mouse   : %s\n", $17
	printf "Refs    : %s\n", $18
	printf "//\n"
}
