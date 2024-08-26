cd ../bin
touch tmp.txt

for i in {8..20}
do
    echo $i
    mpsim.x -c1 -w --ds $i 1>/dev/null 2>/dev/null
    energy=$(cat stats.txt | grep "Total:" | head -1 | awk '{print $2}')
    echo -e "$((2 ** $i)) $energy" >> tmp.txt
done

mv tmp.txt ../bench/bench_dsize.txt
