.FUNCT main
print "Dies ist ein Test"
new_line
print "print 1 to exit"
read_char 1 -> sp
je 1 sp ?w
print "wrong"
quit
w:
print "correct"
quit

