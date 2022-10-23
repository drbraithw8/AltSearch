dict="/usr/share/dict/american-english"
brit="/usr/share/dict/british-english"
out="temp1"
./procDict $dict $brit temp4 >temp2 2>temp3
tr [A-Z] [a-z] < temp4 | sort > $out


