#/usr/bin/awk -f
BEGIN {
	x = SUBSEP

	names = "A"x"B"x "C"x "D"x "E"x"F";
	split(names, sectors, x);
	sector = 1;
	segment = 0;
}
{
    segment +=1
       
    if (segment > 82) {
       sector += 1
       segment = 1
    }

    printf "Seg%c%d %s\n", sectors[sector], segment, $2
    
}
