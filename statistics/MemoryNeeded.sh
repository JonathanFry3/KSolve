#!
export LC_NUMERIC="en_US.utf8"
echo "Move Limit|Draw 1 Peak|Draw 3 Peak"
echo "-----------|------|--------"
for moveLimit in {10000000..150000000..10000000} 
do
        /bin/time --format '%M' ../ran --seed 18778 --end 1 --draw 1 -mv $moveLimit > ran1.out 2>pipe1.out
        peakMemory1=$(cat pipe1.out)
        peakMemory1M=$(($peakMemory1 / 1024))
        /bin/time --format '%M' ../ran --seed 14683 --end 1 --draw 3 -mv $moveLimit > ran3.out 2>pipe3.out
        peakMemory3=$(cat pipe3.out)
        peakMemory3M=$(($peakMemory3 / 1024))
        printf "%'.f|%'.fMB|%'.fMB\n" $moveLimit $peakMemory1M $peakMemory3M
done
