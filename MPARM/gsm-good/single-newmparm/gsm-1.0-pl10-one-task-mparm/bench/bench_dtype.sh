touch tmp.txt

for i in {1..16}
do
    echo $i
    mpsim.x -c1 -w --dt $i 1>/dev/null 2>/dev/null
    energy=$(cat stats.txt | grep "Total:" | head -1 | awk '{print $2}')
    echo -e "$i $energy" >> tmp.txt
done

mv tmp.txt bench_dassociativity.txt
