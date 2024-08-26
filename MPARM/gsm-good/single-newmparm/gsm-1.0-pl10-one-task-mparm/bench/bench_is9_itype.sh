cd ../bin
touch tmp.txt

for i in {1..16}
do
    echo $i
    mpsim.x -c1 -w --is 9 --it $i 1>/dev/null 2>/dev/null
    energy=$(cat stats.txt | grep "Total:" | head -1 | awk '{print $2}')
    echo -e "$i $energy" >> tmp.txt
done

cat tmp.txt
mv tmp.txt ../bench_is9_itype.txt
