*** Known Bugs
These are bugs know to exist in the current version. I will try to keep this as up to date as possible.





date of initial report: 4/18/25
** minor has temporary work around.

for some reason the program only runns the first half of the search area.
 a work around until found and fixxed is to double search area.
	-- this can also be done programmicly by doubling input value?
	--- might also be where we can find the bug.
	
	other notes about debugging:  hash is no longer used for its original purpose. we will now use it for passing degugging information.
	
	
	
	// ****************************************************************************************    fixxed
	
	
date of initial report: 4/18/25
** severe  inacurate results. 

The inacurate results change based on the start of the search range.  



-d 1 --keyspace 1:+100000000 12jbtzBb54r97TCwW3G1gCFoumpckRAPdY -o output1.txt
-d 1 --keyspace 2:+100000000 12jbtzBb54r97TCwW3G1gCFoumpckRAPdY -o output2.txt
-d 1 --keyspace 20:+100000000 12jbtzBb54r97TCwW3G1gCFoumpckRAPdY -o output20.txt
-d 1 --keyspace 200:+100000000 12jbtzBb54r97TCwW3G1gCFoumpckRAPdY -o output200.txt


-d 0 --keyspace 200000000:+20000000000 12jbtzBb54r97TCwW3G1gCFoumpckRAPdY -o output1.txt // intel i7 cpu
-d 1 --keyspace 200000000:+20000000000 12jbtzBb54r97TCwW3G1gCFoumpckRAPdY -o output2.txt // rtx3050

have different line 209 both valid??
have different line 210 both invalid??


all give a different incorect value on line 57 and some are wronge for line 56

fixxed 2/21/25
Turned out wss an array defined different sizes in different places.